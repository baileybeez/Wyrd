#include "bee.h"
#include "bootInfo.h"

u32 bootInfoCalcSystemRam(const BootInfo* bi)
{
   u32 highestAddr = 0;
   for (u32 i = 0; i < bi->mmapEntryCount; i++) {
      const MemoryMapEntry* e = (const MemoryMapEntry*)&bi->mmapEntries[i];
      if (e->baseHigh != 0)
         continue;   // skip anything above 4GB
      
      u64 len = kLowHighToU64(e->lengthHigh, e->lengthLow);
      u64 end = e->baseLow + len;
      if (end > 0xFFFFFFFF)
         end = 0xFFFFFFFF;
      
      if ((u32)end > highestAddr)
         highestAddr = (u32)end;
   }
   return highestAddr;
}
