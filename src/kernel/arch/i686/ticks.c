#include "wyrd.h"
#include "irq.h"
#include "pit.h"

static volatile u32 g_ticks = 0;

static void tickHandler(Registers* regs)
{
   (void)regs;
   g_ticks++;
}

void ticksInit(u32 hz)
{
   g_ticks = 0;
   irqRegister(0, tickHandler);
   pitSetFrequency(hz);
}

u32 ticksGetCount() 
{
   return g_ticks;
}
