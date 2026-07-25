#include "wyrd.h"
#include "arch/i686/io.h"
#include "lib/kprintf.h"
#include "serial.h"

#include <stdarg.h>

static const u16 kCom1Port       = 0x3F8;
static const u8  kLineStatusTHR  = 0x20;
static const u8  kLoopbackByte   = 0xAE;

static bool _convertLFtoCRLF = true;

static bool _isTransmitEmpty()
{
   return (inb(kCom1Port + 5) & kLineStatusTHR) != 0;
}

static void _writeByteRaw(char c)
{
   while (!_isTransmitEmpty()) { }
   outb(kCom1Port, (u8)c);
}

bool serialInit()
{
   outb(kCom1Port + 1, 0x00);
   outb(kCom1Port + 3, 0x80);
   outb(kCom1Port + 0, 0x03);
   outb(kCom1Port + 1, 0x00);
   outb(kCom1Port + 3, 0x03);
   outb(kCom1Port + 2, 0xC7);
   outb(kCom1Port + 4, 0x0B);
 
   outb(kCom1Port + 4, 0x1E);
   outb(kCom1Port + 0, kLoopbackByte);
   if (inb(kCom1Port) != kLoopbackByte)
      return false;
   
   outb(kCom1Port + 4, 0x0F);
   return true;
}

void serialWriteChar(char c)
{
   if (c == '\n' && _convertLFtoCRLF)
      _writeByteRaw('\r');

   _writeByteRaw(c);
}

void serialWriteString(const char* s)
{
   while (*s) {
      serialWriteChar(*s++);
   }
}

void serialWrite(const char* buf, u32 len)
{
   for (u32 i = 0; i < len ; i++)
      serialWriteChar(buf[i]);
}

void serialPrintf(const char* fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   serialVprintf(fmt, args);
   va_end(args);
}

void serialVprintf(const char* fmt, va_list args)
{
   kvPrintf(serialWriteChar, fmt, args);
}
