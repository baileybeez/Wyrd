#pragma once
#include "wyrd.h"
#include "sys.h"
#include "arch/i686/paging.h"

typedef enum {
   kThreadState_Ready,
   kThreadState_Running,
   kThreadState_Blocked,
   kThreadState_Zombie,
   kThreadState_Reaped
} ThreadState;

typedef enum { 
   kWaitErr_OK = 0,
   kWaitErr_NoSuchThread,
   kWaitErr_Self,
   kWaitErr_NotAChild,
} WaitError;

typedef struct {
   struct Thread* head;
   struct Thread* tail;
} WaitQueue;

typedef void (*ThreadEntry)(void);

typedef struct Thread {
   u32            id;
   u32            parentId;
   ThreadState    state;
   u32            savedEsp;
   u32            stackBase;
   u32            stackSize;
   AddressSpace*  space;
   i32            exitCode;
   bool           detached;
   bool           ownsStack;     // flags whether or not the reaper should free `stackBase`
   WaitQueue      waitQueue;     // parents blocked in threadWait on this thread
   bool           claimed;       // a waiting thread has claimed exit code, free-able
   struct Thread* registryNext;  // thread registry linked list
   struct Thread* next;
} Thread;

Thread*   threadBootstrap();
Thread*   threadCreate(ThreadEntry entry);
Thread*   threadCreateUser(u32 entry, u32 userStackTop, AddressSpace* space);
WaitError threadWait(u32 id, i32* outCode);
Thread*   threadFind(u32 id);
void      threadUnregister(Thread* t);
