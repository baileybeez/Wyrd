#include "wyrd.h"
#include "thread.h"
#include "scheduler.h"
#include "mm/heap.h"
#include "lib/logger.h"

#define kThreadStackSize 16384
#define kInitialEflags   0x202

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
      kPanic("threadCreate: kmalloc failed");

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
   //    [top - 12]  0x202          popfd pops this into EFLAGS (IF=1)
   //    [top - 16]  0              ebp
   //    [top - 20]  0              ebx
   //    [top - 24]  0              esi
   //    [top - 28]  0              edi   <- savedEsp should point here
   u32* sp = (u32*)(stack + kThreadStackSize);
   *(--sp) = (u32)_threadExit;
   *(--sp) = (u32)entry;
   *(--sp) = kInitialEflags;  // EFLAGS: IF=1, reserved bit 1 set
   *(--sp) = (u32)0;          // ebp
   *(--sp) = (u32)0;          // ebx
   *(--sp) = (u32)0;          // esi
   *(--sp) = (u32)0;          // edi
   t->savedEsp = (u32)sp;
   schedulerEnqueue(t);

   return t;
}
