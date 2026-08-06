#include "wyrd.h"
#include "usercopy.h"
#include "memory.h"
#include "arch/i686/paging.h"
#include "lib/mem.h"

bool userBufferIsValid(u32 ptr, u32 len, bool needWrite)
{   
   if (len == 0)
      return true;

   if (ptr >= kKernelVirtualBase)
      return false;

   if (len > kKernelVirtualBase - ptr)
      return false;

   u32 last = (ptr + len - 1) & ~0xFFF;
   for (u32 page = ptr & ~0xFFF; page <= last; page += kPageSize) {
      if (!pagingIsUserAccessible(page, needWrite))
         return false;
   }

   return true;
}

bool userCopyIn(void* dest, u32 userSrc, u32 len)
{
   if (!userBufferIsValid(userSrc, len, false))
      return false;

   memcpy(dest, (const void*)userSrc, len);
   return true;
}

bool userCopyOut(u32 userDest, const void* src, u32 len)
{
   if (!userBufferIsValid(userDest, len, true))
      return false;

   memcpy((void*)userDest, src, len);
   return true;
}

i32 userStringLength(u32 ptr, u32 maxLen)
{
   if (ptr >= kKernelVirtualBase)
      return kUserString_Fault;

   if (maxLen > kKernelVirtualBase - ptr)
      maxLen = kKernelVirtualBase - ptr;

   if (!pagingIsUserAccessible(ptr & ~0xFFF, false))
      return kUserString_Fault;

   for (u32 len = 0; len < maxLen; len++) {
      u32 addr = ptr + len;
      if (len != 0 && (addr & (kPageSize - 1)) == 0) {
         if (!pagingIsUserAccessible(addr, false))
            return kUserString_Fault;
      }

      if (*(const u8*)addr == 0)
         return (i32)len;
   }

   return kUserString_TooLong;
}
