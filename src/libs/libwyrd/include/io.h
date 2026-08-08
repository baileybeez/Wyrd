#include "wyrd.h"

// Format Core support the following: 
// opt   desc                    notes 
// %                             displays literal % char
// c     a single character      8bit
// s     a character string   
// d     signed int              32bit
// i
// u     unsigned int            32bit
// x     unsigned int as Hex     32bit, lower case abcdef
// X     unsigned int as Hex     32bit, upper case ABCDEF
// p     32bit pointer in Hex    0x________

i32 puts(const char* str);
i32 putchar(const char c);
i32 printf(const char* fmt, ...);
