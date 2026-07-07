#include "bee.h"
#include "vga.h"
#include "../../lib/kprintf.h"
#include <stdarg.h>

const u16 kVGA_DefaultClearColor = (kColor_Black << 12) | (kColor_LightGray << 8);
VGA g_vga = {0};

void _vgaScrollUp();
void _vgaLineFeed();
void _vgaPutch(char cb);

void vgaInit()
{
   g_vga.mem = kVideoMem;
   g_vga.row = 0;
   g_vga.col = 0;
   g_vga.color = kVGA_DefaultClearColor;
   
   for (u16 y = 0; y < kVideoHeight; y++) {
      for (u16 x = 0; x < kVideoWidth; x++) {
         g_vga.mem[y * kVideoWidth + x] = ' ' | g_vga.color;
      }
   }
}

void vgaSetColor(u8 fg, u8 bg)
{
   g_vga.color = (bg << 12) | (fg << 8);
}

void print(const char *s)
{
   while (*s) {
      switch (*s) 
      {
         case '\n': 
            _vgaLineFeed();
            break;
         case '\r':
            g_vga.col = 0;
            break;
         default: 
            if (g_vga.col >= kVideoWidth)
               _vgaLineFeed();
            
            _vgaPutch(*s);
            break;
      };
      s++;
   }
}

void printf(const char* fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   kvPrintf(_vgaPutch, fmt, args);
   va_end(args);
}

void _vgaPutch(char cb)
{
   g_vga.mem[g_vga.row * kVideoWidth + g_vga.col++] = (u8)cb | g_vga.color;
}

void _vgaScrollUp()
{
   u16 *mem = g_vga.mem;
   for (u16 y = 1; y < kVideoHeight; y++) {
        for (u16 x = 0; x < kVideoWidth; x++) {
            mem[(y - 1) * kVideoWidth + x] = mem[y * kVideoWidth + x];
        }
    }

    for (u16 x = 0; x < kVideoWidth; x++) {
        mem[(kVideoHeight- 1) * kVideoWidth + x] = ' ' | g_vga.color;
    }
}

void _vgaLineFeed()
{
   if (g_vga.row + 1 < kVideoHeight)
        g_vga.row++;
    else 
        _vgaScrollUp();

    g_vga.col = 0;
}
