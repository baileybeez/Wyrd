#include "bee.h"
#include "kprintf.h"

#include <stdarg.h>

static const char kHexLower[] = "0123456789abcdef";

static void emitString(CharSink sink, const char* s)
{
   if (s == 0)
      s = "(null)";

   while(*s) { 
      sink(*s++);
   }
}

// we divide out a string into an array based on the 'base' passed in, 
// then we can simply output that array in reverse order.
//
//   i.e. u = 256    ==>  ['6', '5', '2']   ==>   sink('2'),sink('5'),sink('6'),
static void emitUnsigned(CharSink sink, u64 u, u32 base, u32 minWidth, char padChar)
{
   char buf[32];
   u32  len = 0;

   if (u == 0) {
      buf[len++] = '0';
   } else {
      while (u > 0) {
         buf[len++] = kHexLower[u % base];
         u /= base;
      }
   }

   while (len < minWidth)
      buf[len++] = padChar;

   while (len > 0)
      sink(buf[--len]);
}

static void emitSigned(CharSink sink, i32 i)
{
   if (i < 0) {
      sink('-');
      emitUnsigned(sink, (u32)-i, 10, 0, ' ');
   } else {
      emitUnsigned(sink, (u32)i, 10, 0, ' ');
   }
}

void kvPrintf(CharSink sink, const char* fmt, va_list args)
{
   while (*fmt) {
      if (*fmt != '%') {
         sink(*fmt++);
         continue;
      }
      fmt++;

      char padChar = ' ';
      u32  width   = 0;
      if (*fmt == '0') {
         padChar = '0';
         fmt++;
      }

      while (*fmt >= '0' && *fmt <= '9') {
         width = (width * 10) + (u32)(*fmt - '0');
         fmt++; 
      }

      switch (*fmt) {
         case 'c': {
            char c = (char)va_arg(args, int);
            sink(c);
            break;
         }
         case 's': {
            const char* s = va_arg(args, const char*);
            emitString(sink, s);
            break;
         }
         case 'd':
         case 'i': {
            i32 v = va_arg(args, i32);
            emitSigned(sink, v);
            break;
         }
         case 'u': {
            u32 v = va_arg(args, u32);
            emitUnsigned(sink, (u64)v, 10, 0, padChar);
            break;
         }
         case 'l': {
            u64 v = va_arg(args, u64);
            emitUnsigned(sink, v, 10, 0, padChar);
            break;
         }
         case 'x': {
            u32 v = va_arg(args, u32);
            emitUnsigned(sink, v, 16, width, padChar);
            break;
         }
         case 'p': {
            u32 v = (u32)va_arg(args, void*);
            sink('0');
            sink('x');
            emitUnsigned(sink, v, 16, 8, '0');
            break;
         }
         case '%': {
            sink('%');
            break;
         }
         default: {
            sink('%');
            sink(*fmt);
            break;
         }
      }

      if (*fmt)
         fmt++;
   }
}