#pragma once

#include "wyrd.h"

#define kGDTEntryCount 6

#define kGDT_KernelCodeSelector  0x08
#define kGDT_KernelDataSelector  0x10
#define kGDT_UserCodeSelector    0x1B
#define kGDT_UserDataSelector    0x23
#define kGDT_TssSelector         0x28

void gdtInit();
void gdtInstallTss(u32 base, u32 limit);
