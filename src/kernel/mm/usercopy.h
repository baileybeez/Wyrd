#pragma once
#include "wyrd.h"

#define kUserString_Fault   -1
#define kUserString_TooLong -2

bool userBufferIsValid(u32 ptr, u32 len, bool needWrite);
bool userCopyIn(void* dest, u32 userSrc, u32 len);
bool userCopyOut(u32 userDest, const void* src, u32 len);
i32 userStringLength(u32 ptr, u32 maxLen);
