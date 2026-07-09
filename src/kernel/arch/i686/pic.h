#pragma once
#include "bee.h"

#define kPicPrimary_Cmd    0x20
#define kPicPrimary_Data   0x21
#define kPicSecondary_Cmd  0xA0
#define kPicSecondary_Data 0xA1

#define kPicEOI            0x20
#define kIrqBase           0x20

void picRemap(u8 primaryOffset, u8 secondaryOffset);
void picSendEoi(u8 irq);
void picSetMask(u8 irq);
void picClearMask(u8 irq);
void picMaskAll();
