#pragma once
#include "wyrd.h"
#include "thread.h"

void     schedulerInit();
void     schedulerEnqueue(Thread* t);
Thread*  schedulerCurrent();
void     schedulerYield();
void     schedulerExitThread(i32 code);

void schedule();
