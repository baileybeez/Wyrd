#include "wyrd.h"
#include "mem.h"

void* memset(void* dest, u8 val, u32 count)
{
   u8* ptr = (u8*)dest;
   u8* end = ptr + count;
   for (; ptr < end; ptr++) {
      *ptr = val;
   }
   return dest;
}