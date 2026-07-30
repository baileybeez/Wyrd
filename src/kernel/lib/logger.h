#pragma once
#include "wyrd.h"
#include "args.h"

typedef enum {
   kLogTrace,
   kLogInfo,
   kLogWarn,
   kLogError,
   kLogPanic
} LogLevel;

void logInit(LogLevel minVgaLevel, LogLevel minSerialLevel);
void logSetVgaLevel(LogLevel level);
void logSetSerialLevel(LogLevel level);
void logMessage(LogLevel level, const char* fmt, ...);
void logMessageV(LogLevel level, const char* fmt, va_list args);

#define kTrace(...) logMessage(kLogTrace, __VA_ARGS__)
#define kInfo(...)  logMessage(kLogInfo,  __VA_ARGS__)
#define kWarn(...)  logMessage(kLogWarn,  __VA_ARGS__)
#define kError(...) logMessage(kLogError, __VA_ARGS__)
#define kPanic(...) logMessage(kLogPanic, __VA_ARGS__)
