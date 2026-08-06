#pragma once
#include "wyrd.h"

#define kSysErr_Fault   -1
#define kSysErr_TooLong -2
#define kSysErr_NoSys   -3

#define kSys_Read       0
#define kSys_Write      1
#define kSys_Exit       2

#define kSyscallCount   3

typedef i32 (*fncSyscall)(u32 a0, u32 a1, u32 a2);

i32  syscallDispatch(u32 num, u32 a0, u32 a1, u32 a2);
