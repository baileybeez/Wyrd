#pragma once
#include "args.h"

typedef void (*CharSink)(char);

void kvPrintf(CharSink sink, const char* fmt, va_list args);
