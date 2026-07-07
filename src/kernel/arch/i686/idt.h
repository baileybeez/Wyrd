#pragma once

#include "bee.h"

#define kIDTEntryCount 256

void idtInit();
void idtSetGate(u8 num, u32 base, u16 selector, u8 flags);
