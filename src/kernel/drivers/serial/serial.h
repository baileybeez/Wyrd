#pragma once

bool serialInit();
void serialWriteString(const char* s);
void serialPrintf(const char* fmt, ...);
