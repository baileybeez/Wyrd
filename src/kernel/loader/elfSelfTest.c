#include "wyrd.h"
#include "elf.h"
#include "exec.h"
#include "arch/i686/paging.h"
#include "lib/mem.h"
#include "scheduler/scheduler.h"
#include "drivers/serial/serial.h"

#ifdef kIncludeSelfTests

#define kTestEntry   0x00400000u   // free user VA; matches where the real program links
#define kTestFileSz  8u
#define kTestMemSz   0x1010u       // > filesz and > one page: exercises .bss + multi-page

static const u8 _kPattern[kTestFileSz] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

static u8 _image[128];

static bool _expect(bool cond, const char* label)
{
   serialWriteString(cond ? "  [ok]   " : "  [FAIL] ");
   serialWriteString(label);
   serialWriteString("\n");
   return cond;
}

static ElfError _bufRead(const void* ctx, u32 off, u32 len, void* dst)
{
   BufReader* b = (BufReader*)ctx;
   if (off > b->len || len > b->len - off)
      return kElfErr_TooSmall;
   memcpy(dst, b->base + off, len);
   return kElfErr_OK;
}

static u32 _buildImage(void)
{
   memset(_image, 0, sizeof(_image));

   ELF32Header* eh = (ELF32Header*)_image;
   eh->ident[0]   = 0x7F;
   eh->ident[1]   = 'E';
   eh->ident[2]   = 'L';
   eh->ident[3]   = 'F';
   eh->ident[4]   = 1;                       // ELFCLASS32
   eh->ident[5]   = 1;                       // ELFDATA2LSB
   eh->ident[6]   = 1;                       // EV_CURRENT
   eh->type       = 2;                       // ET_EXEC
   eh->machine    = 3;                       // EM_386
   eh->version    = 1;
   eh->entry      = kTestEntry;
   eh->phOffset   = sizeof(ELF32Header);
   eh->headerSize = sizeof(ELF32Header);
   eh->phSize     = sizeof(ElfProgramHeader);
   eh->phCount    = 1;

   u32 phOff  = sizeof(ELF32Header);
   u32 segOff = phOff + sizeof(ElfProgramHeader);

   ElfProgramHeader* ph = (ElfProgramHeader*)(_image + phOff);
   ph->type        = 1;                      // PT_LOAD
   ph->offset      = segOff;
   ph->virtAddr    = kTestEntry;
   ph->physAddr    = kTestEntry;
   ph->physSegSize = kTestFileSz;
   ph->memSegSize  = kTestMemSz;
   ph->flags       = 0;
   ph->align       = kPageSize;

   memcpy(_image + segOff, _kPattern, kTestFileSz);

   return segOff + kTestFileSz;
}

// caller must already be switched into the space the segments were loaded into
static bool _verifyImage(void)
{
   const u8* seg = (const u8*)kTestEntry;
 
   bool fileOk = true;
   for (u32 i = 0; i < kTestFileSz && fileOk; i++)
      fileOk = (seg[i] == _kPattern[i]);
 
   bool bssOk = true;
   for (u32 i = kTestFileSz; i < kTestMemSz && bssOk; i++)
      bssOk = (seg[i] == 0);
 
   bool ok = _expect(fileOk, "p_filesz bytes copied to p_vaddr");
   return _expect(bssOk, "bss tail zeroed (memsz > filesz, spans page)") && ok;
}


bool elfSelfTest()
{
   serialWriteString("elfSelfTest: begin\n");

   u32 imageLen = _buildImage();

   AddressSpace* space = addressSpaceCreate();   
   if (!_expect(space != nil, "address space created")) {
      serialWriteString("elfSelfTest: FAIL\n");
      return false;
   }

   BufReader br = { _image, imageLen };
   u32 entry = 0;
   ElfError rc = elfLoad(_bufRead, &br, imageLen, space, &entry);

   bool ok = true;
   ok = _expect(rc == kElfErr_OK, "elfLoad returns OK") && ok;
   ok = _expect(entry == kTestEntry, "entry point carried verbatim") && ok;

   if (rc == kElfErr_OK) {
      AddressSpace* prev = schedulerCurrentSpace();
      schedulerSwitchAddressSpace(space);
      bool imageOk = _verifyImage();
      schedulerSwitchAddressSpace(prev);
      ok = imageOk && ok;
   }
 
   // must be back on `prev` before this runs — destroying the live CR3 faults
   addressSpaceDestroy(space);

   serialWriteString(ok ? "elfSelfTest: PASS\n" : "elfSelfTest: FAIL\n");
   return ok;
}

#endif //kIncludeSelfTests
