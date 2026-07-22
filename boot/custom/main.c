#include "bee.h"
#include "drivers/serial/serial.h"

extern u8 kBootInfoAddr[];
extern u8 kKernelLoadAddr[];

static void fallback_vgaWrite(const char* s)
{
   volatile u16* vga = (volatile u16*)0xB8000;
   for (u32 i = 0; s[i]; i++) {
      vga[i] = (u16)(0x4F00 | (u8)s[i]);
   }
}

void stage2Main(u32 bootDrive)
{
   (void)bootDrive;
   (void)kBootInfoAddr;
   (void)kKernelLoadAddr;

   if (!serialInit()) {
      fallback_vgaWrite("[PANIC] failed to initialize serial out!");
      for (;;) { __asm__ volatile ("hlt"); }
   }

   serialPrintf("[STAGE2] alive!");
   for (;;) { __asm__ volatile ("hlt"); }
}
