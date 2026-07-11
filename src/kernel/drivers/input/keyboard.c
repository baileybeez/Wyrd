#include "bee.h"
#include "keyboard.h"
#include "../../arch/i686/irq.h"
#include "../../arch/i686/io.h"

extern const char kScancodeToAscii[128];
extern const char kScancodeToAsciiShift[128];

#define kKbDataPort     0x60
#define kKbIrq          1

#define kBufSize        64
#define kBufMask        (kBufSize - 1)

#define kSC_ReleaseBit     0x80
#define kSC_Extended       0xE0
#define kSC_ExtendedPause  0xE1

#define kSC_LShift   0x2A
#define kSC_RShift   0x36
#define kSC_LCtrl    0x1D
#define kSC_LAlt     0x38
#define kSC_CapsLock 0x3A

#define kMod_Shift      0x01
#define kMod_Ctrl       0x02
#define kMod_Alt        0x04
#define kMod_CapsLock   0x08

typedef struct {
   char data[kBufSize];
   u8   head;
   u8   tail;
} KeyBuffer;

static KeyBuffer g_buffer = {0};
static u8 g_modifiers = 0;
static u8 g_extendedPending = 0;
static u8 g_pauseSkip = 0;

static void _kbPush(char c)
{
   u8 next = (g_buffer.head + 1) & kBufMask;
   if (next == g_buffer.tail)
      return;
   
   g_buffer.data[g_buffer.head] = c;
   g_buffer.head = next;
}

static bool _kbPop(char* out)
{
   __asm__ volatile("cli");
   bool ok = false;
   if (g_buffer.head != g_buffer.tail) {
      *out = g_buffer.data[g_buffer.tail];
      g_buffer.tail = (g_buffer.tail + 1) & kBufMask;
      ok = true;
   }
   __asm__ volatile("sti");
   return ok;
}

static char _kbTranslate(u8 code)
{
   bool shift = (g_modifiers & kMod_Shift) != 0;
   bool caps  = (g_modifiers & kMod_CapsLock) != 0;

   char base  = kScancodeToAscii[code];
   char shft  = kScancodeToAsciiShift[code];

   bool isLetter = base >= 'a' && base <= 'z';
   if (isLetter) {
      bool upper = shift ^ caps;
      return upper ? shft : base;
   }

   return shift ? shft : base;
}

static inline void _kbModifierHelper(u8 modFlag, bool released)
{
   if (released)
      g_modifiers &= ~modFlag;
   else 
      g_modifiers |= modFlag;
}

static void keyHandler(Registers* regs)
{
   (void)regs;
   u8 sc = inb(kKbDataPort);

   if (g_pauseSkip > 0) {
      g_pauseSkip--;
      return;
   }

   if (sc == kSC_ExtendedPause) {
      g_pauseSkip = 5;
      return;
   } 

   if (sc == kSC_Extended) {
      g_extendedPending = 1;
      return;
   }

   if (g_extendedPending) {
      g_extendedPending = 0;
      return;
   }

   bool released = (sc & kSC_ReleaseBit) != 0;
   u8 code = sc & 0x7F;
   switch (code)
   {
      case kSC_LShift:
      case kSC_RShift:
         _kbModifierHelper(kMod_Shift, released);
         break;
      case kSC_LCtrl:
         _kbModifierHelper(kMod_Ctrl, released);
         break;
      case kSC_LAlt:
         _kbModifierHelper(kMod_Alt, released);
         break;
      case kSC_CapsLock:
         if (!released)
            g_modifiers ^= kMod_CapsLock;
         break;
   }

   if (released)
      return;

   char c = _kbTranslate(code);
   if (c != 0)
      _kbPush(c);
}

void keyboardInit()
{
   irqRegister(1, keyHandler);
}

bool keyboardTryReadKey(char* out)
{
   return _kbPop(out);
}

char keyboardReadKey()
{
   char c;
   while (!_kbPop(&c)) {
      __asm__ volatile("hlt");
   }
   return c;
}
