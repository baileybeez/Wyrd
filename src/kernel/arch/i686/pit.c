#include "bee.h"
#include "pit.h"
#include "arch/i686/io.h"

#define kPitMode_SquareWave   0x36
#define kPitMode_RateGen      0x34

void pitSetFrequency(u32 hz)
{
   u32 div = kPitBaseFrequency / hz;
   if (div > 0xFFFF)
      div = 0xFFFF;
   else if (div < 1)
      div = 1;

   outb(kPitCommand, kPitMode_RateGen);
   outb(kPitChannel0Data, (u8)(div & 0xFF));
   outb(kPitChannel0Data, (u8)((div >> 8) & 0xFF));
}
