#include "wyrd.h"
#include "keyboard.h"
#include "arch/i686/cpu.h"
#include "arch/i686/irq.h"
#include "arch/i686/io.h"
#include "arch/i686/pic.h"
#include "lib/assert.h"
#include "scheduler/scheduler.h"

extern const char kScancodeToAscii[128];
extern const char kScancodeToAsciiShift[128];

#define kKbDataPort     0x60
#define kKbStatusPort   0x64
#define kKbIrq          1

#define kKbStatus_OutputFull  0x01
#define kKbDrainLimit         16

#define kBufSize        64
#define kBufMask        (kBufSize - 1)

#define kSC_ReleaseBit     0x80
#define kSC_Extended       0xE0
#define kSC_ExtendedPause  0xE1

#define kSC_LShift      0x2A
#define kSC_RShift      0x36
#define kSC_LCtrl       0x1D
#define kSC_LAlt        0x38
#define kSC_CapsLock    0x3A

#define kMod_Shift      0x01
#define kMod_Ctrl       0x02
#define kMod_Alt        0x04
#define kMod_CapsLock   0x08

typedef struct {
   KeyEvent    data[kBufSize];
   volatile u8 head;
   volatile u8 tail;
} KeyBuffer;

static KeyBuffer _buffer       = {0};
static WaitQueue _keyWaitQueue = {0};

static u8 _modifiers           = 0;
static u8 _extendedPending     = 0;
static u8 _pauseSkip           = 0;

static bool _kbPush(KeyEvent ev)
{
   u8 next = (_buffer.head + 1) & kBufMask;
   if (next == _buffer.tail)
      return false;
   
   _buffer.data[_buffer.head] = ev;
   _buffer.head = next;
   return true;
}

static bool _kbPop(KeyEvent* out)
{
   kAssert((readEflags() & kEflags_IF) == 0);

   bool ok = false;
   if (_buffer.head != _buffer.tail) {
      *out = _buffer.data[_buffer.tail];
      _buffer.tail = (_buffer.tail + 1) & kBufMask;
      ok = true;
   }
 
   return ok;
}

static KeyEvent _kbTranslate(u8 code)
{
   bool shift = (_modifiers & kMod_Shift) != 0;
   bool caps  = (_modifiers & kMod_CapsLock) != 0;

   KeyEvent ev = {0};
   ev.scanCode = code;
   ev.modifiers = _modifiers;

   char base  = kScancodeToAscii[code];
   char shft  = kScancodeToAsciiShift[code];
   
   bool isLetter = base >= 'a' && base <= 'z';
   if (isLetter) {
      bool upper = shift ^ caps;
      ev.ascii = upper ? shft : base;
   } else {
      ev.ascii = shift ? shft : base;
   }

   return ev;
}

static inline void _kbModifierHelper(u8 modFlag, bool released)
{
   if (released)
      _modifiers &= ~modFlag;
   else 
      _modifiers |= modFlag;
}

static void _keyHandler(Registers* regs)
{
   kUnused(regs);
   u8 sc = inb(kKbDataPort);

   if (_pauseSkip > 0) {
      _pauseSkip--;
      return;
   }

   if (sc == kSC_ExtendedPause) {
      _pauseSkip = 5;
      return;
   } 

   if (sc == kSC_Extended) {
      _extendedPending = 1;
      return;
   }

   if (_extendedPending) {
      _extendedPending = 0;
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
            _modifiers ^= kMod_CapsLock;
         break;
   }

   if (released)
      return;

   KeyEvent ev = _kbTranslate(code);
   if (ev.ascii != 0 && _kbPush(ev))
      schedulerWakeOne(&_keyWaitQueue);
}

// A byte left in the 8042 output buffer by BIOS suppresses every further
// IRQ1 until it is read, so drain before unmasking.
static void _kbDrain()
{
   for (u32 i = 0; i < kKbDrainLimit; i++) {
      if ((inb(kKbStatusPort) & kKbStatus_OutputFull) == 0)
         return;
 
      inb(kKbDataPort);
   }
}

void keyboardInit()
{
   irqRegister(kKbIrq, _keyHandler);
   _kbDrain();
}

void keyboardClearMask()
{
   picClearMask(kKbIrq);
}

bool keyboardTryReadKey(KeyEvent* ev)
{
   u32 flags = irqSave();
   bool ret  = _kbPop(ev);
   irqRestore(flags);

   return ret;
}

KeyEvent keyboardReadKey()
{
   u32 flags = irqSave();
   while (_buffer.head == _buffer.tail) {
      schedulerBlockCurrent(&_keyWaitQueue);
   }

   KeyEvent ev;
   bool ok = _kbPop(&ev);
   kAssert(ok);
   kUnused(ok);

   irqRestore(flags);
   return ev;
}
