#pragma once
#include "wyrd.h"

#define kHalt()      for(;;) { __asm__ volatile("cli; hlt;"); }

static inline void interruptsDisable() { __asm__ volatile("cli" ::: "memory"); }
static inline void interruptsEnable()  { __asm__ volatile("sti" ::: "memory"); }

static inline u32 irqSave()
{
   u32 flags;
   __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) :: "memory");
   return flags;
}

static inline void irqRestore(u32 flags)
{
   __asm__ volatile("push %0; popf" :: "r"(flags) : "memory", "cc");
}
