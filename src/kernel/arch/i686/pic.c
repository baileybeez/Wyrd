#include "bee.h"
#include "pic.h"
#include "arch/i686/io.h"

#define kIcw1_Init      0x10
#define kIcw1_Icw4      0x01
#define kIcw4_8086      0x01

void picRemap(u8 primaryOffset, u8 secondOffset)
{
   u8 primMask = inb(kPicPrimary_Data);
   u8 secdMask = inb(kPicSecondary_Data);

   outb(kPicPrimary_Cmd, kIcw1_Init | kIcw1_Icw4);
   ioWait();
   outb(kPicSecondary_Cmd, kIcw1_Init | kIcw1_Icw4);
   ioWait();

   outb(kPicPrimary_Data, primaryOffset);
   ioWait();
   outb(kPicSecondary_Data, secondOffset);
   ioWait();

   outb(kPicPrimary_Data, 0x04);
   ioWait();
   outb(kPicSecondary_Data, 0x02);
   ioWait();

   outb(kPicPrimary_Data, kIcw4_8086);
   ioWait();
   outb(kPicSecondary_Data, kIcw4_8086);
   ioWait();

   outb(kPicPrimary_Data, primMask);
   outb(kPicSecondary_Data, secdMask);
}

void picSendEoi(u8 irq)
{
   if (irq >= 8) {
      outb(kPicSecondary_Cmd, kPicEOI);
   }

   outb(kPicPrimary_Cmd, kPicEOI);
}

void picSetMask(u8 irq)
{
   u16 port = irq < 8 ? kPicPrimary_Data : kPicSecondary_Data;
   u8 bit   = irq < 8 ? irq : irq - 8;
   u8 value = inb(port) | (1 << bit);

   outb(port, value);
}

void picClearMask(u8 irq)
{
   u16 port = irq < 8 ? kPicPrimary_Data : kPicSecondary_Data;
   u8 bit   = irq < 8 ? irq : irq - 8;
   u8 value = inb(port) & ~(1 << bit);

   outb(port, value);
}

void picMaskAll(void)
{
   outb(kPicPrimary_Data, 0xFF);
   outb(kPicSecondary_Data, 0xFF);
}