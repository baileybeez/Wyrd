#pragma once
#include "wyrd.h"
#include "sys.h"

i32 write(i32 fd, const char* p, u32 len);
i32 exit(i32 code) kNoReturn;
