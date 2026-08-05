#include "wyrd.h"
#include "exec.h"
#include "arch/i686/cpu.h"
#include "arch/i686/ticks.h"
#include "fs/fat16/fat16.h"
#include "lib/logger.h"
#include "mm/heap.h"
#include "mm/pmm.h"
#include "scheduler/scheduler.h"
#include "scheduler/thread.h"

#ifdef kIncludeSelfTests

#define kLifecycleIterations 20
#define kReaperTimeoutTicks  50

static volatile i32 _quickCode = 0;
static void _quickExitThread()
{
   schedulerExitThread(_quickCode);
}

static bool _threadIsGone(u32 id)
{
   u32 flags = irqSave();
   bool gone = (threadFind(id) == nil);
   irqRestore(flags);
   return gone;
}

static bool _threadIsReaped(u32 id)
{
   u32 flags = irqSave();
   Thread* t = threadFind(id);
   bool reaped = (t != nil && t->state == kThreadState_Reaped);
   irqRestore(flags);
   return reaped;
}

static bool _spinUntil(bool (*predicate)(u32), u32 id)
{
   u32 deadline = ticksGetCount() + kReaperTimeoutTicks;
   while (ticksGetCount() < deadline) {
      if (predicate(id))
         return true;
 
      schedulerYield();
   }
 
   return predicate(id);
}

static void _reportDelta(const char* label, u32 framesBefore, u32 heapBefore)
{
   u32 framesAfter = pmmFreeFrames();
   u32 heapAfter   = heapFreeBytes();

   u32 framesConsumed = framesBefore - framesAfter;
   u32 heapGrowth     = framesConsumed * kFrameSize;
   i32 heapDelta      = (i32)(heapAfter - heapBefore);
 
   if (framesAfter <= framesBefore && heapDelta == (i32)heapGrowth) {
      kTrace("[lifecycle] %s: CLEAN (frames=%u heap=%u)", label, framesAfter, heapAfter);
      return;
   }
 
   kError("[lifecycle] %s: LEAK frames %u -> %u (%i), heap %u -> %u (%i)",
      label,
      framesBefore, framesAfter, (i32)(framesAfter - framesBefore),
      heapBefore,   heapAfter,   (i32)(heapAfter - heapBefore));
}

// Tier 1 - detached kernel thread: struct + kernel stack reclaimed
static void _lifecycleTierOne()
{
   kTrace("[lifecycle] tier 1: detached kernel threads");
 
   u32 framesBefore = pmmFreeFrames();
   u32 heapBefore   = heapFreeBytes();
 
   for (u32 i = 0; i < kLifecycleIterations; i++) {
      _quickCode = (i32)i;
 
      Thread* t = threadCreate(_quickExitThread);
      if (t == nil) {
         kError("[lifecycle] tier 1: threadCreate failed at %u", i);
         return;
      }
 
      u32 id = t->id;
      if (!_spinUntil(_threadIsGone, id)) {
         kError("[lifecycle] tier 1: thread %u never left the registry", id);
         return;
      }
   }
 
   _reportDelta("tier 1", framesBefore, heapBefore);
}

// Tier 2 - user thread: frames, page tables, directory, stack, struct
static void _lifecycleTierTwo(const Fat16Volume* vol, const char* path, i32 expectedCode)
{
   kTrace("[lifecycle] tier 2: user threads via %s", path);
 
   u32 framesBefore = pmmFreeFrames();
   u32 heapBefore   = heapFreeBytes();
 
   for (u32 i = 0; i < kLifecycleIterations; i++) {
      Thread* t = execFromDisk(vol, path);
      if (t == nil) {
         kError("[lifecycle] tier 2: execFromDisk failed at %u", i);
         return;
      }
 
      u32 id   = t->id;
      i32 code = 0;
 
      WaitError err = threadWait(id, &code);
      if (err != kWaitErr_OK) {
         kError("[lifecycle] tier 2: threadWait(%u) returned %u", id, err);
         return;
      }
 
      if (code != expectedCode) {
         kError("[lifecycle] tier 2: exit code %i, expected %i", code, expectedCode);
         return;
      }
 
      if (!_spinUntil(_threadIsGone, id)) {
         kError("[lifecycle] tier 2: tombstone %u survived", id);
         return;
      }
   }
 
   _reportDelta("tier 2", framesBefore, heapBefore);
}

