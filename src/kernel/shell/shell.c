#include "wyrd.h"
#include "shell.h"
#include "drivers/input/keyboard.h"
#include "drivers/serial/serial.h"
#include "drivers/video/vga.h"
#include "lib/logger.h"
#include "scheduler/scheduler.h"
#include "scheduler/thread.h"

#define kBackspace    '\b'

#define kShellMaxLine 256

static Thread* _thread = nil;

static void _shellEcho(char c)
{
   putChar(c);
   serialWriteChar(c);
   if (c == kBackspace) {
      serialWriteChar(' ');
      serialWriteChar(c);
   }
}

static void _shellReadLine(char outLine[kShellMaxLine], u32* outLen)
{
   u32 len     = 0;
   bool show   = false;
   bool ret    = false;
   KeyEvent ev = {0};
   while (true) {
      ev   = keyboardReadKey();
      show = false;
      if (ev.modifiers & (kMod_Ctrl | kMod_Alt))
         continue;

      if (ev.scanCode == kScanCode_Return) {
         outLine[len] = '\0';
         show = true;
         ret  = true;
      } else if (ev.scanCode == kScanCode_Backspace) {
         if (len > 0) {
            len--;
            show = true;
         }
      } else if (len + 1 < kShellMaxLine) {
         outLine[len++] = ev.ascii;
         show = true;
      }

      if (show) 
         _shellEcho(ev.ascii);
      
      if (ret) {
         *outLen = len;
         return;
      }
   }
}

static void _shellThread(void) 
{
   char line[kShellMaxLine] = {0};
   u32  len = 0;
   kForever {
      print("> ");
      _shellReadLine(line, &len);
      print(line);
      putChar('\n');
   }
}

bool shellInit(void)
{
   _thread = threadCreate(_shellThread);
   return _thread != nil;
}
