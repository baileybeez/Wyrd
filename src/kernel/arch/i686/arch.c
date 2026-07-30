#include "wyrd.h"
#include "arch.h"
#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "pic.h"
#include "syscallEntry.h"
#include "ticks.h"
#include "tss.h"
#include "lib/logger.h"

void archInitEarly()
{
   gdtInit();
   kTrace("+ GDT initialized.");
   tssInit();
   kTrace("+ TSS initialized.");
   idtInit();
   kTrace("+ IDT initialized.");
   picRemap(kIrqBase, kIrqBase + 8);
   irqInit();
}

void archInitLate()
{
   ticksInit(100);
   picClearMask(0);
}
