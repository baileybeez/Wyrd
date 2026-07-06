#include "bee.h"
#include "vga.h"

void kernelMain()
{
   vga_init();
   print("hello world");

   // Halt
   for (;;);
   return;
}