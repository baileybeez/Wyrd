#pragma once
#include "wyrd.h"
#include "thread.h"

void           schedulerInit();
void           schedulerEnqueue(Thread* t);
Thread*        schedulerCurrent();
void           schedulerYield();
void           schedulerExitThread(i32 code) kNoReturn;
void           schedulerSwitchAddressSpace(AddressSpace* space);
AddressSpace*  schedulerCurrentSpace();
u32            schedulerIdleCount();

// NOTE: caller must hold interrupts OFF, and must re-test its wake
//       condition in a loop after this returns
void schedulerBlockCurrent(WaitQueue* queue);

// NOTE: Safe to call from IRQ context. Moves wait-ers to the run
//       queue; the switch happens on the next `schedule()`
void schedulerWakeOne(WaitQueue* queue);
void schedulerWakeAll(WaitQueue* queue);

void schedule();
