#pragma once
#include "wyrd.h"
#include "mm/memory.h"
#include <stdarg.h>

#define kVideoPhysMem      0xB8000
#define kVideoMem          ((u16*)physToVirtual(kVideoPhysMem))

#define kVideoWidth  80
#define kVideoHeight 25

#define kColor_Black        0
#define kColor_Blue         1
#define kColor_Green        2
#define kColor_Cyan         3
#define kColor_Red          4
#define kColor_Magenta      5
#define kColor_Brown        6
#define kColor_LightGray    7
#define kColor_DarkGray     8
#define kColor_LightBlue    9
#define kColor_LightGreen   10
#define kColor_LightCyan    11
#define kColor_LightRed     12
#define kColor_LightMagenta 13
#define kColor_Yellow       14
#define kColor_White        15

typedef struct {
   u8    row;
   u8    col;
   u16*  mem;
   u16   color;
} VGA;

void vgaInit();
void vgaSetColor(u8 fg, u8 bg);
void putChar(char cb);
void print(const char * s);
void printf(const char* fmt, ...);
void vprintf(const char* fmt, va_list args);
