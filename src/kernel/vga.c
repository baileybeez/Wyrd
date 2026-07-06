#include "bee.h"
#include "vga.h"

const u16 kVGA_DefaultClearColor = (kColor_Black << 12) | (kColor_LightGray << 8);
VGA g_vga = {0};

void _vga_scrollUp();
void _vga_lineFeed();
void _vga_putch(u8 cb);

void vga_init()
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

void print(const char *s)
{
   while (*s) {
      switch (*s) 
      {
         case '\n': 
            _vga_lineFeed();
            break;
         case '\r':
            g_vga.col = 0;
            break;
         default: 
            if (g_vga.col >= kVideoWidth)
               _vga_lineFeed();
            
            _vga_putch((u8)*s);
            break;
      };
      s++;
   }
}

void _vga_putch(u8 cb)
{
   g_vga.mem[g_vga.row * kVideoWidth + g_vga.col++] = cb | g_vga.color;
}

void _vga_scrollUp()
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

void _vga_lineFeed()
{
   if (g_vga.row + 1 < kVideoHeight)
        g_vga.row++;
    else 
        _vga_scrollUp();

    g_vga.col = 0;
}
