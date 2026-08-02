#include "wyrd.h"
#include "elf.h"
#include "arch/i686/paging.h"
#include "lib/logger.h"
#include "lib/mem.h"
#include "mm/pmm.h"
#include "mm/memory.h"
#include "scheduler/scheduler.h"

#define kElfClass_32bit       0x01
#define kElfClass_64bit       0x02
#define kElfData_LittleEndian 0x01
#define kElfData_BigEndian    0x02
#define kElfType_Exec         0x02
#define kElfMachine_x86       0x03
#define kElfVer               0x01

#define kProgHeaderType_Load  0x01  // PT_LOAD

#define kMaxProgHeaders       8
#define kMaxProgHeaderSize    256


// elf header     sz       info
// [magic]        4               :: 0x7f 0x45 0x4c 0x46
// class          1        format :: 1 (32-bit), 2 (64-bit)
// data           1        endian :: 1 (little), 2 (big)
// version        1        1
// ABI            1        System ABI
// ABI ver        1        ABI version
// PAD            7        0x00
// type           2        file type    (0x02 == EXEC)
// machine        2        architecture (0x03 == x86)
// version        4        1
#define kElfIdent_Class 5
#define kElfIdent_Data  6
#define kElfIdent_Ver   7

static bool _elfRangeValid(u32 offset, u32 len, u32 limit)
{
   if (len > limit)
      return false;
   if (offset > limit - len)
      return false;

   return true;
}

static ElfError _validateElfHeader(const ELF32Header* hdr, u32 imageLen)
{
   if (hdr->ident[0] != 0x7F || hdr->ident[1] != 0x45 || hdr->ident[2] != 0x4c || hdr->ident[3] != 0x46)
      return kElfErr_BadMagic;
   if (hdr->ident[kElfIdent_Class] != kElfClass_32bit)
      return kElfErr_Not32bit;
   if (hdr->ident[kElfIdent_Data] != kElfData_LittleEndian)
      return kElfErr_BadEndian;
   if (hdr->type != kElfType_Exec)
      return kElfErr_NotExec;
   if (hdr->machine != kElfMachine_x86)
      return kElfErr_BadMachine;

   if (hdr->phSize < sizeof(ElfProgramHeader) || hdr->phSize > kMaxProgHeaderSize)
      return kElfErr_BadProgramHeader;
   if (hdr->phCount == 0 || hdr->phCount > kMaxProgHeaders)
      return kElfErr_BadProgramHeader;
   if (!_elfRangeValid(hdr->phOffset, hdr->phCount * hdr->phSize, imageLen))
      return kElfErr_BadProgramHeader;
 
   if (hdr->entry < kPageSize || hdr->entry >= kUserStackBase)
      return kElfErr_BadEntry;
 
   return kElfErr_OK;
}

static ElfError _validateSegments(ElfReadFn read, const void* context, const ELF32Header* hdr, 
                           u32 imageLen, ElfProgramHeader* outHeaders, u32* outCount)
{
   u32 count = 0;
   for (u32 i = 0; i < hdr->phCount; i++) {
      ElfProgramHeader progHeader = {0};
      u32 offset = hdr->phOffset + (i * hdr->phSize);
      ElfError err = read(context, offset, sizeof(ElfProgramHeader), (void*)&progHeader);
      if (err != kElfErr_OK)
         return err;

      if (progHeader.type != kProgHeaderType_Load)
         continue;
      if (progHeader.memSegSize == 0)
         continue;
 
      if (progHeader.physSegSize > progHeader.memSegSize)
         return kElfErr_SegOutOfRange;
      if (!_elfRangeValid(progHeader.offset, progHeader.physSegSize, imageLen))
         return kElfErr_SegOutOfRange;
      if (progHeader.virtAddr < kPageSize)
         return kElfErr_SegOutOfRange;
      if (!_elfRangeValid(progHeader.virtAddr, progHeader.memSegSize, kUserStackBase))
         return kElfErr_SegOutOfRange;
 
      outHeaders[count++] = progHeader;
   }
 
   if (count == 0)
      return kElfErr_BadProgramHeader;
 
   *outCount = count;
   return kElfErr_OK;
}

static ElfError _elfLoadSegments(ElfReadFn read, const void* context, const ElfProgramHeader* progHeader)
{
   u32 vaStart = progHeader->virtAddr & ~0xFFF;
   u32 vaEnd   = (progHeader->virtAddr + progHeader->memSegSize + 0xFFF) & ~0xFFF;

   for (u32 addr = vaStart; addr < vaEnd; addr += kPageSize) {
      if (pagingIsMapped(addr))
         continue;

      kTrace("elfLoad: allocating frame for user process");
      u32 frame = pmmAllocFrame();
      if (frame == kInvalidFrame)
         return kElfErr_NoMemory;

      if (!pagingMapPage(addr, frame, kPageFlag_Writable | kPageFlag_User)) {
         pmmFreeFrame(frame);
         return kElfErr_NoMemory;
      }

      memset((void*)addr, 0, kPageSize);
   }

   if (progHeader->physSegSize == 0)
      return kElfErr_OK;

   return read(context, progHeader->offset, progHeader->physSegSize, (void*)progHeader->virtAddr);
}

static ElfError _elfLoadInner(ElfReadFn read, const void* context, const ElfProgramHeader* progHeaders, 
                              u32 count, u32 entry)
{
   for (u32 i = 0; i < count; i++) {
      ElfError err = _elfLoadSegments(read, context, &progHeaders[i]);
      if (err != kElfErr_OK)
         return err;
   }

   if (!pagingIsMapped(entry & ~0xFFF))
      return kElfErr_BadEntry;
 
   return kElfErr_OK;

}

// validate ELF header, then process
ElfError elfLoad(ElfReadFn read, const void* context, u32 imageLen, AddressSpace* space, u32* outEntry)
{
   if (read == nil || space == nil || outEntry == nil)
      return kElfErr_BadArgument;
   if (imageLen < sizeof(ELF32Header))
      return kElfErr_TooSmall;

   ELF32Header header = {0};
   ElfError err = read(context, 0, sizeof(ELF32Header), (void*)&header);
   if (err != kElfErr_OK)
      return err;

   err = _validateElfHeader(&header, imageLen);
   if (err != kElfErr_OK)
      return err;

   ElfProgramHeader progHeaders[kMaxProgHeaders] = {0};
   u32 count = 0;
   err = _validateSegments(read, context, &header, imageLen, progHeaders, &count);
   if (err != kElfErr_OK)
      return err;

   AddressSpace* prev = schedulerCurrentSpace();
   schedulerSwitchAddressSpace(space);
   err = _elfLoadInner(read, context, progHeaders, count, header.entry);
   schedulerSwitchAddressSpace(prev);

   if (err != kElfErr_OK)
      return err;
      
   *outEntry = header.entry;
   return kElfErr_OK;
}
