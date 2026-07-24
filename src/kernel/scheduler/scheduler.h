#pragma once
#include "wyrd.h"
#include "thread.h"

void schedulerInit();
void schedulerEnqueue(Thread* t);
void schedule();
