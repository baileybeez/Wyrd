#pragma once
#include "wyrd.h"

typedef enum {
   kThreadState_Ready,
   kThreadState_Running,
   kThreadState_Blocked,
   kThreadState_Terminated
} ThreadState;

typedef void (*ThreadEntry)(void);

typedef struct Thread {
   u32            id;
   ThreadState    state;
   u32            savedEsp;
   u32            stackBase;
   u32            stackSize;
   struct Thread* next;
} Thread;

Thread*  threadBootstrap();
Thread*  threadCreate(ThreadEntry entry);
Thread*  threadCreateUser(u32 entry, u32 userStackTop);
