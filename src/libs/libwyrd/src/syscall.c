#include "wyrd.h"
#include "sys.h"

static inline i32 _syscall1(u32 number, u32 arg0)
{
   i32 result;
   __asm__ volatile (
      "int $0x80" 
      : "=a"(result) 
      : "a"(number), "b"(arg0) : "memory"
   );
   return result;
}

static inline i32 _syscall2(u32 number, u32 arg0, u32 arg1)
{
   i32 result;
   __asm__ volatile (
      "int $0x80" 
      : "=a"(result) 
      : "a"(number), "b"(arg0), "c"(arg1) : "memory"
   );
   return result;
}

static inline i32 _syscall3(u32 number, u32 arg0, u32 arg1, u32 arg2)
{
   i32 result;
   __asm__ volatile (
      "int $0x80" 
      : "=a"(result) 
      : "a"(number), "b"(arg0), "c"(arg1), "d"(arg2) : "memory"
   );
   return result;
}

i32 write(i32 fd, const void* buffer, u32 len)
{
   return _syscall3(kSyscall_Write, (u32)fd, (u32)buffer, len);
}

i32 exit(i32 code)
{
   _syscall1(kSyscall_Exit, code);
   kForever;
}
