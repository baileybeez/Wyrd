#include "wyrd.h"
#include "arch/i686/cpu.h"
#include "thread.h"
#include "scheduler.h"
#include "mm/heap.h"
#include "lib/logger.h"
#include "lib/mem.h"
#include "lib/panic.h"

#define kThreadStackSize 16384
#define kInitialEflags   0x202
#define kUserBootEflags  0x002

extern void switchContext(u32* oldEspSlot, u32 nextEsp);
extern void enterUserMode(u32 entry, u32 userStack);

extern u8 stack_bottom[];
extern u8 stack_top[];

static u32 _nextThreadId = 0;
static inline u32 getThreadId() { return _nextThreadId++; }

static void _threadExit()
{
   kernelPanic("thread returned from its entry function!");
}

static Thread* _threadAlloc()
{
   Thread* t = kmalloc(sizeof(Thread));
   if (t == nil)
      kernelPanic("_threadAlloc: kmalloc failed");

   memset(t, 0x00, sizeof(Thread));
   t->id        = getThreadId();
   t->state     = kThreadState_Ready;
   t->space     = pagingBootSpace();
   t->next      = nil;  

   return t;
}

Thread* threadBootstrap()
{
   Thread* t = _threadAlloc();
   
   t->state     = kThreadState_Running;
   t->stackBase = (u32)stack_bottom;
   t->stackSize = (u32)(stack_top - stack_bottom);
   
   return t;
}

Thread* threadCreate(ThreadEntry entry)
{
   Thread* t = _threadAlloc();

   u8* stack = kmalloc(kThreadStackSize);
   if (stack == nil) {
      kfree(t);
      return nil;
   }

   t->stackBase = (u32)stack;
   t->stackSize = kThreadStackSize;
   
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

Thread* threadCreateUser(u32 entry, u32 userStackTop, AddressSpace* space)
{
   Thread* t = _threadAlloc();

   u8* stack = kmalloc(kThreadStackSize);
   if (stack == nil) {
      kfree(t);
      return nil;
   }
   
   t->space     = space;
   t->stackBase = (u32)stack;
   t->stackSize = kThreadStackSize;
   
   // first switchContext() 'ret's into enterUserMode, which finds 
   // 'entry' and 'userStackTop' as its cdecl args and iret's into 
   // Ring3 (consumed once)
   // IF is left  clear so the brief ring-0 window inside 
   // enterUserMode (segregs already user data) can't take an IRQ;
   // the ring-3 iret sets IF
   //
   // layout, high -> low:
   // [top -  4]  userStackTop    enterUserMode arg1  ([esp+8] after the ret)
   // [top -  8]  entry           enterUserMode arg0  ([esp+4] after the ret)
   // [top - 12]  _threadExit     enterUserMode's return slot — never taken (it iret's)
   // [top - 16]  enterUserMode   switchContext's RET pops this into EIP
   // [top - 20]  eflags          popfd
   // [top - 24]  0  (ebp)
   // [top - 28]  0  (ebx)
   // [top - 32]  0  (esi)
   // [top - 36]  0  (edi)   <- savedEsp
   u32* sp = (u32*)(stack + kThreadStackSize);
   *(--sp) = userStackTop;
   *(--sp) = entry;
   *(--sp) = (u32)_threadExit;
   *(--sp) = (u32)enterUserMode;
   *(--sp) = kUserBootEflags;
   *(--sp) = (u32)0;          // ebp
   *(--sp) = (u32)0;          // ebx
   *(--sp) = (u32)0;          // esi
   *(--sp) = (u32)0;          // edi
   t->savedEsp = (u32)sp;
   schedulerEnqueue(t);

   return t;
}
