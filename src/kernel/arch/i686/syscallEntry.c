#include "wyrd.h"
#include "syscallEntry.h"
#include "syscall/syscall.h"
#include "isr.h"

static void _syscallTrap(Registers* regs)
{
   regs->eax = syscallDispatch(regs->eax, regs->ebx, regs->ecx, regs->edx);
}

void syscallInit()
{
   isrRegister(0x80, _syscallTrap);
}
