#include "wyrd.h"
#include "exec.h"
#include "elf.h"
#include "arch/i686/paging.h"
#include "lib/logger.h"
#include "lib/mem.h"
#include "mm/memory.h"
#include "mm/heap.h"
#include "mm/pmm.h"
#include "fs/fat16/fat16.h"
#include "scheduler/thread.h"
#include "scheduler/scheduler.h"

static ElfError _bufferRead(const void* ctx, u32 off, u32 len, void* dst)
{
   BufReader* b = (BufReader*)ctx;
   if (off > b->len || len > b->len - off)
      return kElfErr_TooSmall;

   memcpy(dst, b->base + off, len);
   return kElfErr_OK;
}

static u8* _execReadImage(const Fat16Volume* vol, const char* path, u32* outLen)
{
   u16 firstCluster = 0;
   u32 fileSize = 0;
   Fat16Error fatErr = fat16FindFile(vol, path, &firstCluster, &fileSize);
   if (fatErr != kFatErr_OK)
      return nil;
   if (fileSize == 0)
      return nil;

   u8* buffer = kmalloc(fileSize);
   if (buffer == nil)
      return nil;

   fatErr = fat16ReadFile(vol, firstCluster, fileSize, (void*)buffer);
   if (fatErr != kFatErr_OK) {
      kfree(buffer);
      return nil;
   }

   *outLen = fileSize;
   return buffer;
}

static u32 _execBuildUserStackFrame(u32 stackTop)
{
   u32* sp = (u32*)stackTop;

   *(--sp) = 0;      // envp[0]
   *(--sp) = 0;      // argv[0]
   *(--sp) = 0;      // argc

   return (u32)sp;
}

static bool _execMapUserStack(AddressSpace* space, u32* outStackTop)
{
   kTrace("execFromDisk: mapping %u user stack pages", kUserStackPages);

   AddressSpace* prev = schedulerCurrentSpace();
   schedulerSwitchAddressSpace(space);

   bool ok = true;
   for (u32 vaStack = kUserStackBase; vaStack < kUserStackTop; vaStack += kPageSize) {
      if (pagingIsMapped(vaStack))
         continue;

      u32 stackFrame = pmmAllocFrame();
      if (stackFrame == kInvalidFrame) {
         ok = false;
         break;
      }

      if (!pagingMapPage(vaStack, stackFrame, kPageFlag_Writable | kPageFlag_User)) {
         pmmFreeFrame(stackFrame);
         ok = false;
         break;
      }

      memset((void*)vaStack, 0x00, kPageSize);
   }
   
   u32 userEsp = _execBuildUserStackFrame(kUserStackTop);
   schedulerSwitchAddressSpace(prev);
   if (!ok)
      return false;

   *outStackTop = userEsp;
   return true;
}

// Executing a file from disk
//
//    1. find the file on disk
//    2. alloc a buffer and read the image from disk
//    3. parse Elf header and read program into virual memory
//       3.5. free buffer from disk image (#2)
//    4. alloc a frame for a stack, map it into paging
//    5. create the user thread
Thread* execFromDisk(const Fat16Volume* vol, const char* path, ElfError* outError)
{
   u32 fileSize = 0;
   u8* buffer = _execReadImage(vol, path, &fileSize);
   if (buffer == nil)
      return nil;

   AddressSpace* space = addressSpaceCreate();
   if (space == nil) {
      kfree(buffer);
      return nil;
   }
   
   BufReader buf = { .base = buffer, .len = fileSize };
   
   u32 entryPoint = 0;
   ElfError elfErr = elfLoad(_bufferRead, (const void*)&buf, buf.len, space, &entryPoint);   
   kfree(buffer);

   if (elfErr != kElfErr_OK) {
      addressSpaceDestroy(space);
      if (outError != nil)
         *outError = elfErr;
      return nil;
   }

   u32 stackTop = 0;
   if (!_execMapUserStack(space, &stackTop)) {
      addressSpaceDestroy(space);
      return nil;
   }
   
   Thread* thread = threadCreateUser(entryPoint, stackTop, space);
   if (thread == nil) {
      addressSpaceDestroy(space);
      return nil;
   }

   return thread;
}
