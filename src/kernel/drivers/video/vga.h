#pragma once

#include "bee.h"

#define kVideoMem    ((u16*)0xB8000)

#define kVideoWidth  80
#define kVideoHeight 25

#define kColor_Black          0
#define kColor_LightGray      7

typedef struct {
   u8    row;
   u8    col;
   u16*  mem;
   u16   color;
} VGA;

void vgaInit();
void print(const char *s);
