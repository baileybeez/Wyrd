#pragma once
#include "wyrd.h"

#define kATA_WaitTimeout 100000

bool ataReadSectors(u32 lba, u32 count, void* dest);

#ifdef kATA_SelfTest
void ataSelfTest();
#endif
