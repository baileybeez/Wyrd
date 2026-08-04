#include "wyrd.h"
#include "arch/i686/cpu.h"
#include "arch/i686/tss.h"
#include "lib/assert.h"
#include "lib/logger.h"
#include "lib/panic.h"
#include "scheduler.h"
#include "thread.h"

// Scheduler is currently a round-robin: a circular linked list maintains the queue
// 
//    - Run Queue       :: circular linked list
//    - Wait Queue(s)   :: null terminated FIFO
// 
// ** all queues use `thread->next`, so a thread is on at MOST one list at a time
// ** every enqueue asserts that on entry.

extern void switchContext(u32* oldEspSlot, u32 nextEsp);

// Run Queue
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

static u32 _idleIterations = 0;
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
   Thread* next = _dequeue();
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
   kAssert(_current != _idle);

   Thread* prev = _current;
   prev->state = kThreadState_Blocked;
   _waitEnqueue(queue, prev);

   Thread* next = _dequeue();
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

   Thread* next = _dequeue();
   if (next == nil)
      kernelPanic("scheduler: no runnable thread");
   
   _swapContext(dead, next);
   kernelPanic("scheduler: returned from final switch");
}

AddressSpace*  schedulerCurrentSpace()
{
   return _current->space;
}
