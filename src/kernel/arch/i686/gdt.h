#pragma once

#include "wyrd.h"

#define kGDTEntryCount 5

#define kGDT_KernelCodeSelector  0x08
#define kGDT_KernelDataSelector  0x10
#define kGDT_UserCodeSelector    0x1B
#define kGDT_UserDataSelector    0x23

void gdtInit();
