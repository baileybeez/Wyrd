#include "wyrd.h"
#include "../arch/i686/paging.h"
#include "pmm.h"
#include "heap.h"
#include "lib/logger.h"

#define kHeapWalkLimit 2

// our heap is linked-list, first-fit 
static BlockHeader*  g_heapHead = nil;
static u32           g_heapEnd  = nil;

static BlockHeader* _heapFindFreeBlock(u32 request)
{
   BlockHeader* block = g_heapHead;
   while (block != nil) {
      if (block->free && block->size >= request)
         return block;

      block = block->next;
   }

   return nil;
}

static bool _heapGrow(u32 minBytes)
{
   u32 needed = (minBytes + sizeof(BlockHeader) + kPageSize - 1) & ~(kPageSize - 1);
   u32 oldEnd = g_heapEnd;

   for (u32 mapped = 0; mapped < needed; mapped += kPageSize) {
      u32 frame = pmmAllocFrame();
      if (frame == kInvalidFrame)
         return false;

      if (!pagingMapPage(g_heapEnd, frame, kPageFlag_Writable)) {
         pmmFreeFrame(frame);
         return false;
      }

      g_heapEnd += kPageSize;
   }

   BlockHeader* freshBlock = (BlockHeader*)oldEnd;
   freshBlock->size = (g_heapEnd - oldEnd) - sizeof(BlockHeader);
   freshBlock->free = true;
   freshBlock->next = nil;

   BlockHeader* tail = g_heapHead;
   while (tail->next)
      tail = tail->next;
   tail->next = freshBlock;
   return true;
}

// walk through the heap, coalescing each neighboring free block into a single block
void _heapCoalesce()
{
   BlockHeader* block = g_heapHead;
   while (block && block->next) {
      if (block->free && block->next->free) {
         block->size += sizeof(BlockHeader) + block->next->size;
         block->next = block->next->next;
         // don't advance here - newly merged `block` may need to merge with next neighbor
      } else {
         block = block->next;
      }
   }
}

void heapInit()
{
   for (u32 offset = 0; offset < kHeapInitialSize; offset += kPageSize) {
      u32 frame = pmmAllocFrame();
      if (frame == kInvalidFrame)
         kPanic("unable to allocate initial heap frame %u", offset);

      pagingMapPage(kHeapVirtualStart + offset, frame, kPageFlag_Writable);
   }
   g_heapEnd  = kHeapVirtualStart + kHeapInitialSize;
   g_heapHead = (BlockHeader*)kHeapVirtualStart;
   g_heapHead->size = kHeapInitialSize - sizeof(BlockHeader);
   g_heapHead->free = true;
   g_heapHead->next = nil;
}

// we want to round the size up to the nearest 8-byte multiple
// then we can walk the heap looking for a free block thats big enough
// if we do not find one, we grow the heap and try one more time
// otherwise, we check to see if we can split the found block (min heap payload)
//            and splice it into the linked list
// lastly, we claim the block and return a pointer to the block's data (skip header)
void* kmalloc(u32 size)
{
   if (g_heapHead == nil)
      return nil;                      // heap is not initialized yet

   u32 request = (size + 7) & ~0x07;   // round size up to 8byte multiple
   BlockHeader* block = _heapFindFreeBlock(request);
   if (block == nil) {
      _heapGrow(request);
      block = _heapFindFreeBlock(request);
      if (block == nil) 
         return nil;                   // grew but can't fit
   }
   
   if (block->size - request >= sizeof(BlockHeader) + kHeapMinPayload) {
      BlockHeader* frag = (BlockHeader*)((u8*)block + sizeof(BlockHeader) + request);
      frag->size = block->size - request - sizeof(BlockHeader);
      frag->free = true;
      frag->next = block->next;

      block->size = request;
      block->next = frag;
   }

   block->free = false;
   return (void*)(block + 1);  // this advances past the header (to the data segemnt)
}

// if the ptr isn't nil, we move the pointer back to the block header
// then we free the block
// then we attempt to coalesce
void kfree(void* ptr)
{
   if (ptr == nil)
      return;

   BlockHeader* block = (BlockHeader*)ptr - 1;
   block->free = true;
   _heapCoalesce();
}
