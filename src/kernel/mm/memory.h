#pragma once
#include "wyrd.h"
#include "arch/i686/paging.h"

#define kKernelVirtualBase 0xC0000000

#define kUserStackPages    8
#define kUserStackTop      kKernelVirtualBase
#define kUserStackSize     (kUserStackPages * kPageSize)
#define kUserStackBase     (kUserStackTop - (kUserStackSize))
#define kUserGuardPages    4
#define kUserGuardSize     (kUserGuardPages * kPageSize)
#define kUserGuardBase     ((kUserStackBase) - (kUserGuardSize))
#define kUserHeapLimit     kUserGuardBase

static inline u32 physToVirtual(u32 physAddr)
{
   return physAddr + kKernelVirtualBase;
}

static inline u32 virtualToPhys(u32 virtAddr)
{
   return virtAddr - kKernelVirtualBase;
}
