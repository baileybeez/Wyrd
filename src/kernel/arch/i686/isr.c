#include "wyrd.h"
#include "isr.h"
#include "drivers/serial/serial.h"

#define kPageFault 14

static const char* kExceptionMessages[32] = {
    /*  0 */ "Divide by Zero",
    /*  1 */ "Debug",
    /*  2 */ "Non-Maskable Interrupt",
    /*  3 */ "Breakpoint",
    /*  4 */ "Overflow",
    /*  5 */ "Out of Bounds",
    /*  6 */ "Invalid Opcode",
    /*  7 */ "No Coprocessor", 
    /*  8 */ "Double Fault", 
    /*  9 */ "Coprocessor Segment Overrun",
    /* 10 */ "Invalid TSS",
    /* 11 */ "Segment not present",
    /* 12 */ "Stack Fault",
    /* 13 */ "General Protection Fault",
    /* 14 */ "Page Fault",
    /* 15 */ "Reserved",
    /* 16 */ "Coprocessor Fault",
    /* 17 */ "Alignment Fault",
    /* 18 */ "Machine Check", 
    /* 19 */ "SIMD Floating Point Exception",   
    /* 20 */ "Virtualization Exception",   
    /* 21 */ "Control Protection Exception",   
    /* 22 */ "Reserved",   /* 23 */ "Reserved",   /* 24 */ "Reserved",   /* 25 */ "Reserved",   
    /* 26 */ "Reserved",   /* 27 */ "Reserved",   /* 28 */ "Reserved",   /* 29 */ "Reserved",   
    /* 30 */ "Reserved",   /* 31 */ "Reserved"
};

static IsrHandler _isrHandlers[32] = { nil };

static void _dumpRegisters(Registers* regs, const char* name)
{
   u32 cr2 = 0;
   if (regs->intNo == kPageFault) {
      __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
   }

   serialPrintf("*** EXCEPTION 0x%02x (%s), error=0x%08x\n", regs->intNo, name, regs->errCode);
   serialPrintf("    EAX=%08x  EBX=%08x  ECX=%08x  EDX=%08x\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
   serialPrintf("    ESI=%08x  EDI=%08x  EBP=%08x  ESP=%08x\n", regs->esi, regs->edi, regs->ebp, regs->espDummy);
   serialPrintf("    EIP=%08x  CS=%08x   EFLAGS=%08x\n", regs->eip, regs->cs, regs->eflags);
   if (regs->intNo == kPageFault) {
      serialPrintf("  CR2=%08x\n", cr2);
   }
   serialPrintf("--- HALTED ---\n");

}

void isrHandler(Registers* regs)
{
   IsrHandler fnc = _isrHandlers[regs->intNo];
   if (fnc == nil) {
      const char* name = (regs->intNo < 32) ? kExceptionMessages[regs->intNo] : "Unexpected Interrupt";
      _dumpRegisters(regs, name);
      for(;;) { __asm__ volatile("cli; hlt"); }
   }

   fnc(regs);
}

void isrRegister(u8 vector, IsrHandler handler)
{
   if (vector <32)
      _isrHandlers[vector] = handler;
}
