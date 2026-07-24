#pragma once

typedef char  i8;
typedef short i16;
typedef int   i32;
typedef long  i64;

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;

typedef unsigned char  bool;

#define true  1
#define false 0
#define nil   0

#define kInvalidHandle  -1

#define min(a, b)   ((a) < (b) ? (a) : (b))
#define max(a, b)   ((a) > (b) ? (a) : (b))

#define kLowHighToU64(high, low) (u64)(((u64)high << 16) | low)
