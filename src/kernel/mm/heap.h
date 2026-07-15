#pragma once
#include "bee.h"

#define kHeapVirtaulStart  0xD0000000
#define kHeapInitialSize   (64 * 1024)
#define kHeapMinPayload    16

typedef struct _BlockHeader {
   u32   size;
   bool  free;
   struct _BlockHeader* next;
} BlockHeader;

void heapInit();
void* kmalloc(u32 size);
void kfree(void* ptr);
