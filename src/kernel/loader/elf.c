#include "wyrd.h"
#include "elf.h"
#include "arch/i686/paging.h"
#include "lib/logger.h"
#include "lib/mem.h"
#include "mm/pmm.h"
#include "mm/memory.h"

#define kElfClass_32bit       0x01
#define kElfClass_64bit       0x02
#define kElfData_LittleEndian 0x01
#define kElfData_BigEndian    0x02
#define kElfType_Exec         0x02
#define kElfMachine_x86       0x03
#define kElfVer               0x01

#define kProgHeaderType_Load  0x01  // PT_LOAD

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

// validate ELF header, then process
ElfError elfLoad(ElfReadFn read, const void* context, u32 imageLen, AddressSpace* space, u32* outEntry)
{
   if (imageLen < sizeof(ELF32Header))
      return kElfErr_TooSmall;

   ELF32Header header = {0};
   ElfError err = read(context, 0, sizeof(ELF32Header), (void*)&header);
   if (err != kElfErr_OK)
      return err;

   if (header.ident[0] != 0x7F || header.ident[1] != 0x45 || 
       header.ident[2] != 0x4c || header.ident[3] != 0x46)
      return kElfErr_BadMagic;
   if (header.ident[kElfIdent_Class] != kElfClass_32bit)
      return kElfErr_Not32bit;
   if (header.ident[kElfIdent_Data] != kElfData_LittleEndian)
      return kElfErr_Not32bit;
   if (header.type != kElfType_Exec)
      return kElfErr_NotExec;
   if (header.machine != kElfMachine_x86)
      return kElfErr_BadMachine;

   u32 physTotal = header.phOffset + (header.phCount * header.phSize);
   if (physTotal > imageLen)
      return kElfErr_BadProgramHeader;

   ElfProgramHeader progHeader = {0};
   for (u32 i = 0; i < header.phCount; i++) {
      u32 offset = header.phOffset + (i * header.phSize);
      err = read(context, offset, sizeof(ElfProgramHeader), (void*)&progHeader);
      if (err != kElfErr_OK)
         return err;

      if (progHeader.type != kProgHeaderType_Load)
         continue;
      if (progHeader.virtAddr + progHeader.memSegSize > kKernelVirtualBase)
         return kElfErr_SegOutOfRange;
      if (progHeader.physSegSize > progHeader.memSegSize)
         return kElfErr_SegOutOfRange;

      u32 vaStart = progHeader.virtAddr & ~0xFFF;
      u32 vaEnd   = (progHeader.virtAddr + progHeader.memSegSize + 0xFFF) & ~0xFFF;
      for (u32 addr = vaStart; addr < vaEnd; addr += kPageSize) {
         if (pagingIsMapped(addr))
            continue;

         kTrace("elfLoad: allocating frame for user process");
         u32 frame = pmmAllocFrame();
         if (frame == kInvalidFrame)
            return kElfErr_NoMemory;

         if (!pagingMapPage(addr, frame, kPageFlag_Writable | kPageFlag_User))
            return kElfErr_NoMemory;

         memset((void*)addr, 0, kPageSize);
      }

      err = read(context, progHeader.offset, progHeader.physSegSize, (void*)progHeader.virtAddr);
      if (err != kElfErr_OK)
         return kElfErr_ReadFailed;
   }

   *outEntry = header.entry;
   return kElfErr_OK;
}
