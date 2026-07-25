#include "wyrd.h"
#include "syscall.h"
#include "drivers/serial/serial.h"

typedef i32 (*fncSyscall)(u32 a0, u32 a1, u32 a2);

static i32 _sysWrite(u32 ptr, u32 len, u32 unused)
{
   kUnused(unused);
   serialWrite((const char*)ptr, len);
   return (i32)len;
}

static fncSyscall _sysCalls[kSyscallCount] = 
{
   [kSys_Read]    = nil, 
   [kSys_Write]   = _sysWrite
};

i32 syscallDispatch(u32 callId, u32 a0, u32 a1, u32 a2)
{
   if (callId >= kSyscallCount || !_sysCalls[callId])
      return -1;

   return _sysCalls[callId](a0, a1, a2);
}
