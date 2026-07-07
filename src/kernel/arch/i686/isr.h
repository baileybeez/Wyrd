#pragma once
#include "bee.h"

struct Registers
{
   u32 ds;
   u32 edi, esi, ebp, espDummy, ebx, edx, ecx, eax;
   u32 intNo, errCode;
   u32 eip, cs, eflags;
}__attribute__((packed));

void isrHandler(struct Registers* regs);
