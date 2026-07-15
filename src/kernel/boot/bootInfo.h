#pragma once
#include "bee.h"

#define kMMapEntryType_Available 1
#define kMMapEntryType_Useable   3  // holding ACPI information
#define kMMapEntryType_Reserved  4  // must be preserved for hibernation
#define kMMapEntryType_Defective 5  // defefctive RAM modules
// *NOTE* all other 'types' are defined as Reserved

#define kMaxMemoryMapEntries     32 

typedef struct
{
   u32 baseLow;
   u32 baseHigh;
   u32 lengthLow;
   u32 lengthHigh;
   u32 type;
   u32 reserved;
} MemoryMapEntry;

typedef struct 
{
   u32 totalSystemRam;
   u32 mmapEntryCount;
   MemoryMapEntry mmapEntries[kMaxMemoryMapEntries];
   u32* kernelPhysStart;
   u32* kernelPhysEnd;
} BootInfo;
