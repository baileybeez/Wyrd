#include "wyrd.h"
#include "exec.h"
#include "elf.h"
#include "arch/i686/paging.h"
#include "lib/mem.h"
#include "mm/memory.h"
#include "mm/heap.h"
#include "mm/pmm.h"
#include "fs/fat16/fat16.h"
#include "scheduler/thread.h"

#define kUserStackTop  kKernelVirtualBase

static ElfError _bufferRead(const void* ctx, u32 off, u32 len, void* dst)
{
   BufReader* b = (BufReader*)ctx;
   if (off > b->len || len > b->len - off)
      return kElfErr_TooSmall;

   memcpy(dst, b->base + off, len);
   return kElfErr_OK;
}

// Executing a file from disk
//
//    1. find the file on disk
//    2. alloc a buffer and read the image from disk
//    3. parse Elf header and read program into virual memory
//       3.5. free buffer from disk image (#2)
//    4. alloc a frame for a stack, map it into paging
//    5. create the user thread
Thread* execFromDisk(const Fat16Volume* vol, const char* path)
{
   u16 firstCluster = 0;
   u32 fileSize = 0;
   Fat16Error fatErr = fat16FindFile(vol, path, &firstCluster, &fileSize);
   if (fatErr != kFatErr_OK)
      return nil;

   u8* buffer = kmalloc(fileSize);

   fatErr = fat16ReadFile(vol, firstCluster, fileSize, (void*)buffer);
   if (fatErr != kFatErr_OK) {
      kfree(buffer);
      return nil;
   }
   
   BufReader buf = {0};
   buf.len  = fileSize;
   buf.base = buffer;

   u32 entryPoint = 0;
   ElfError elfErr = elfLoad(_bufferRead, (const void*)&buf, buf.len, &entryPoint);
   
   kfree(buffer);
   if (elfErr != kElfErr_OK) {
      return nil;
   }

   u32 stackFrame = pmmAllocFrame();
   if (stackFrame == kInvalidFrame) {
      // clean up elf allocated frames?
      return nil;
   }

   u32 vaStack = kUserStackTop - kPageSize;
   if (!pagingMapPage(vaStack, stackFrame, kPageFlag_Writable | kPageFlag_User)) {
      // clean up elf allocated frames?
      return nil;
   }
   u32 stackTop = vaStack + kPageSize;

   return threadCreateUser(entryPoint, stackTop);
}
