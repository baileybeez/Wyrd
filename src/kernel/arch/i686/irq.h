#pragma once
#include "wyrd.h"
#include "isr.h"

typedef void (*IrqHandler)(Registers* regs);

void irqInit();
void irqRegister(u8 irq, IrqHandler handler);
void irqUnregister(u8 irq);

void irqDispatch(Registers* regs);
