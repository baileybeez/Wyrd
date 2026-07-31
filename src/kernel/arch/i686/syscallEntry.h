#pragma once
#include "wyrd.h"

void syscallInit();

static inline i32 syscall3(u32 num, u32 a0, u32 a1, u32 a2)
{
   i32 ret;
   __asm__ volatile("int $0x80"
      : "=a"(ret)
      : "a"(num), "b"(a0), "c"(a1), "d"(a2)
      : "memory");
   return ret;
}
