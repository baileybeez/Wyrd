#include "wyrd.h"
#include "arch/i686/cpu.h"
#include "arch/i686/tss.h"
#include "lib/assert.h"
#include "lib/logger.h"
#include "lib/panic.h"
#include "scheduler.h"
#include "thread.h"

// Scheduler supports a two-tier approach to thread scheduling
//    `schedule()` will prefer to skip _idle whenever the RunQueue isn't empty
//
// There are two distinct types of lists maintained by scheduler:    
//    - Run Queue       :: round-robin, circular linked list (declared here)
//    - Wait Queue(s)   :: null terminated FIFO (maintained by caller, ie. keyboard driver)
// 
// ** all queues use `thread->next`, so any thread should only ever be on ONE list at a time
// ** every enqueue asserts that fact on entry.
// ** `schedulerBlockCurrent()` expects interrupts to already be OFF, and callers must
//    retest their wake conditionin a loop - the check and block must be atomic
//    against the waking IRQ
// ** a blocked thread resumes with IF still clear — switchContext saves and
//    restores EFLAGS per context, which is what keeps the recheck safe.

extern void switchContext(u32* oldEspSlot, u32 nextEsp);

// [Run Queue]
static Thread* _current;
static Thread* _tail;
static Thread* _idle;

static void _enqueue(Thread* t)
{
   kAssert(t->next == nil);

   t->state = kThreadState_Ready;
   if (_tail == nil) {
      _tail = t;
      t->next = t;
   } else {
      t->next = _tail->next;
      _tail->next = t;
      _tail = t;
   }
}

static Thread* _dequeue()
{
   if (_tail == nil)
      return nil;

   Thread *head = _tail->next;
   if (head == _tail) {
      _tail = nil;
   } else {
      _tail->next = head->next;
   }
   head->next = nil;
   return head;
}

// attempt to dequeue, skipping the Idle thread
static Thread* _dequeueWithoutIdle()
{
   kAssert(_current != nil);
   kAssert(_idle != nil);

   Thread* t = _dequeue();
   if (t != _idle)
      return t;

   Thread* real = _dequeue();
   _enqueue(_idle);
   return real;
}

// attempt to dequeue, skipping the Idle thread
// if that fails (its only thread), dequeue the idle thread
static Thread* _dequeueSafe()
{
   Thread* t = _dequeueWithoutIdle();
   if (t == nil)
      t = _dequeue();

   return t;
}

static void _waitEnqueue(WaitQueue* queue, Thread* t)
{
   kAssert(t->next == nil);
   
   if (queue->head == nil)
      queue->head = t;
   else 
      queue->tail->next = t;

   queue->tail = t;
}

static Thread* _waitDequeue(WaitQueue* queue)
{
   Thread* t = queue->head;
   if (t == nil)
      return nil;

   queue->head = t->next;
   if (queue->head == nil)
      queue->tail = nil;

   t->next = nil;
   return t;
}

static volatile u32 _idleIterations = 0;
static void _schedulerIdle()
{
   for (;;) {
      _idleIterations++;
      __asm__ volatile("sti; hlt");    // enable interrupts, halt until one fires
   }
}

// Makes `next` the running current and switches into it, saving the
// outgoing context into `prev`. Deliberately does NOT touch prev's
// state or queue membership — the caller owns prev's lifecycle
//    - schedule()               re-enqueues it Ready
//    - schedulerExit()          marks it Terminated
//    - schedulerBlockCurrent()  marks it blocked, adds to a waitQueue
static inline void _swapContext(Thread* prev, Thread* next)
{
   _current = next;
   next->state = kThreadState_Running;
   tssSetKernelStack(next->stackBase + next->stackSize);

   if (prev->space != next->space)
      addressSpaceLoad(next->space);

   switchContext(&prev->savedEsp, next->savedEsp);
}

void schedulerSwitchAddressSpace(AddressSpace* space)
{
   u32 flags = irqSave();
   _current->space = space;
   addressSpaceLoad(space);
   irqRestore(flags);
}

void schedule()
{
   u32 flags = irqSave();

   Thread* prev = _current;
   Thread* next = _dequeueWithoutIdle();
   if (next != nil) {
      _enqueue(prev);
      _swapContext(prev, next);
   }

   irqRestore(flags);
}

void schedulerYield() { schedule(); }

void schedulerInit()
{
   _current = threadBootstrap();
   _tail    = nil;
   _idle    = threadCreate(_schedulerIdle);
}

u32 schedulerIdleCount()
{
   return _idleIterations;
}

void schedulerEnqueue(Thread* t)
{
   u32 flags = irqSave();
   _enqueue(t);
   irqRestore(flags);
}

Thread* schedulerCurrent()
{
   return _current;
}

void schedulerBlockCurrent(WaitQueue* queue)
{
   kAssert((readEflags() & kEflags_IF) == 0);
   kAssert(_current != nil);
   kAssert(_current != _idle);

   Thread* prev = _current;
   prev->state = kThreadState_Blocked;
   _waitEnqueue(queue, prev);

   Thread* next = _dequeueSafe();
   if (next == nil)
      kernelPanic("scheduler: blocked with empty run queue");

   _swapContext(prev, next);
}

void schedulerWakeOne(WaitQueue* queue)
{
   u32 flags = irqSave();

   Thread* t = _waitDequeue(queue);
   if (t != nil)
      _enqueue(t);

   irqRestore(flags);
}

void schedulerWakeAll(WaitQueue* queue)
{
   u32 flags = irqSave();
   for (Thread* t = _waitDequeue(queue); t != nil; t = _waitDequeue(queue))
      _enqueue(t);
   
   irqRestore(flags);
}

kNoReturn void schedulerExitThread(i32 code)
{
   kTrace("Thread exiting with code %i", code);
   interruptsDisable();

   kUnused(code);

   Thread* dead = schedulerCurrent();
   dead->state = kThreadState_Terminated;

   Thread* next = _dequeueSafe();
   if (next == nil)
      kernelPanic("scheduler: no runnable thread");
   
   _swapContext(dead, next);
   kernelPanic("scheduler: returned from final switch");
}

AddressSpace*  schedulerCurrentSpace()
{
   return _current->space;
}
