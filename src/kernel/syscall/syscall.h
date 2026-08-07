#pragma once
#include "wyrd.h"
#include "sys.h"

typedef i32 (*fncSyscall)(u32 a0, u32 a1, u32 a2);

i32  syscallDispatch(u32 num, u32 a0, u32 a1, u32 a2);
