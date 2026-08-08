#include "wyrd.h"
#include "args.h"
#include "io.h"
#include "syscall.h"

typedef bool (*EmitFn)(char, void*);

static const char kHexLower[] = "0123456789abcdef";
static const char kHexUpper[] = "0123456789ABCDEF";

i32 putchar(const char c)
{
   return write(kFd_StdOut, &c, 1);
}

i32 puts(const char* str)
{
   u32 n = 0;
   for (const char* c = str; *c; c++) {
      putchar(*c);
      n++;
   }

   putchar('\n');
   return n;
}

static bool _emitToWrite(char c, void* ctx)
{
   kUnused(ctx);
   putchar(c);
   return true;
}

static i32 _emitString(EmitFn emit, const char* str, void* ctx)
{
   i32 len = 0;
   while (*str) {
      if (!emit(*str++, ctx))
         return -1;
      len++;
   }

   return len;
}

static i32 _emitUnsigned(EmitFn emit, u32 u, void* ctx, u32 base, bool upper)
{
   const char* hex = upper ? kHexUpper : kHexLower;
   char buf[32] = {0};
   u32  len = 0;

   if (u == 0) {
      buf[len++] = '0';
   } else {
      while(u > 0) {
         buf[len++] = hex[u % base];
         u /= base;
      }
   }

   u32 ret = len;
   while (len > 0) {
      if (!emit(buf[--len], ctx))
         return -1;
   }
   return ret;
}

static i32 _emitSigned(EmitFn emit, i32 i, void* ctx, u32 base)
{
   i32 len = 0;
   if (i < 0) {
      if (!emit('-', ctx))
         return -1;

      // (u32)0 - (u32)i leverages wrap-around of int to avoid problems with -INT_MIN
      len = _emitUnsigned(emit, (u32)0 - (u32)i, ctx, base, false);
      if (len >= 0)
         len++;
   } else {
      len = _emitUnsigned(emit, (u32)i, ctx, base, false);
   }

   return len;
}

static i32 _formatCore(EmitFn emit, void* ctx, const char* fmt, va_list args)
{
   i32 ret = 0;
   while (*fmt) {
      if (*fmt != '%') {
         if (!emit(*fmt++, ctx))
            return -1;

         ret++;
         continue;
      }
      fmt++;

      // TODO: support for padding and min-width

      switch (*fmt) {
         case 's': {
            const char* s = va_arg(args, const char*);
            i32 len = _emitString(emit, s, ctx);
            if (len < 0)
               return len;
            
            ret += len;
            break;
         }
         case 'c': {
            char c = (char)va_arg(args, int);
            if (!emit(c, ctx))
               return -1;

            ret++;
            break;
         }
         case 'd':
         case 'i': {
            i32 i = va_arg(args, i32);
            i32 len = _emitSigned(emit, i, ctx, 10);
            if (len < 0)
               return len;

            ret += len;
            break;
         }
         case 'u': {
            u32 v = va_arg(args, u32);
            i32 len = _emitUnsigned(emit, v, ctx, 10, false);
            if (len < 0)
               return len;
            
            ret += len;
            break;
         }
         case 'x':
         case 'X': {
            u32 v = va_arg(args, u32);
            i32 len = _emitUnsigned(emit, v, ctx, 16, *fmt == 'X');
            if (len < 0)
               return len;
            
            ret += len;
            break;
         }
         case 'p': {
            emit('0', ctx);
            emit('x', ctx);
            ret += 2;
            u32 v = va_arg(args, u32);
            i32 len = _emitUnsigned(emit, v, ctx, 16, *fmt == 'X');
            if (len < 0)
               return len;

            ret += len;
            break;
         }
         case '\0': 
            break;
         default: {
            if (!emit(*fmt, ctx))
               return -1;

            break;
         }
      }

      if (*fmt)
         fmt++;
   }

   return ret;
}

i32 printf(const char* fmt, ...) 
{
   va_list args;
   va_start(args, fmt);
   i32 ret = _formatCore(_emitToWrite, nil, fmt, args);
   va_end(args);
   return ret;
}
