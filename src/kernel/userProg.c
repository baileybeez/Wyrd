#include "wyrd.h"
#include "arch/i686/paging.h"
#include "userProg.h"
#include "lib/logger.h"
#include "lib/mem.h"
#include "mm/pmm.h"
#include "scheduler/thread.h"

extern char _binary_build_tests_userProg_bin_start[];
extern char _binary_build_tests_userProg_bin_end[];

Thread* spawnUserThread()
{
   const u32 blobVa  = 0x00401000;
   const u32 stackVa = 0x00500000;

   u32 blobLen = (u32)_binary_build_tests_userProg_bin_end
               - (u32)_binary_build_tests_userProg_bin_start;

   u32 blobFrame = pmmAllocFrame();
   if (blobFrame == kInvalidFrame) { kError("spawn: no blob frame"); return nil; }

   if (!pagingMapPage(blobVa, blobFrame, kPageFlag_Writable | kPageFlag_User)) {
      kError("spawn: blob map failed");
      pmmFreeFrame(blobFrame);
      return nil;
   }
   memcpy((void*)blobVa, _binary_build_tests_userProg_bin_start, blobLen);

   u32 stackFrame = pmmAllocFrame();
   if (stackFrame == kInvalidFrame) { kError("spawn: no stack frame"); return nil; }

   if (!pagingMapPage(stackVa, stackFrame, kPageFlag_Writable | kPageFlag_User)) {
      kError("spawn: stack map failed");
      pmmFreeFrame(stackFrame);
      return nil;
   }

   u32 userStackTop = stackVa + kPageSize;

   kTrace("spawn: user thread entry %x, stack %x (blob %d bytes)",
          blobVa, userStackTop, blobLen);
   return threadCreateUser(blobVa, userStackTop);
}
