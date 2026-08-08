#include "wyrd.h"

// counts the number of characters in a null terminated string
u32 strlen(const char* str);u32 strlen(const char* str)
{
   const char* p = str;
   u32 len = 0;
   while (*p++ != '\0')
      len++;

   return len;
}

// compares characters of two null terminated strings
// return 0 if equal, otherwise 1 or -1 based on failed compare
u32 strcmp(const char* a, const char* b)
{
   const char* p1 = a;
   const char* p2 = b;

   while(*p1 != '\0' && *p2 != '\0') {
      if (*p1 != *p2)
         return *p1 < *p2 ? -1 : 1;
      
      ++p1;
      ++p2;
   }

   if (*p1 == *p2)
      return 0;
      
   return *p1 < *p2 ? -1 : 1;
}

// compares characters of two null terminated strings up to a max of COUNT chracters
// return 0 if equal, otherwise 1 or -1 based on failed compare
u32 strncmp(const char* a, const char* b, u32 count)
{
   u32         n  = 0;
   const char* p1 = a;
   const char* p2 = b;

   while(*p1 != '\0' && *p2 != '\0' && n < count) {
      if (*p1 != *p2)
         return *p1 < *p2 ? -1 : 1;
      
      ++p1;
      ++p2;
      ++n;
   }

   if (*p1 == *p2 || n >= count)
      return 0;
      
   return *p1 < *p2 ? -1 : 1;
}

// copies a null-term string from SRC to DST (includes null-term)
char* strcpy(char* dst, const char* src)
{
   char* p = dst;
   const char* s = src;
   while (*s) {
      *p++ = *s++;
   }
   *p = *s;
   return dst;
}

// copies at most COUNT characters of a null-term string 
// from SRC to DST (includes null-term)
// if SRC copy ends before count is reached, nulls are added up to COUNT 
// if COUNT is reached before SRC is copied, DST will NOT be null terminated
char* strncpy(char* dst, const char* src, u32 count)
{
   u32   n = 0;
   char* p = dst;
   const char* s = src;
   while (*s && n < count) {
      *p++ = *s++;
      n++;
   }

   while (n < count) {
      *p++ = '\0';
      n++;
   }

   return dst;
}

// find the first instance of char C within STR
// returns pointer to the first instance, nil if not found
const char* strchr(const char* str, char c)
{
   const char* p = str;
   while (*p) {
      if (*p == c)
         return p;

      p++;
   }

   return nil;
}

// appends SRC to DST, replacing the null-term of DST with
// the fisrt char in SRC. the resulting string is null-term
// returns a pointer to the new string
char* strcat(char* dst, const char* src)
{
   char* p = dst;
   while (*p)
      p++;

   const char* s = src;
   while (*s) {
      *p++ = *s++;
   }

   *p = nil;
   return dst;
}
