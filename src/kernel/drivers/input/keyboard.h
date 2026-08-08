#pragma once
#include "wyrd.h"

#define kScanCode_Return      28
#define kScanCode_Backspace   14

#define kMod_Shift      0x01
#define kMod_Ctrl       0x02
#define kMod_Alt        0x04
#define kMod_CapsLock   0x08

typedef struct {
   u8 scanCode;
   u8 ascii;
   u8 modifiers;
} KeyEvent;

void     keyboardInit();
void     keyboardClearMask();
KeyEvent keyboardReadKey();
bool     keyboardTryReadKey(KeyEvent *ev);
