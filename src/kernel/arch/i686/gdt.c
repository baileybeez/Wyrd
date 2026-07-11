#include "bee.h"
#include "gdt.h"

typedef struct  
{
   u16 limit;
   u16 baseLow;
   u8  baseMid;
   u8  access;
   u8  granularity;
   u8  baseHigh;
}__attribute__((packed)) GDTEntry;

typedef struct 
{
   u16 limit;
   u32 ptr;
}__attribute__((packed)) GDTDescriptor;

static GDTEntry      _gdtEntries[kGDTEntryCount];
static GDTDescriptor _gdtDescriptor;

extern void gdtFlush(u32 gdtAddr);

void _setGdtEntry(u32 entry, u32 base, u32 limit, u8 access, u8 granularity);

void gdtInit()
{
   _gdtDescriptor.limit = (sizeof(GDTEntry) * kGDTEntryCount) - 1;
   _gdtDescriptor.ptr   = (u32)&_gdtEntries;

   _setGdtEntry(0, 0, 0,          0,    0);
   _setGdtEntry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // kernel code -- 0b10011010, 0b011001111
   _setGdtEntry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // kernel data -- 0b10010010, 0b011001111
   _setGdtEntry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // user code   -- 0b11111010, 0b011001111
   _setGdtEntry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // user data   -- 0b11110010, 0b011001111

   gdtFlush((u32)&_gdtDescriptor);
}

void _setGdtEntry(u32 entry, u32 base, u32 limit, u8 access, u8 granularity)
{
   _gdtEntries[entry].baseLow  = base & 0xFFFF;
   _gdtEntries[entry].baseMid  = (base >> 16) & 0xFF;
   _gdtEntries[entry].baseHigh = (base >> 24) & 0xFF;

   _gdtEntries[entry].limit       = (limit & 0xFFFF);
   _gdtEntries[entry].granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);

   _gdtEntries[entry].access = access;
}
