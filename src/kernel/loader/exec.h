#pragma once
#include "wyrd.h"
#include "fs/fat16/fat16.h"
#include "scheduler/thread.h"

typedef struct { 
   const u8* base; 
   u32       len; 
} BufReader;

Thread* execFromDisk(const Fat16Volume* vol, const char* path);
