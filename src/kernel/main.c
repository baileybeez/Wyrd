#include "bee.h"
#include "./lib/logger.h"
#include "./drivers/serial/serial.h"
#include "./drivers/video/vga.h"
#include "./arch/i686/gdt.h"
#include "./arch/i686/idt.h"


void kernelMain()
{
   vgaInit();
   serialInit();
   logInit(kLogInfo, kLogTrace);
   kInfo("+ VGA initialized.");
   
   gdtInit();
   kInfo("+ GDT initialized.");
   idtInit();
   kInfo("+ IDT initialized.");

   // Halt
   kPanic("- System Halted -");
   for (;;);
   return;
}