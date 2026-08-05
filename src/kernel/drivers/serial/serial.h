#pragma once
#include "wyrd.h"
#include "args.h"

bool serialInit();
void serialWrite(const char* buf, u32 len);
void serialPrintf(const char* fmt, ...);
void serialVprintf(const char* fmt, va_list args);
