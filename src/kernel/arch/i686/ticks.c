#include "wyrd.h"
#include "irq.h"
#include "pit.h"
#include "scheduler/scheduler.h"

static volatile u32 _ticks = 0;
static u32 _hz = 0;

static void tickHandler(Registers* regs)
{
   kUnused(regs);
   _ticks++;
   schedule();
}

void ticksInit(u32 hz)
{
   _hz = hz;
   _ticks = 0;
   irqRegister(0, tickHandler);
   pitSetFrequency(hz);
}

u32 ticksGetCount() 
{
   return _ticks;
}

u32 ticksGetHz() 
{
   return _hz;
}
