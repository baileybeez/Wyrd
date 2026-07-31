#include "wyrd.h"
#include "arch/i686/cpu.h"
#include "arch/i686/tss.h"
#include "lib/logger.h"
#include "lib/panic.h"
#include "scheduler.h"
#include "thread.h"

// Scheduler is currently a round-robin
//
// a circular linked list maintains the queue

extern void switchContext(u32* oldEspSlot, u32 nextEsp);

static Thread* _current;
static Thread* _tail;

static void _enqueue(Thread* t)
{
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

static void _schedulerIdle()
{
   for (;;) {
      __asm__ volatile("sti; hlt");    // enable interrupts, halt until one fires
   }
}

// Makes `next` the running current and switches into it, saving the
// outgoing context into `prev`. Deliberately does NOT touch prev's
// state or queue membership — the caller owns prev's lifecycle
// (schedule() re-enqueues it Ready; schedulerExit() marks it Terminated).
static inline void _swapContext(Thread* prev, Thread* next)
{
   _current = next;
   next->state = kThreadState_Running;
   tssSetKernelStack(next->stackBase + next->stackSize);
   switchContext(&prev->savedEsp, next->savedEsp);
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
   _tail = nil;
   threadCreate(_schedulerIdle);
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

kNoReturn void schedulerExitThread(i32 code)
{
   kTrace("Thread exiting with code %u", code);
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
