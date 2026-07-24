#pragma once
#include "wyrd.h"

void keyboardInit();
char keyboardReadKey();
bool keyboardTryReadKey(char *out);
