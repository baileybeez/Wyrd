#pragma once
#include "wyrd.h"

u32 strlen(const char* str);
u32 strcmp(const char* a, const char* b);
u32 strncmp(const char* a, const char* b, u32 count);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, u32 count);
const char* strchr(const char* str, char c);
char* strcat( char* dest, const char* src );
