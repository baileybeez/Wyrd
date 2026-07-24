#pragma once
#include "wyrd.h"
#include "../boot/bootInfo.h"

#define kFrameSize         4096
#define kKB                1024
#define kMB                1024   * kKB
#define kGB                1024   * kMB
#define kMaxMemory         (u64)4 * kGB

#define kInvalidFrame      0xFFFFFFFF

#define kMaxFrames         1048576     /* = kMaxMemory / kFrameSize */
#define kFrameBitmapSize   131072      /* = kMaxFrames / 8 bits */

typedef struct 
{
   u32 totalFrames;
   u32 usedFrames;
   u8* bitmap;
   u32 bitmapSize;
   u32 lastHint;
} MemoryBitmap;

void pmmInit(BootInfo* info);
u32  pmmAllocFrame();
void pmmFreeFrame(u32 addr); 
void pmmDumpStats();
