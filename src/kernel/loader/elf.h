#pragma once
#include "wyrd.h"
#include "arch/i686/paging.h"

typedef enum {
   kElfErr_OK                 = 0,
   kElfErr_TooSmall           = 1,
   kElfErr_BadMagic           = 2,
   kElfErr_Not32bit           = 3,
   kElfErr_NotExec            = 4,
   kElfErr_BadMachine         = 5,
   kElfErr_BadProgramHeader   = 6,
   kElfErr_SegOutOfRange      = 7,
   kElfErr_ReadFailed         = 8,
   kElfErr_NoMemory           = 9, 
   kElfErr_BadEndian          = 10,
   kElfErr_BadEntry           = 11,
   kElfErr_BadArgument        = 12
} ElfError;

typedef struct {
   u8  ident[16];
   u16 type;
   u16 machine;
   u32 version;
   u32 entry;
   u32 phOffset;     // program header offset
   u32 shOffset;     // section header offset
   u32 flags;
   u16 headerSize;   // this header's size
   u16 phSize;       // program header's entry size
   u16 phCount;      // program header entry count
   u16 shSize;       // section header's entry size
   u16 shCount;      // section header entry count
} __attribute__((packed)) ELF32Header;

typedef struct {
   u32 type;
   u32 offset;       // segments offset
   u32 virtAddr;     // virtual address of segment
   u32 physAddr;     // physical address of segment
   u32 physSegSize;
   u32 memSegSize;
   u32 flags;
   u32 align;
} __attribute__((packed)) ElfProgramHeader;

typedef struct {
   u32 name;         // offset to a string in the .shstrtab section
   u32 type;
   u32 flags;
   u32 virtAddr;
   u32 offset;
   u32 size;
   u32 link;
   u32 info;
   u32 align;
   u32 entrySize;
} __attribute__((packed)) ElfSectionHeader;

typedef ElfError (*ElfReadFn)(const void* ctx, u32 offset, u32 len, void* dst);
ElfError elfLoad(ElfReadFn read, const void* context, u32 imageLen, AddressSpace* space, u32* outEntry);

#ifdef kIncludeSelfTests
bool elfSelfTest();
#endif
