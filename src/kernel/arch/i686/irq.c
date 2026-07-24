#include "wyrd.h"
#include "irq.h"
#include "pic.h"
#include "gdt.h"
#include "idt.h"

#define kIrqCount     16
#define kIdtFlagsIrq  0x8E

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

static IrqHandler _handlers[kIrqCount] = { nil };

void irqInit(void)
{
   picMaskAll();

   idtSetGate(kIrqBase +  0, (u32)irq0,  kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase +  1, (u32)irq1,  kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase +  2, (u32)irq2,  kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase +  3, (u32)irq3,  kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase +  4, (u32)irq4,  kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase +  5, (u32)irq5,  kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase +  6, (u32)irq6,  kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase +  7, (u32)irq7,  kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase +  8, (u32)irq8,  kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase +  9, (u32)irq9,  kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase + 10, (u32)irq10, kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase + 11, (u32)irq11, kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase + 12, (u32)irq12, kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase + 13, (u32)irq13, kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase + 14, (u32)irq14, kGDT_KernelCodeSelector, kIdtFlagsIrq);
   idtSetGate(kIrqBase + 15, (u32)irq15, kGDT_KernelCodeSelector, kIdtFlagsIrq);
}

void irqRegister(u8 irq, IrqHandler handler)
{
   if (irq < kIrqCount)
   {
      _handlers[irq] = handler;
   }
}

void irqUnregister(u8 irq)
{
   if (irq < kIrqCount)
   {
      _handlers[irq] = nil;
   }
}

void irqDispatch(Registers* regs)
{
   u32 irq = regs->intNo - kIrqBase;

   picSendEoi((u8)irq);
   if (irq < kIrqCount && _handlers[irq] != nil)
      _handlers[irq](regs);
}
