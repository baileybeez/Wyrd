#pragma once
#include "wyrd.h"
#include "arch/i686/paging.h"

#define kKernelVirtualBase 0xC0000000

#define kUserStackPages    1
#define kUserStackTop      kKernelVirtualBase
#define kUserStackBase     (kUserStackTop - (kUserStackPages * kPageSize))

static inline u32 physToVirtual(u32 physAddr)
{
   return physAddr + kKernelVirtualBase;
}

static inline u32 virtualToPhys(u32 virtAddr)
{
   return virtAddr - kKernelVirtualBase;
}
