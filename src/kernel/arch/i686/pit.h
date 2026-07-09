#pragma once
#include "bee.h"

#define kPitBaseFrequency  1193182
#define kPitChannel0Data   0x40
#define kPitCommand        0x43

void pitSetFrequency(u32 hz);
