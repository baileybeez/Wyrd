#include "wyrd.h"
#include "args.h"
#include "logger.h"
#include "../drivers/video/vga.h"
#include "../drivers/serial/serial.h"

static LogLevel _minVgaLevel     = kLogInfo;
static LogLevel _minSerialLevel  = kLogTrace;

static const char* _logLevelPrefix(LogLevel level)
{
   switch (level)
   {
      case kLogTrace:   return "[TRACE] ";
      case kLogInfo:    return "[INFO ] ";
      case kLogWarn:    return "[WARN ] ";
      case kLogError:   return "[ERROR] ";
      case kLogPanic:   return "[PANIC] ";
   }
   return "[?????] ";
}

static void _applyVgaColor(LogLevel level)
{
   switch (level)
   {
      case kLogTrace:   vgaSetColor(kColor_DarkGray,  kColor_Black); break;
      case kLogInfo:    vgaSetColor(kColor_LightGray, kColor_Black); break;
      case kLogWarn:    vgaSetColor(kColor_Yellow,    kColor_Black); break;
      case kLogError:   vgaSetColor(kColor_LightRed,  kColor_Black); break;
      case kLogPanic:   vgaSetColor(kColor_White,     kColor_Red);   break;
   }
}

void logInit(LogLevel minVgaLevel, LogLevel minSerialLevel)
{
   _minVgaLevel    = minVgaLevel;
   _minSerialLevel = minSerialLevel;
}

void logSetVgaLevel(LogLevel level)    { _minVgaLevel    = level; }
void logSetSerialLevel(LogLevel level) { _minSerialLevel = level; }

void logMessageV(LogLevel level, const char* fmt, va_list args)
{
   if (level >= _minSerialLevel) {
      serialPrintf("%s", _logLevelPrefix(level));
      serialVprintf(fmt, args);
      serialPrintf("\n");
   }

   if (level >= _minVgaLevel) {
      _applyVgaColor(level);
      if (level >= kLogWarn)
         printf("%s", _logLevelPrefix(level));

      vprintf(fmt, args);
      print("\n");
   }
}

void logMessage(LogLevel level, const char* fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   logMessageV(level, fmt, args);
   va_end(args);
}
