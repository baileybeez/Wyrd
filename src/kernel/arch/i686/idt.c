#include "bee.h"
#include "idt.h"
#include "gdt.h"

typedef struct 
{
   u16 baseLow;
   u16 selector;
   u8 zero;
   u8 flags;
   u16 baseHigh;
}__attribute__((packed)) IdtEntry;

typedef struct 
{
   u16 limit;
   u32 base;
}__attribute__((packed)) IdtDescriptor;

static IdtEntry      _idtEntries[kIDTEntryCount];
static IdtDescriptor _idtDescriptor;

extern void idtFlush(u32 idtAddr);

extern void isr0();  extern void isr1();  extern void isr2();  extern void isr3(); 
extern void isr4();  extern void isr5();  extern void isr6();  extern void isr7(); 
extern void isr8();  extern void isr9();  extern void isr10(); extern void isr11(); 
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15(); 
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19(); 
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23(); 
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27(); 
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31(); 
extern void isrDefault();

void idtSetGate(u8 num, u32 base, u16 selector, u8 flags)
{
   _idtEntries[num].baseLow  = base & 0xFFFF;
   _idtEntries[num].baseHigh = (base >> 16) & 0xFFFF;
   _idtEntries[num].selector = selector;
   _idtEntries[num].zero     = 0;
   _idtEntries[num].flags    = flags;
}

void idtInit()
{
   _idtDescriptor.limit = (sizeof(IdtEntry) * kIDTEntryCount) - 1;
   _idtDescriptor.base  = (u32)&_idtEntries;

   for (i32 i = 0; i < kIDTEntryCount; i++) {
      idtSetGate((u8)i, (u32)isrDefault, kGDT_KernelCodeSelector, 0x8E);
   }

   idtSetGate(0,  (u32)isr0,  kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(1,  (u32)isr1,  kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(2,  (u32)isr2,  kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(3,  (u32)isr3,  kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(4,  (u32)isr4,  kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(5,  (u32)isr5,  kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(6,  (u32)isr6,  kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(7,  (u32)isr7,  kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(8,  (u32)isr8,  kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(9,  (u32)isr9,  kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(10, (u32)isr10, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(11, (u32)isr11, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(12, (u32)isr12, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(13, (u32)isr13, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(14, (u32)isr14, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(15, (u32)isr15, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(16, (u32)isr16, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(17, (u32)isr17, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(18, (u32)isr18, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(19, (u32)isr19, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(20, (u32)isr20, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(21, (u32)isr21, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(22, (u32)isr22, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(23, (u32)isr23, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(24, (u32)isr24, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(25, (u32)isr25, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(26, (u32)isr26, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(27, (u32)isr27, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(28, (u32)isr28, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(29, (u32)isr29, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(30, (u32)isr30, kGDT_KernelCodeSelector, 0x8E);
   idtSetGate(31, (u32)isr31, kGDT_KernelCodeSelector, 0x8E);

   idtFlush((u32)&_idtDescriptor);
}
