#include "bee.h"
#include "./drivers/serial/serial.h"
#include "./drivers/video/vga.h"

void kernelMain()
{
   vgaInit();
   print("hello world");

   if (serialInit()) {
      serialWriteString("hello from serial\n");
      serialPrintf("decimal=%d, hex=%08x, ptr=%p, char=%c, pct=%%\n", 
         -42, 0xDEADBEEF, (void*)0x1000, 'K');
   }

   // Halt
   for (;;);
   return;
}