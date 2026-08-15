#include "wyrd.h"
#include "args.h"
#include "vga.h"
#include "lib/kprintf.h"

#define kLineFeed       '\n'
#define kCarriageReturn '\r'
#define kBackspace      '\b'
#define kTab            '\t'

const u16 kVGA_DefaultClearColor = (kColor_Black << 12) | (kColor_LightGray << 8);

VGA _vga = {0};

void _vgaScrollUp(void);
void _vgaLineFeed(void);

void vgaInit(void)
{
   _vga.mem = kVideoMem;
   _vga.row = 0;
   _vga.col = 0;
   _vga.color = kVGA_DefaultClearColor;
   
   vgaClear();
}

void vgaClear(void)
{
   for (u16 y = 0; y < kVideoHeight; y++) {
      for (u16 x = 0; x < kVideoWidth; x++) {
         _vga.mem[y * kVideoWidth + x] = ' ' | _vga.color;
      }
   }

   _vga.row = 0;
   _vga.col = 0;
}

void vgaSetColor(u8 fg, u8 bg)
{
   _vga.color = (bg << 12) | (fg << 8);
}

void print(const char *s)
{
   while (*s) {
      putChar(*s++);
   }
}

void printn(const char* s, u32 len)
{
   for (u32 i = 0; i < len ; i++)
      putChar(s[i]);
}

void printf(const char* fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   vprintf(fmt, args);
   va_end(args);
}

void vprintf(const char* fmt, va_list args)
{
   kvPrintf(putChar, fmt, args);
}

void putChar(char cb)
{
   switch (cb)
   {
      case kLineFeed: 
         _vgaLineFeed();
         break;
      case kCarriageReturn:
         _vga.col = 0;
         break;
      case kBackspace:
         if (_vga.col > 0) {
            _vga.col--;
            _vga.mem[_vga.row * kVideoWidth + _vga.col] = (u8)' ' | _vga.color;
         }
         break;
      case kTab: {
            if (_vga.col % 5 == 0)
               _vga.col += 5;
            else 
               _vga.col = (_vga.col + 4) / 5 * 5;
               
            if (_vga.col >= kVideoWidth)
               _vgaLineFeed();
         }
         break;
      default:
         if (_vga.col >= kVideoWidth)
            _vgaLineFeed();

         _vga.mem[_vga.row * kVideoWidth + _vga.col++] = (u8)cb | _vga.color;
         break;
   }
}

void _vgaScrollUp()
{
   u16 *mem = _vga.mem;
   for (u16 y = 1; y < kVideoHeight; y++) {
        for (u16 x = 0; x < kVideoWidth; x++) {
            mem[(y - 1) * kVideoWidth + x] = mem[y * kVideoWidth + x];
        }
    }

    for (u16 x = 0; x < kVideoWidth; x++) {
        mem[(kVideoHeight- 1) * kVideoWidth + x] = ' ' | _vga.color;
    }
}

void _vgaLineFeed()
{
   if (_vga.row + 1 < kVideoHeight)
        _vga.row++;
    else 
        _vgaScrollUp();

    _vga.col = 0;
}
