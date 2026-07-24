#include "wyrd.h"
#include "thread.h"
#include "mm/heap.h"
#include "lib/logger.h"

#define kThreadStackSize 16384

extern void switchContext(u32* oldEspSlot, u32 nextEsp);
extern u8 stack_bottom[];
extern u8 stack_top[];

static u32 _nextThreadId = 0;
static inline u32 getThreadId() { return _nextThreadId++; }

static void _threadExit()
{
   kPanic("thread returned from its entry function!");
   kHalt();
}

Thread* threadBootstrap()
{
   Thread* t = kmalloc(sizeof(Thread));
   if (t == nil)
      kPanic("threadBootstrap: kmalloc failed");

   t->id        = getThreadId();
   t->state     = kThreadState_Running;
   t->savedEsp  = 0;
   t->stackBase = (u32)stack_bottom;
   t->stackSize = (u32)(stack_top - stack_bottom);
   t->next      = nil;
   return t;
}

Thread* threadCreate(ThreadEntry entry)
{
   Thread* t = kmalloc(sizeof(Thread));
   if (t == nil)
      kPanic("threadBootstrap: kmalloc failed");

   u8* stack = kmalloc(kThreadStackSize);
   if (stack == nil) {
      kfree(t);
      return nil;
   }

   t->id        = getThreadId();
   t->state     = kThreadState_Ready;
   t->stackBase = (u32)stack;
   t->stackSize = kThreadStackSize;
   t->next      = nil;

   // hand-craft initial stack so that the first switchContext() into this
   // thread pops zeroed callee-saved registers and 'ret's into 'entry'
   //
   // layout, high -> low:
   //    [top - 4]   _threadExit    handler if entry() returns
   //    [top - 8]   entry          switchContext's RET pops this into EIP
   //    [top - 12]  0              ebp
   //    [top - 16]  0              ebx
   //    [top - 20]  0              esi
   //    [top - 24]  0              edi   <- savedEso should point here
   u32* sp = (u32*)(stack + kThreadStackSize);
   *(--sp) = (u32)_threadExit;
   *(--sp) = (u32)entry;
   *(--sp) = (u32)0;
   *(--sp) = (u32)0;
   *(--sp) = (u32)0;
   *(--sp) = (u32)0;
   t->savedEsp = (u32)sp;

   return t;
}

void threadYield(Thread* prev, Thread* next)
{
   if (prev == next)
      return;

   prev->state = kThreadState_Ready;
   next->state = kThreadState_Running;

   switchContext(&prev->savedEsp, next->savedEsp);
}
