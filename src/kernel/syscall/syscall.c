#include "wyrd.h"
#include "syscall.h"
#include "drivers/serial/serial.h"
#include "mm/usercopy.h"
#include "scheduler/scheduler.h"

#define kSyscallBufferSize 256
#define kSyscallWriteMax   0x7fff 

static i32 _sysWrite(u32 ptr, u32 len, u32 unused)
{
   kUnused(unused);
   if (len > kSyscallWriteMax)
      return kSysErr_Fault;

   // upfront check is not redundant with userCopyIn: it makes a bad
   // buffer fail before any bytes reach serial, not halfway through
   if (!userBufferIsValid(ptr, len, false))
      return kSysErr_Fault;

   u32 remaining = len;
   u32 offset = 0;
   while (remaining > 0) {
      char buffer[kSyscallBufferSize];
      u32  chunk = min(remaining, kSyscallBufferSize);
      
      // TOCTOU: validated then dereferenced. Unreachable while all threads
      // share one directory; closing it needs fault fixup tables (deferred #14)
      if (!userCopyIn(buffer, ptr + offset, chunk))
         return kSysErr_Fault;

      serialWrite(buffer, chunk);
      offset += chunk;
      remaining -= chunk;
   }
   return (i32)len;
}

static i32 _sysExit(u32 code, u32 a1, u32 a2)
{
   kUnused(a1);
   kUnused(a2);
   schedulerExitThread((i32)code);
}

static const fncSyscall _sysCalls[kSyscallCount] = 
{
   [kSys_Read]    = nil, 
   [kSys_Write]   = _sysWrite,
   [kSys_Exit]    = _sysExit
};

i32 syscallDispatch(u32 callId, u32 a0, u32 a1, u32 a2)
{
   if (callId >= kSyscallCount || !_sysCalls[callId])
      return kSysErr_NoSys;

   return _sysCalls[callId](a0, a1, a2);
}
