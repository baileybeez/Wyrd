#include "wyrd.h"
#include "elf.h"
#include "arch/i686/paging.h"
#include "lib/mem.h"
#include "drivers/serial/serial.h"

#ifdef kIncludeSelfTests

#define kTestEntry   0x00400000u   // free user VA; matches where the real program links
#define kTestFileSz  8u
#define kTestMemSz   0x1010u       // > filesz and > one page: exercises .bss + multi-page

static const u8 _kPattern[kTestFileSz] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

static u8 _image[128];

typedef struct {
   const u8* base;
   u32       len;
} BufReader;

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

bool elfSelfTest()
{
   serialWriteString("elfSelfTest: begin\n");

   u32 imageLen = _buildImage();

   BufReader br = { _image, imageLen };
   u32 entry = 0;
   ElfError rc = elfLoad(_bufRead, &br, imageLen, &entry);

   bool ok = true;
   ok = _expect(rc == kElfErr_OK, "elfLoad returns OK") && ok;
   ok = _expect(entry == kTestEntry, "entry point carried verbatim") && ok;

   const u8* seg = (const u8*)kTestEntry;

   bool filesOk = (rc == kElfErr_OK);
   for (u32 i = 0; i < kTestFileSz && filesOk; i++)
      filesOk = (seg[i] == _kPattern[i]);
   ok = _expect(filesOk, "p_filesz bytes copied to p_vaddr") && ok;

   bool bssOk = (rc == kElfErr_OK);
   for (u32 i = kTestFileSz; i < kTestMemSz && bssOk; i++)
      bssOk = (seg[i] == 0);
   ok = _expect(bssOk, "bss tail zeroed (memsz > filesz, spans page)") && ok;

   // release the mapping so the real loader can reuse kTestEntry cleanly
   if (rc == kElfErr_OK) {
      u32 vaStart = kTestEntry & ~0xFFF;
      u32 vaEnd   = (kTestEntry + kTestMemSz + 0xFFF) & ~0xFFF;
      for (u32 va = vaStart; va < vaEnd; va += kPageSize)
         pagingUnmapPage(va);
   }

   serialWriteString(ok ? "elfSelfTest: PASS\n" : "elfSelfTest: FAIL\n");
   return ok;
}

#endif //kIncludeSelfTests
