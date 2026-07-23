#include "bee.h"
#include "drivers/serial/serial.h"
#include "boot/bootInfo.h"
#include "drivers/ata/ata.h"
#include "fs/fat16/fat16.h"

#define kKernelFilename "kernel.bin"

extern u8 kBootInfoAddr[];
extern u8 kKernelLoadAddr[];
extern u8 kFatBufferAddr[];
extern u8 kRootDirBufferAddr[];

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
      goto halt;
   }

   serialPrintf("[STAGE2] alive!\n");
   BootInfo* bi = (BootInfo*)kBootInfoAddr;
   serialPrintf("E820: %u entries\n", bi->mmapEntryCount);
   for (u32 i = 0; i < bi->mmapEntryCount; i++) {
      MemoryMapEntry* e = &bi->mmapEntries[i];
      serialPrintf("  [%u] base=%x%x len=%x type=%u\n",       
         i, e->baseHigh, e->baseLow, e->lengthHigh, e->lengthLow, e->type);
   }

   #ifdef kATA_SelfTest
   // this should report (via serial console): 
   //    [ATA] selftest: reading LBA 0...
   //    [ATA] first 16 bytes: eb 3c 90 42 45 45 42 4f 4f 54 20 0 2 1 11 0
   //    [ATA] selftest: PASS - boot signature 55 AA present
   ataSelfTest();
   #endif

   Fat16Volume vol;
   Fat16Error  err = fat16Mount(&vol, ataReadSectors, kFatBufferAddr, 0x10000, kRootDirBufferAddr, 0x4000);
   if (err != kFAT16_OK) {
      serialPrintf("[FAT] mount err=%d", err);
      goto halt;
   }

   serialPrintf("[FAT] mounted: fatLba=%d rootLba=%d dataLba=%d bpc=%d\n",
      vol.fatStartLba, vol.rootDirStartLba, vol.dataStartLba, vol.bytesPerCluster);

   u32 firstCluster;
   u32 fileSize;
   err = fat16FindFile(&vol, kKernelFilename, &firstCluster, &fileSize);
   if (err != kFAT16_OK) {
      serialPrintf("[FAT] findFile err=%d", err);
      goto halt;
   }
   serialPrintf("[FAT] KERNEL.BIN cluster=%d size=%d\n", firstCluster, fileSize);

   err = fat16ReadFile(&vol, firstCluster, fileSize, (void*)kKernelLoadAddr);
   if (err != kFAT16_OK) {
      serialPrintf("[FAT] readFile err=%d", err);
      goto halt;
   }
   
   u8* k = (u8*)kKernelLoadAddr;
   serialPrintf("[FAT] loaded, first 8 bytes: %x %x %x %x %x %x %x %x\n",
      k[0], k[1], k[2], k[3], k[4], k[5], k[6], k[7]);
      
   // TODO: jump?
halt:
   for (;;) { __asm__ volatile ("hlt"); }
}
