#include "bee.h"
#include "heap.h"

#define kHeapWalkLimit 2

// our heap is linked-list, first-fit 
static BlockHeader*  g_heapHead;
static u32           g_heapEnd;

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

void _heapGrow(u32 minBytes)
{
   // TODO: 
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
   // TODO: loop through and create frames for `kHeapInitialSize`
   // TODO: setup `g_heapEnd`
   // TODO: setup `g_heapHead`
}

// we want to round the size up to the nearest 8-byte multiple
// then we can walk the heap looking for a free block thats big enough
// if we do not find one, we grow the heap and try one more time
// otherwise, we check to see if we can split the found block (min heap payload)
//            and splice it into the linked list
// lastly, we claim the block and return a pointer to the block's data (skip header)
void* kmalloc(u32 size)
{
   u32 request = (size + 7) & ~0x07;   // round size up to 8byte multiple
   BlockHeader* block = _heapFindFreeBlock(request);
   if (block == nil) {
      _heapGrow(request);
      block = _heapFindFreeBlock(request);
      if (block == nil) { /* should we panic here? */ }
   }
   
   if (block->size - request >= sizeof(BlockHeader) + kHeapMinPayload) {
      // TODO: split block and splice new fragment into linked list
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
