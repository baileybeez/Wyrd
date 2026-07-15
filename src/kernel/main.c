#include "bee.h"
#include "./arch/i686/gdt.h"
#include "./arch/i686/idt.h"
#include "./arch/i686/irq.h"
#include "./arch/i686/pic.h"
#include "./arch/i686/paging.h"
#include "./arch/i686/ticks.h"
#include "./boot/bootInfo.h"
#include "./boot/multiboot.h"
#include "./drivers/input/keyboard.h"
#include "./drivers/serial/serial.h"
#include "./drivers/video/vga.h"
#include "./lib/logger.h"
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

void testPaging()
{
   u32 addr = kInvalidPhysical;

   // test get phys addr returns correct address when passed a valid request
   addr = pagingGetPhysical(0xC0100000);
   kTrace("getPhys(0xC0100000) returns 0x%x (expects 0x00100000)", addr);
   if (addr != 0x00100000)
      kPanic("expected 0x00100000");

   // test get phys addr returns invalid address when passed a bad request
   addr = pagingGetPhysical(0x00001000);
   kTrace("getPhys(0x00001000) returns 0x%x (expects 0xFFFFFFFF)", addr);
   if (addr != kInvalidPhysical)
      kPanic("expected 0xFFFFFFFF");

   // Map / Write / Read round-trip test
   u32 frame = pmmAllocFrame();
   pagingMapPage(0xE0000000, frame, kPageFlag_Writable);
   *(u32*)0xE0000000 = 0xDEADBEEF;
   kTrace("readback = %x (expects 0xDEADBEEF)", *(u32*)0xE0000000);
   kTrace("phys = %x (want %x)", pagingGetPhysical(0xE0000000), frame);

   // trigger page fault handler
   pagingUnmapPage(0xE0000000);
   *(u32*)0xE0000000 = 0x1234;
}

void kernelMain(u32 mbMagic, u32 mbInfo)
{
   kTrace("Preparing BeeOS ...");
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

   pagingInit();
   testPaging();
   
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
