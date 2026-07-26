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

// TODO: we'll want to migrate this to a word-size copy with remnant handling
void* memcpy(void* dest, const void* src, u32 count)
{
   u8* p       = (u8*)dest;
   const u8* s = (const u8*)src;
   while(count--)
      *p++ = *s++;
   
   return dest;
}