#include "wyrd.h"
#include "arch/i686/io.h"
#include "ata.h"

#include "drivers/serial/serial.h"

#define kATA_PrimaryIOBase    0x1F0
#define kATA_PrimaryCtrlBase  0x3F6

#define kATA_RegData          (kATA_PrimaryIOBase + 0)
#define kATA_RegError         (kATA_PrimaryIOBase + 1)
#define kATA_RegSectorCount   (kATA_PrimaryIOBase + 2)
#define kATA_RegLbaLow        (kATA_PrimaryIOBase + 3)
#define kATA_RegLbaMid        (kATA_PrimaryIOBase + 4)
#define kATA_RegLbaHigh       (kATA_PrimaryIOBase + 5)
#define kATA_RegDriveHead     (kATA_PrimaryIOBase + 6)
#define kATA_RegStatus        (kATA_PrimaryIOBase + 7)
#define kATA_RegCommand       (kATA_PrimaryIOBase + 7)

#define kATA_RegAltStatus     (kATA_PrimaryCtrlBase + 0)

#define kATA_StatusErr        0x01
#define kATA_StatusDrq        0x08
#define kATA_StatusDf         0x20
#define kATA_StatusBusy       0x80

#define kATA_CmdReadSectors   0x20
 
#define kATA_DriveMasterLba   0xE0
 
#define kATA_SectorSize       512
#define kATA_WordsPerSector   256

static void ataDelay400ns()
{
   inb(kATA_RegAltStatus);
   inb(kATA_RegAltStatus);
   inb(kATA_RegAltStatus);
   inb(kATA_RegAltStatus);
}

static bool ataAwaitNotBusy()
{
   for (u32 i = 0; i < kATA_WaitTimeout; i++) {
      u8 status = inb(kATA_RegStatus);
      if (status == 0xFF)
         return false;

      if (!(status & kATA_StatusBusy))
         return true;
   }

   return false;
}

static bool ataWaitDrq()
{
   for (u32 i = 0; i < kATA_WaitTimeout; i++) {
      u8 status = inb(kATA_RegStatus);
      if (status == 0xFF)
         return false;

      if (!(status & kATA_StatusBusy) && (status & kATA_StatusDrq))
         return true;
   }

   return false;
}

bool ataReadSectors(u32 lba, u8 count, void* dest)
{
   if (count == 0)
      return true;
   if ((lba >> 28) != 0)
      return false;

   if (!ataAwaitNotBusy())
      return false;

   outb(kATA_RegDriveHead, kATA_DriveMasterLba | ((lba >> 24) & 0x0F));
   ataDelay400ns();

   outb(kATA_RegSectorCount, count);
   outb(kATA_RegLbaLow,  (u8)(lba & 0xFF));
   outb(kATA_RegLbaMid,  (u8)((lba >> 8) & 0xFF));
   outb(kATA_RegLbaHigh, (u8)((lba >> 16) & 0xFF));
 
   outb(kATA_RegCommand, kATA_CmdReadSectors);
 
   u8* dst = (u8*)dest;
   for (u32 s = 0; s < count; s++) {
      if (!ataWaitDrq()) {
         return false;
      }
      insw(kATA_RegData, dst, kATA_WordsPerSector);
      dst += kATA_SectorSize;
      ataDelay400ns();
   }
 
   return true;

}

#ifdef kATA_SelfTest
void ataSelfTest()
{
   static u8 buffer[kATA_SectorSize];
 
   serialPrintf("[ATA] selftest: reading LBA 0...\n");
 
   if (!ataReadSectors(0, 1, buffer)) {
      serialPrintf("[ATA] selftest: FAIL - read returned false\n");
      return;
   }
 
   serialPrintf("[ATA] first 16 bytes:");
   for (u32 i = 0; i < 16; i++) {
      serialPrintf(" %x", buffer[i]);
   }
   serialPrintf("\n");
 
   if (buffer[510] == 0x55 && buffer[511] == 0xAA) {
      serialPrintf("[ATA] selftest: PASS - boot signature 55 AA present\n");
   } else {
      serialPrintf("[ATA] selftest: FAIL - signature was %x %x (expected 55 AA)\n",
         buffer[510], buffer[511]);
   }
}
#endif
