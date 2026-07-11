#pragma once
#include "bee.h"

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

#define kTrace(...) logMessage(kLogTrace, __VA_ARGS__)
#define kInfo(...)  logMessage(kLogInfo,  __VA_ARGS__)
#define kWarn(...)  logMessage(kLogWarn,  __VA_ARGS__)
#define kError(...) logMessage(kLogError, __VA_ARGS__)
#define kPanic(...) logMessage(kLogPanic, __VA_ARGS__)
