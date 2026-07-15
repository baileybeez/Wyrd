#pragma once
#include "bee.h"

#define kKernelVirtualBase 0xC0000000

static inline u32 physToVirtual(u32 physAddr)
{
   return physAddr + kKernelVirtualBase;
}

static inline u32 virtualToPhys(u32 virtAddr)
{
   return virtAddr - kKernelVirtualBase;
}
