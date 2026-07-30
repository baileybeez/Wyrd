#include "wyrd.h"
#include "args.h"
#include "arch/i686/cpu.h"
#include "panic.h"
#include "logger.h"

kNoReturn void kernelPanic(const char* fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   logMessageV(kLogPanic, fmt, args);
   va_end(args);
   kHalt();
}
