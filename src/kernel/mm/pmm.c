#include "bee.h"
#include "pmm.h"
#include "../lib/mem.h"
#include "../lib/logger.h"
#include "../boot/bootInfo.h"

typedef void (*fncFlagFrame)(u32 idx);

static u8 g_frameBitmap[kFrameBitmapSize] = {0};
static MemoryBitmap g_pmm;

/*
 * frames are flagged in a bit mask (g_pmm.bitmap)
 * where the 0th idx for a given stretch of 8 is the 
 * least significant bit: (0b00000001).
 */
void _pmmClaimFrame(u32 frameIndex)
{
   if (frameIndex >= g_pmm.totalFrames)
      return;

   u32 idx = frameIndex >> 3; // frame / 8
   u32 bit = frameIndex &  7; // frame % 8
   u8 mask = 1 << bit;
   if (g_pmm.bitmap[idx] & mask) {
      return;
   }

   g_pmm.bitmap[idx] |= mask;
   g_pmm.usedFrames++;
}

void _pmmReleaseFrame(u32 frameIndex)
{
   if (frameIndex >= g_pmm.totalFrames)
      return;

   u32 idx = frameIndex >> 3; // frame / 8
   u32 bit = frameIndex &  7; // frame % 8
   u8 mask = 1 << bit;
   if (!(g_pmm.bitmap[idx] & mask)) {
      return;
   }

   g_pmm.bitmap[idx] &= ~mask;
   g_pmm.usedFrames--;
}

u32 _pmmFindFreeFrame(u32 hint)
{
   u32 frameIdx = hint;
   while(frameIdx < g_pmm.totalFrames) {
      u32 idx  = frameIdx >> 3; // frame / 8
      u32 bit  = frameIdx &  7; // frame % 8
      u8  mask = 1 << bit;
      if (!(g_pmm.bitmap[idx] & mask)) {
         break;
      }

      frameIdx++;
   }
   
   if (frameIdx >= g_pmm.totalFrames) {
      return kInvalidFrame;
   }

   return frameIdx;
}

void _pmmFlagRangeHelper(u32 startAddr, u32 endAddr, fncFlagFrame fnc)
{
   if (endAddr > startAddr) {
      u32 startIdx = startAddr / kFrameSize;
      u32 endIdx   = endAddr   / kFrameSize;
      for (u32 idx = startIdx; idx < endIdx; idx++) {
         fnc(idx);
      }
   }
}

void _pmmReleaseAddrRange(u32 start, u32 end) 
{ 
   u32 startAddr = (start + kFrameSize - 1) & ~(kFrameSize - 1); // round up
   u32 endAddr   = end & ~(kFrameSize - 1);                      // round down

   _pmmFlagRangeHelper(startAddr, endAddr, _pmmReleaseFrame); 
}

void _pmmClaimAddrRange(u32 start, u32 end)   
{ 
   u32 startAddr = start & ~(kFrameSize - 1);                    // round down
   u32 endAddr   = (end + kFrameSize - 1) & ~(kFrameSize - 1);   // round up

   _pmmFlagRangeHelper(startAddr, endAddr, _pmmClaimFrame);  
}

void pmmInit(BootInfo* info)
{
   g_pmm.totalFrames = info->totalSystemRam / kFrameSize;
   g_pmm.usedFrames  = g_pmm.totalFrames;
   g_pmm.bitmap      = g_frameBitmap;
   g_pmm.bitmapSize  = kFrameBitmapSize;
   g_pmm.lastHint    = 0;

   // set all frames as claimed, then work through mmap entries and release available space
   memset(g_pmm.bitmap, 0xFF, kFrameBitmapSize);
   for (u32 i = 0; i < info->mmapEntryCount; i++) {
      if (info->mmapEntries[i].type == kMMapEntryType_Available) {
         u64 length   = kLowHighToU64(info->mmapEntries[i].lengthHigh, info->mmapEntries[i].lengthLow);
         if (length > 0xFFFFFFFF)
            length = 0xFFFFFFFF;

         u64 baseAddr = kLowHighToU64(info->mmapEntries[i].baseHigh, info->mmapEntries[i].baseLow);
         if (baseAddr > 0xFFFFFFFF)
            baseAddr = 0xFFFFFFFF;

         u64 endAddr  = baseAddr + length;
         if (endAddr > 0xFFFFFFFF)
            endAddr = 0xFFFFFFFF;

         _pmmReleaseAddrRange((u32)baseAddr, (u32)endAddr);
      }
   }

   // ensure we flag lower 1mb and kernel space as claimed
   _pmmClaimAddrRange(0x0, 1 * kMB);
   _pmmClaimAddrRange((u32)info->kernelPhysStart, (u32)info->kernelPhysEnd);
}

u32  pmmAllocFrame()
{
   u32 frameIdx = _pmmFindFreeFrame(g_pmm.lastHint);
   if (frameIdx == kInvalidFrame && g_pmm.lastHint != 0) {
      kTrace("last hint failed, trying again from 0");
      frameIdx = _pmmFindFreeFrame(0);
   }   
   if (frameIdx == kInvalidFrame)
      return kInvalidFrame;

   _pmmClaimFrame(frameIdx); 
   g_pmm.lastHint = frameIdx + 1;
   return frameIdx * kFrameSize;
}

void pmmFreeFrame(u32 addr)
{
   _pmmReleaseFrame(addr / kFrameSize);
}

void pmmDumpStats()
{
   u32 free = g_pmm.totalFrames - g_pmm.usedFrames;
   kTrace("*** PMM ***  Total: %u, Used: %u, Free: %u", g_pmm.totalFrames, g_pmm.usedFrames, free);
   kTrace("*** PMM ***  Last Hint: %u", g_pmm.lastHint);

   u8 map[8] = {0};
   for (u32 i = 0; i < 8; i++)
      map[i] = g_pmm.bitmap[i];

   kTrace("*** PMM ***  %x %x %x %x %x %x %x %x", 
      map[0], map[1], map[2], map[3], map[4], map[5], map[6], map[7]);
}
