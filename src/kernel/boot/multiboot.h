#pragma once
#include "wyrd.h"
#include "bootInfo.h"

#define kMultibootMagic 0x36D76289

#define kMultibootTag_BootCommandLine  1
#define kMultibootTag_BootLoaderName   2
#define kMultibootTag_Modules          3
#define kMultibootTag_BasicMemory      4
#define kMultibootTag_BiosBootDevice   5
#define kMultibootTag_MemoryMap        6
#define kMultibootTag_Elfi386Entry     8
#define kMultibootTag_ElfSymbols       9
#define kMultibootTag_ApmTable         10
#define kMultibootTag_ACPI_RSDP        14
#define kMultibootTag_BasePhysAddy     21

typedef struct {
   u32 tagType;
   u32 tagSize;
} MultibootTag;

typedef struct {
   u32 tagType;
   u32 tagSize;
   char string[];
} MultibootTagString;

typedef struct {
   u32 tagType;
   u32 tagSize;
   u32 entrySize;
   u32 entryVersion;
} MultibootTagMemoryMap;

typedef struct {
   u32 tagType;
   u32 tagSize;
   u32 memLower;
   u32 memUpper;
} MultibootTagBasicMemory;

void multiboot2Parse(u32 infoAddr, BootInfo* out);
