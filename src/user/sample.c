#include "wyrd.h"
#include "arch/i686/syscallEntry.h"
#include "syscall.h"

int _start() 
{
   const char* str = "hello world\n";
   u32 len = 12;
   syscall3(kSys_Write, (u32)str, len, 0);

   syscall3(kSys_Exit, 0, 0, 0);
   return 0;
}
