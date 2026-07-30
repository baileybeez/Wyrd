#include "wyrd.h"
#include "arch/i686/arch.h"
#include "arch/i686/cpu.h"
#include "arch/i686/paging.h"
#include "arch/i686/pic.h"
#include "arch/i686/syscallEntry.h"
#include "arch/i686/ticks.h"
#include "boot/bootInfo.h"
#include "boot/multiboot.h"
#include "drivers/ata/ata.h"
#include "drivers/input/keyboard.h"
#include "drivers/serial/serial.h"
#include "drivers/video/vga.h"
#include "fs/fat16/fat16.h"
#include "lib/logger.h"
#include "lib/mem.h"
#include "lib/panic.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "scheduler/thread.h"
#include "scheduler/scheduler.h"
#include "syscall.h"
#include "userProg.h"

extern u32 _kernelStart;
extern u32 _kernelPhysicalEnd;

static Fat16Volume  g_Vol;
static u8           g_FatBuffer[64 * 1024];
static u8           g_RootDirBuffer[16 * 1024];


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
      kernelPanic("expected 0x00100000");

   // test get phys addr returns invalid address when passed a bad request
   addr = pagingGetPhysical(0x00001000);
   kTrace("getPhys(0x00001000) returns 0x%x (expects 0xFFFFFFFF)", addr);
   if (addr != kInvalidPhysical)
      kernelPanic("expected 0xFFFFFFFF");

   // Map / Write / Read round-trip test
   u32 frame = pmmAllocFrame();
   pagingMapPage(0xE0000000, frame, kPageFlag_Writable);
   *(u32*)0xE0000000 = 0xDEADBEEF;
   kTrace("readback = %x (expects 0xDEADBEEF)", *(u32*)0xE0000000);
   kTrace("phys = %x (want %x)", pagingGetPhysical(0xE0000000), frame);

   // trigger page fault handler (uncomment to test - will panic kernel for now)
   // pagingUnmapPage(0xE0000000);
   // *(u32*)0xE0000000 = 0x1234;
}

void testHeap()
{
   // TEST: allocation
   u32* p = (u32*)kmalloc(100);
   kTrace("alloc gave: %p (expects > 0xD0000000)", p);

   // TEST: accessing / assignment
   *p = 0xdeadbeef;
   u32 v = *p;
   kTrace("memory assignment: 0x%x ==? 0xdeadbeef", v);

   p[0] = 1;
   p[1] = 2;
   p[2] = 4;
   p[3] = 8;
   u32 u = 0;
   for (int i = 0; i < 4; i++) {
      u += p[i];
   }
   kTrace("array access assignment: %u ==? 15", u);

   // TEST: frame splitting
   u32* q = (u32*)kmalloc(100);
   kTrace("alloc gave: %p (expects ~112 bytes higher than first alloc)", q);

   // TEST: frame freeing / coalescing
   u32* r = kmalloc(100);
   kfree(p);
   kfree(q);
   p = kmalloc(200);
   kTrace("alloc gave: %p (expects > 0xD0000000 and reusing earlier frame)", p);
   kfree(r);

   // TEST: fragment use without coalesce
   p = kmalloc(100);
   q = kmalloc(100);
   r = kmalloc(100);
   kTrace(" -- freeing frame at %p (should be reused in the next malloc)", q);
   kfree(q);
   q = kmalloc(64);
   kTrace("alloc gave: %p (expects > 0xD0000000 and reusing middle frame)", q);
   kfree(p);
   kfree(q);
   kfree(r);

   // TEST: force grow
   p = kmalloc(kHeapInitialSize * 2);
   u32 phys = pagingGetPhysical((u32)p);
   kTrace("growth alloc gave: %p at phys addr: 0x%x (expects > 0xD0000000)", p, phys);
}

// static void _demoA() { for (;;) { serialWriteString("A"); } }
// static void _demoB() { for (;;) { serialWriteString("B"); } }

static void _kernelCompanion()
{
   for (;;) {
      kTrace("[kernel] tick");
      for (volatile u32 i = 0; i < 0x02000000; ++i) { }
   }
}

void kernelMain(BootInfo* bi)
{
   vgaInit();
   kTrace("+ VGA initialized.");

   bi->kernelPhysStart = &_kernelStart;
   bi->kernelPhysEnd   = &_kernelPhysicalEnd;
   bi->totalSystemRam  = bootInfoCalcSystemRam(bi);
   
   kTrace("System RAM: %u", bi->totalSystemRam);
   kTrace("Kernel    : %p -> %p", bi->kernelPhysStart, bi->kernelPhysEnd);
   pmmInit(bi);
   pmmDumpStats();
   // stressTestPMM();
   
   archInitEarly();

   pagingInit();
   // testPaging();
   // pagingTestUserMapping();
   
   heapInit();
   testHeap();
   
   syscallInit();
   archInitLate();

   keyboardInit();
   picClearMask(1);

   schedulerInit();
   // threadCreate(_demoA);
   // threadCreate(_demoB);
   interruptsEnable();

#ifdef kIncludeSelfTests
   // ATA
   ataSelfTest();
#endif

   // FAT16
   Fat16Error err = fat16Mount(&g_Vol, ataReadSectors, g_FatBuffer, sizeof g_FatBuffer, g_RootDirBuffer, sizeof g_RootDirBuffer);
   if (err != kFatErr_OK) {
      serialPrintf("[FAT16] mount failed: %x\n", err);
#ifdef kIncludeSelfTests
   } else {
      fat16SelfTest(&g_Vol);
#endif
   }
   
   // TODO: spawn user Process and system thread
   threadCreate(_kernelCompanion);
   spawnUserThread();
   for(;;);

   // syscall (sysWrite) test
   const char* str = "Hello World\n";
   i32 n = syscall3(kSys_Write, (u32)str, 12, 0);
   kTrace("sys_write returned %d (excepted 11)", n);

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
   kernelPanic("- System Halted -");
   return;
}

void kernelBootstrap(u32 magic, u32 ptr)
{
   serialInit();
   logInit(kLogInfo, kLogTrace);
   kTrace("Preparing the Wyrd ...");
   kTrace("[BOOT] magic=%x, ptr=%x", magic, ptr);

   BootInfo* bi = (BootInfo*)kBootInfoAddr;
   if (magic == kCustomBootMagic) {
      // stage2 already built out the BootInfo struct
      // ptr should equal (u32)kBootInfoAddr; nothing to do
      (void)ptr;
   } else if (magic == kMultibootMagic) {
      memset(bi, 0, sizeof(BootInfo));
      bi->magic          = kCustomBootMagic;
      bi->kernelLoadAddr = kKernelLoadAddr;
      bi->bootDrive      = 0;
      multiboot2Parse(ptr, bi);
   } else {
      kernelPanic("Unknown bootloader: %x", magic);
   }

   kernelMain(bi);
}
