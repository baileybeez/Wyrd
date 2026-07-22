#include "bee.h"
#include "drivers/serial/serial.h"
#include "boot/bootInfo.h"

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

   serialPrintf("[STAGE2] alive!\n");
   BootInfo* bi = (BootInfo*)kBootInfoAddr;
   serialPrintf("E820: %u entries\n", bi->mmapEntryCount);
   for (u32 i = 0; i < bi->mmapEntryCount; i++) {
      MemoryMapEntry* e = &bi->mmapEntries[i];
      serialPrintf("  [%u] base=%x%x len=%x type=%u\n",       
         i, e->baseHigh, e->baseLow, e->lengthHigh, e->lengthLow, e->type);
   }
      
   for (;;) { __asm__ volatile ("hlt"); }
}
