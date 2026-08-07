#include "wyrd.h"
#include "sys.h"
#include "syscall.h"
#include "drivers/serial/serial.h"
#include "mm/usercopy.h"
#include "scheduler/scheduler.h"

#define kSyscallBufferSize 256
#define kSyscallWriteMax   0x7fff 

static i32 _sysWrite(u32 fd, u32 ptr, u32 len)
{
   kUnused(fd);
   if (len > kSyscallWriteMax)
      return kSysErr_TooLong;

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
   i32 ret = (i32)code;
   if (ret <= kExitReservedFloor)
      ret = kExitReservedFloor + 1;
      
   kUnused(a1);
   kUnused(a2);
   schedulerExitThread(ret);
}

static const fncSyscall _sysCalls[kSyscall_Count] = 
{
   [kSyscall_Invalid]   = nil,
   [kSyscall_Read]      = nil, 
   [kSyscall_Write]     = _sysWrite,
   [kSyscall_Exit]      = _sysExit
};

i32 syscallDispatch(u32 callId, u32 a0, u32 a1, u32 a2)
{
   if (callId >= kSyscall_Count || !_sysCalls[callId])
      return kSysErr_NoSys;

   return _sysCalls[callId](a0, a1, a2);
}
