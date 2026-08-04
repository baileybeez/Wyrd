#pragma once
#include "wyrd.h"

typedef struct {
   u8 scanCode;
   u8 ascii;
   u8 modifiers;
} KeyEvent;

void     keyboardInit();
void     keyboardClearMask();
KeyEvent keyboardReadKey();
bool     keyboardTryReadKey(KeyEvent *ev);
