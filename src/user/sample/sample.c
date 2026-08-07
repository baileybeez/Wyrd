#include "wyrd.h"
#include "sys.h"
#include "syscall.h"

int main() 
{
   const char* str = "hello world from write\n";
   u32 len = 23;
   write(kFd_StdOut, str, len);
   
   return 0; 
}
