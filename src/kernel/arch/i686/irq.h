#pragma once
#include "bee.h"
#include "isr.h"

typedef void (*IrqHandler)(struct Registers* regs);

void irqInit();
void irqRegister(u8 irq, IrqHandler handler);
void irqUnregister(u8 irq);

void irqDispatch(struct Registers* regs);
