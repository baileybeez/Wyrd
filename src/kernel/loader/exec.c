#include "wyrd.h"
#include "exec.h"
#include "elf.h"
#include "arch/i686/paging.h"
#include "lib/logger.h"
#include "lib/mem.h"
#include "mm/memory.h"
#include "mm/heap.h"
#include "mm/pmm.h"
#include "fs/vfs/vfs.h"
#include "scheduler/thread.h"
#include "scheduler/scheduler.h"
#include "string.h"

#define kMaxArgsLength  256

static ElfError _bufferRead(const void* ctx, u32 off, u32 len, void* dst)
{
   BufReader* b = (BufReader*)ctx;
   if (off > b->len || len > b->len - off)
      return kElfErr_TooSmall;

   memcpy(dst, b->base + off, len);
   return kElfErr_OK;
}

static u8* _execReadImage(const char* path, u32* outLen)
{
   *outLen = 0;

   File file = {0};
   VfsError err = vfsFileOpen(path, &file);
   if (err != kVfsErr_OK)
      return nil;
   if (file.size == 0)
      return nil;

   u8* buffer = kmalloc(file.size);
   if (buffer == nil)
      return nil;

   u32 bytesRead = 0;
   err = vfsFileRead(&file, buffer, file.size, &bytesRead);
   if (err != kVfsErr_OK || bytesRead != file.size) {
      kfree(buffer);
      return nil;
   }

   *outLen = bytesRead;
   return buffer;
}

static u32 _execBuildUserStackFrame(u32 stackTop, u32 argc, const char* blob, u32 blobLen)
{
   u32 base = (stackTop - blobLen) & ~0x03;  // round down so the pointer array stays aligned
   if (base - (argc + 3) * sizeof(u32) < kUserStackBase)
      return 0;

   memcpy((void*)base, blob, blobLen);

   u32* sp = (u32*)base;
   *(--sp) = 0;
   *(--sp) = 0;

   sp -= argc;
   u32 offset = 0;
   for (u32 i = 0; i < argc; i++) {
      sp[i]   = base + offset;
      offset += strlen(blob + offset) + 1;
   }

   *(--sp) = argc;
   return (u32)sp;
}

static bool _execMapUserStack(AddressSpace* space, u32 argc, const char* blob, u32 blobLen, u32* outStackTop)
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
   
   u32 userEsp = _execBuildUserStackFrame(kUserStackTop, argc, blob, blobLen);
   schedulerSwitchAddressSpace(prev);
   if (!ok)
      return false;

   *outStackTop = userEsp;
   return true;
}

static i32 _execFlattenArgs(char* blob, u32 max, u32 argc, const char* args[])
{
   u32 len = 0;
   for (u32 i = 0; i < argc; i++) {
      u32 n = strlen(args[i]) + 1;
      if (len + n > max)
         return false;

      memcpy(blob + len, args[i], n);
      len += n;
   }

   return (i32)len;
}

// Executing a file from disk
//
//    1. find the file on disk
//    2. alloc a buffer and read the image from disk
//    3. parse Elf header and read program into virual memory
//       3.5. free buffer from disk image (#2)
//    4. alloc a frame for a stack, map it into paging
//    5. create the user thread
Thread* execFromDisk(const char* path, u32 argc, char* argv[], ElfError* outError)
{
   u32 fileSize = 0;
   u8* buffer = _execReadImage(path, &fileSize);
   if (buffer == nil)
      return nil;

   AddressSpace* space = addressSpaceCreate();
   if (space == nil) {
      kfree(buffer);
      return nil;
   }
   
   char blob[kMaxArgsLength];
   i32 blobLen = _execFlattenArgs(blob, kMaxArgsLength, argc, argv);
   if (blobLen < 0) {
      addressSpaceDestroy(space);
      kfree(buffer);
      return nil;
   }

   BufReader buf = { .base = buffer, .len = fileSize };
   
   u32 entryPoint = 0;
   ElfError elfErr = elfLoad(_bufferRead, (const void*)&buf, buf.len, space, &entryPoint);   
   kfree(buffer);
   if (outError != nil)
         *outError = elfErr;

   if (elfErr != kElfErr_OK) {
      addressSpaceDestroy(space);
      return nil;
   }

   u32 stackTop = 0;
   if (!_execMapUserStack(space, argc, blob, blobLen, &stackTop)) {
      addressSpaceDestroy(space);
      return nil;
   }

   const char* q = path;
   if (*q == kPathSep)
      q++;

   const char* pname = q;
   while (*q != '\0') {
      if (*q++ == kPathSep) {
         pname = q;
      }
   }
   
   kTrace("launched: %s", pname);
   Thread* thread = threadCreateUser(entryPoint, pname, stackTop, space);
   if (thread == nil) {
      addressSpaceDestroy(space);
      return nil;
   }

   return thread;
}
