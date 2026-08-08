#include "wyrd.h"
#include "args.h"
#include "arch/i686/cpu.h"
#include "arch/i686/io.h"
#include "lib/kprintf.h"
#include "serial.h"

#ifdef kSerialTrace
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

static void _serialWriteCharInternal(char c)
{
   if (c == '\n' && _convertLFtoCRLF)
      _writeByteRaw('\r');

   _writeByteRaw(c);
}
#endif

bool serialInit(void)
{
#ifdef kSerialTrace
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
#endif
   return true;
}

void serialWriteChar(char c)
{
#ifdef kSerialTrace
   u32 flags = irqSave();
   _serialWriteCharInternal(c);
   irqRestore(flags);
#else
   kUnused(c);
#endif
}

void serialWrite(const char* buf, u32 len)
{
#ifdef kSerialTrace
   u32 flags = irqSave();
   for (u32 i = 0; i < len ; i++)
      _serialWriteCharInternal(buf[i]);
   irqRestore(flags);
#else 
   kUnused(buf);
   kUnused(len);
#endif
}

void serialPrintf(const char* fmt, ...)
{
#ifdef kSerialTrace
   va_list args;
   va_start(args, fmt);
   serialVprintf(fmt, args);
   va_end(args);
#else
   kUnused(fmt);
#endif
}

void serialVprintf(const char* fmt, va_list args)
{
#ifdef kSerialTrace
   u32 flags = irqSave();
   kvPrintf(_serialWriteCharInternal, fmt, args);
   irqRestore(flags);
#else
   kUnused(fmt);
   kUnused(args);
#endif
}
