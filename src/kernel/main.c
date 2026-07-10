#include "bee.h"
#include "./lib/logger.h"
#include "./drivers/serial/serial.h"
#include "./drivers/video/vga.h"
#include "./drivers/input/keyboard.h"
#include "./arch/i686/gdt.h"
#include "./arch/i686/idt.h"
#include "./arch/i686/pic.h"
#include "./arch/i686/irq.h"
#include "./arch/i686/ticks.h"

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

   picRemap(kIrqBase, kIrqBase + 8);
   irqInit();
   ticksInit(100);
   picClearMask(0);
   __asm__ volatile("sti");

   keyboardInit();
   picClearMask(1);
   print("> ");

   u32 prev = 0;
   while (true) {
      char c;
      if (keyboardTryReadKey(&c))
         putChar(c);

      u32 now = ticksGetCount();
      if (now - prev >= 100) {
         serialPrintf("tick: %u\n", now);
         prev = now;
      }
   }

   // Halt
   kPanic("- System Halted -");
   for (;;);
   return;
}
