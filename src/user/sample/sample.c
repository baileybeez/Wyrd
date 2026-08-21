#include "wyrd.h"
#include "sys.h"
#include "syscall.h"
#include "string.h"

i32 main(i32 argc, char** argv) 
{
   const char* str = "hello world from write\n\n";
   u32 len = 23;
   write(kFd_StdOut, str, len);
   
   // echo out all arguments
   for (i32 i = 0; i < argc; i++) {
      u32 len = strlen(argv[i]);
      write(kFd_StdOut, argv[i], len);
      write(kFd_StdOut, "\n", 1);
   }

   return 0; 
}
