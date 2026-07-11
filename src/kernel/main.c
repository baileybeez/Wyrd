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
#include "./boot/bootInfo.h"
#include "./boot/multiboot.h"
#include "./mm/pmm.h"

void stressTestPMM()
{
   u32 a = pmmAllocFrame();
   u32 b = pmmAllocFrame();
   u32 c = pmmAllocFrame();
   kTrace("Alloc attempts: %x, %x, %x", a, b, c);
   kTrace("        frames: %u, %u, %u", a / kFrameSize, b / kFrameSize, c / kFrameSize);

   u32 addr = 0;
   while (addr != kInvalidFrame) {
      addr = pmmAllocFrame();
   }
   pmmDumpStats();

   pmmFreeFrame(b);
   pmmDumpStats();

   u32 d = pmmAllocFrame();
   kTrace("*** PMM STRESS *** Expected that %x == %x", b, d);
   pmmDumpStats();
}

void kernelMain(u32 mbMagic, u32 mbInfo)
{
   vgaInit();
   serialInit();
   logInit(kLogInfo, kLogTrace);
   kTrace("+ VGA initialized.");

   BootInfo bootInfo = {0};
   if (mbMagic != kMultibootMagic) {
      kPanic("Invalid Magic");
      for(;;);
   }
   multiboot2Parse(mbInfo, &bootInfo);
   kTrace("System RAM: %u", bootInfo.totalSystemRam);
   kTrace("Kernel    : %p -> %p", bootInfo.kernelPhysStart, bootInfo.kernelPhysEnd);
   pmmInit(&bootInfo);
   pmmDumpStats();
   // stressTestPMM();

   gdtInit();
   kTrace("+ GDT initialized.");
   idtInit();
   kTrace("+ IDT initialized.");

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
         kTrace("tick: %u", now);
         prev = now;
      }
   }

   // Halt
   kPanic("- System Halted -");
   for (;;);
   return;
}
