#pragma once
#include "bee.h"

void keyboardInit();
char keyboardReadKey();
bool keyboardTryReadKey(char *out);
