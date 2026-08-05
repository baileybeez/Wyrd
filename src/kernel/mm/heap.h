#pragma once
#include "wyrd.h"

#define kHeapVirtualStart  0xD0000000
#define kHeapPDEFirst      (kHeapVirtualStart >> 22)
#define kHeapPDECount      16
#define kHeapVirtualLimit  (kHeapVirtualStart + (kHeapPDECount * 0x400000))
#define kHeapInitialSize   (64 * 1024)
#define kHeapMinPayload    16

typedef struct _BlockHeader {
   u32   size;
   bool  free;
   struct _BlockHeader* next;
} BlockHeader;

void heapInit();
u32  heapFreeBytes();

void* kmalloc(u32 size);
void  kfree(void* ptr);