// Tier 3 -- both threadWait orderings, plus the error returns
static void _lifecycleWaiterFirst(const Fat16Volume* vol, const char* path)
{
   u32 framesBefore = pmmFreeFrames();
   u32 heapBefore   = heapFreeBytes();
 
   Thread* t = execFromDisk(vol, path);
   if (t == nil) {
      kError("[lifecycle] tier 3a: execFromDisk failed");
      return;
   }
 
   u32 id   = t->id;
   i32 code = 0;
 
   // no yield here: the child is still Ready, so threadWait must block
   WaitError err = threadWait(id, &code);
   if (err != kWaitErr_OK) {
      kError("[lifecycle] tier 3a: threadWait returned %u", err);
      return;
   }
 
   kTrace("[lifecycle] tier 3a: blocked path took code %i", code);
   if (!_spinUntil(_threadIsGone, id)) {
      kError("[lifecycle] tier 3a: tombstone %u survived", id);
      return;
   }
 
   _reportDelta("tier 3a (waiter first)", framesBefore, heapBefore);
}

static void _lifecycleReaperFirst(const Fat16Volume* vol, const char* path)
{
   u32 framesBefore = pmmFreeFrames();
   u32 heapBefore   = heapFreeBytes();
 
   Thread* t = execFromDisk(vol, path);
   if (t == nil) {
      kError("[lifecycle] tier 3b: execFromDisk failed");
      return;
   }
 
   u32 id = t->id;
 
   // let the child exit AND the reaper run before we ever wait
   if (!_spinUntil(_threadIsReaped, id)) {
      kError("[lifecycle] tier 3b: thread %u never reached Reaped", id);
      return;
   }
 
   i32 code = 0;
   WaitError err = threadWait(id, &code);
   if (err != kWaitErr_OK) {
      kError("[lifecycle] tier 3b: threadWait returned %u", err);
      return;
   }
 
   kTrace("[lifecycle] tier 3b: tombstone path took code %i", code);
   if (!_threadIsGone(id)) {
      kError("[lifecycle] tier 3b: threadWait did not free tombstone %u", id);
      return;
   }
 
   _reportDelta("tier 3b (reaper first)", framesBefore, heapBefore);
}

static void _lifecycleErrorReturns()
{
   i32 code = 0;
 
   WaitError err = threadWait(0xDEADBEEF, &code);
   if (err != kWaitErr_NoSuchThread)
      kError("[lifecycle] tier 3c: unknown tid returned %u, expected NoSuchThread", err);
 
   err = threadWait(schedulerCurrent()->id, &code);
   if (err != kWaitErr_Self)
      kError("[lifecycle] tier 3c: self returned %u, expected Self", err);
 
   Thread* detached = threadCreate(_quickExitThread);
   if (detached != nil) {
      u32 id = detached->id;
      err = threadWait(id, &code);
      if (err != kWaitErr_NoSuchThread)
         kError("[lifecycle] tier 3c: detached returned %u, expected NoSuchThread", err);
 
      _spinUntil(_threadIsGone, id);
   }
 
   kTrace("[lifecycle] tier 3c: error returns checked");
}

void lifecycleSelfTest(const Fat16Volume* vol, const char* path, i32 expectedCode)
{
   kTrace("[lifecycle] baseline frames=%u heap=%u", pmmFreeFrames(), heapFreeBytes());
   
   _lifecycleTierOne();
   _lifecycleTierTwo(vol, path, expectedCode);
   _lifecycleWaiterFirst(vol, path);
   _lifecycleReaperFirst(vol, path);
   _lifecycleErrorReturns();
 
   kTrace("[lifecycle] complete: frames=%u heap=%u", pmmFreeFrames(), heapFreeBytes());
}

#endif // kIncludeSelfTests
