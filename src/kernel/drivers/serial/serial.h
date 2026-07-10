#pragma once
#include <stdarg.h>

bool serialInit();
void serialWriteString(const char* s);
void serialPrintf(const char* fmt, ...);
void serialVprintf(const char* fmt, va_list args);
