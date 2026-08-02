#pragma once
#include "wyrd.h"
#include "arch/i686/paging.h"

typedef enum {
   kThreadState_Ready,
   kThreadState_Running,
   kThreadState_Blocked,
   kThreadState_Terminated
} ThreadState;

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
   struct Thread* next;
} Thread;

Thread*  threadBootstrap();
Thread*  threadCreate(ThreadEntry entry);
Thread*  threadCreateUser(u32 entry, u32 userStackTop, AddressSpace* space);
