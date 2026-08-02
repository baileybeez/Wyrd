#pragma once
#include "wyrd.h"
#include "thread.h"

void           schedulerInit();
void           schedulerEnqueue(Thread* t);
Thread*        schedulerCurrent();
void           schedulerYield();
kNoReturn void schedulerExitThread(i32 code);
void           schedulerSwitchAddressSpace(AddressSpace* space);
AddressSpace*  schedulerCurrentSpace();

void schedule();
