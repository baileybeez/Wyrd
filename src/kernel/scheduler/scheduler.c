#include "wyrd.h"
#include "arch/i686/tss.h"
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

static void _idle()
{
   for (;;) {
      __asm__ volatile("sti; hlt");    // enable interrupts, halt until one fires
   }
}

static inline u32 irqSave(void)
{
   u32 flags;
   __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) :: "memory");
   return flags;
}

static inline void irqRestore(u32 flags)
{
   __asm__ volatile("push %0; popf" :: "r"(flags) : "memory", "cc");
}

void schedule()
{
   u32 flags = irqSave();

   Thread* prev = _current;
   Thread* next = _dequeue();
   if (next != nil) {
      _enqueue(prev);
      _current = next;
      next->state = kThreadState_Running;
      tssSetKernelStack(next->stackBase + next->stackSize);
      switchContext(&prev->savedEsp, next->savedEsp);
   }

   irqRestore(flags);
}

void schedulerInit()
{
   _current = threadBootstrap();
   _tail = nil;
   threadCreate(_idle);
}

void schedulerEnqueue(Thread* t)
{
   u32 flags = irqSave();
   _enqueue(t);
   irqRestore(flags);
}

void yield() { schedule(); }
