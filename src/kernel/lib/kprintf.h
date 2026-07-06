#pragma once

#include <stdarg.h>

typedef void (*CharSink)(char);

void kvPrintf(CharSink sink, const char* fmt, va_list args);
