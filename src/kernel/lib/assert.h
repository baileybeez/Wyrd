#pragma once
#include "panic.h"

#define kStringifyRaw(x)   #x
#define kStringify(x)      kStringifyRaw(x)

#define kAssert(cond)                                                \
   do {                                                              \
      if (!(cond))                                                   \
         kernelPanic("assert failed: " #cond                         \
                     " (" __FILE__ ":" kStringify(__LINE__) ")");    \
   } while (0)
