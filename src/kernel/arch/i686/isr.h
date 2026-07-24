#pragma once
#include "wyrd.h"

typedef struct
{
   u32 ds;
   u32 edi, esi, ebp, espDummy, ebx, edx, ecx, eax;
   u32 intNo, errCode;
   u32 eip, cs, eflags;
}__attribute__((packed)) Registers;

typedef void (*IsrHandler)(Registers*);

void isrHandler(Registers* regs);
void isrRegister(u8 vector, IsrHandler handler);
