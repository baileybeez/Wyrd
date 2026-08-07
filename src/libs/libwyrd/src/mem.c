#include "wyrd.h"

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

int memcmp (const void *ptr1, const void *ptr2, u32 count)
{
   const u8* p1 = ptr1;
   const u8* p2 = ptr2;
   while (count--) {
      if (*p1 != *p2)
         return *p1 < *p2 ? -1 : 1;
      
      ++p1;
      ++p2;
   }

   return 0;
}

void* memmove(void* dest, const void* src, u32 len)
{
   u8* p       = dest;
   const u8* s = src;
   if (p > s) {
      p = (u8*)(dest + len);
      s = (const u8*)(src + len);
      while (len--) {
         *p-- = *s--;
      }
   } else {
      while (len--) {
         *p++ = *s++;
      }
   }

   return dest;
}
