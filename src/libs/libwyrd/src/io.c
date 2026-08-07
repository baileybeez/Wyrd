#include "wyrd.h"
#include "io.h"
#include "syscall.h"

i32 putchar(const char c)
{
   return write(kFd_StdOut, &c, 1);
}

i32 puts(const char* str)
{
   u32 n = 0;
   for (const char* c = str; c != 0; c++) {
      putchar(*c);
      n++;
   }

   if (n == 1)
      return 1;

   putchar('\n');
   return n;
}
