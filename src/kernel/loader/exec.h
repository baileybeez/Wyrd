#pragma once
#include "wyrd.h"
#include "elf.h"
#include "scheduler/thread.h"

typedef struct { 
   const u8* base; 
   u32       len; 
} BufReader;

Thread* execFromDisk(const char* path, ElfError* outError);

#ifdef kIncludeSelfTests
#include "fs/fat16/fat16.h"

void lifecycleSelfTest(const Fat16Volume* vol, const char* path, i32 expectedCode);
#endif