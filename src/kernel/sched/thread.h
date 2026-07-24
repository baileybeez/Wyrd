#pragma once
#include "wyrd.h"

typedef enum {
   kThreadState_Ready,
   kThreadState_Running,
   kThreadState_Blocked
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
void     threadYield(Thread* prev, Thread* next);
