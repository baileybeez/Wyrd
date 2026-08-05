#include "wyrd.h"
#include "../arch/i686/paging.h"
#include "pmm.h"
#include "heap.h"
#include "lib/logger.h"
#include "lib/panic.h"

#define kHeapWalkLimit 2

// our heap is linked-list, first-fit 
static BlockHeader*  _heapHead = nil;
static u32           _heapEnd  = nil;

static BlockHeader* _heapFindFreeBlock(u32 request)
{
   BlockHeader* block = _heapHead;
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
   if (_heapEnd + needed >= kHeapVirtualLimit) {
      kTrace("heap: unable to grow heap (out of reserved space)");
      return false;
   }

   u32 oldEnd = _heapEnd;
   for (u32 mapped = 0; mapped < needed; mapped += kPageSize) {
      u32 frame = pmmAllocFrame();
      if (frame == kInvalidFrame)
         return false;

      if (!pagingMapPage(_heapEnd, frame, kPageFlag_Writable)) {
         pmmFreeFrame(frame);
         return false;
      }

      _heapEnd += kPageSize;
   }

   BlockHeader* freshBlock = (BlockHeader*)oldEnd;
   freshBlock->size = (_heapEnd - oldEnd) - sizeof(BlockHeader);
   freshBlock->free = true;
   freshBlock->next = nil;

   BlockHeader* tail = _heapHead;
   while (tail->next)
      tail = tail->next;
   tail->next = freshBlock;
   return true;
}

// walk through the heap, coalescing each neighboring free block into a single block
void _heapCoalesce()
{
   BlockHeader* block = _heapHead;
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
   if (!pagingReserveRange(kHeapVirtualStart, kHeapVirtualLimit))
      kernelPanic("heapInit: unable to preallocate PDEs");

   for (u32 offset = 0; offset < kHeapInitialSize; offset += kPageSize) {
      u32 frame = pmmAllocFrame();
      if (frame == kInvalidFrame)
         kernelPanic("heapInit: unable to allocate initial heap frame %u", offset);

      if (!pagingMapPage(kHeapVirtualStart + offset, frame, kPageFlag_Writable))
         kernelPanic("heapInit: unable to map virtual heap frame %x", kHeapVirtualStart + offset);
   }
   _heapEnd  = kHeapVirtualStart + kHeapInitialSize;
   _heapHead = (BlockHeader*)kHeapVirtualStart;
   _heapHead->size = kHeapInitialSize - sizeof(BlockHeader);
   _heapHead->free = true;
   _heapHead->next = nil;
}

u32 heapFreeBytes()
{
   u32 freeBytes = 0;
   BlockHeader* block = _heapHead;
   while (block != nil) {
      if (block->free) 
         freeBytes += block->size;

      block = block->next;
   }

   return freeBytes;
}

// we want to round the size up to the nearest 8-byte multiple
// then we can walk the heap looking for a free block thats big enough
// if we do not find one, we grow the heap and try one more time
// otherwise, we check to see if we can split the found block (min heap payload)
//            and splice it into the linked list
// lastly, we claim the block and return a pointer to the block's data (skip header)
void* kmalloc(u32 size)
{
   if (_heapHead == nil)
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
