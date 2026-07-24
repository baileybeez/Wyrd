#include "wyrd.h"
#include "lib/logger.h"
#include "mm/memory.h"
#include "multiboot.h"
#include "bootInfo.h"

extern u32 _kernelStart;
extern u32 _kernelPhysicalEnd;


void multiboot2Parse(u32 infoAddr, BootInfo* out)
{
   out->kernelPhysStart = &_kernelStart;
   out->kernelPhysEnd   = &_kernelPhysicalEnd;

   u32 infoBase = physToVirtual(infoAddr);

   u32 totalSize = *(u32*)(infoBase + 0);
   u32 reserved  = *(u32*)(infoBase + 4);
   kTrace(" Multiboot Info (total size: %u, resv: %u)", totalSize, reserved);
   
   MultibootTagMemoryMap* mmap = nil;
   MultibootTagString* str     = nil;
   MultibootTag* tag = (MultibootTag*)(infoBase + 8);
   while (tag->tagType != 0)
   {
      kTrace(":: Tag   type: %u, size: %u", tag->tagType, tag->tagSize);
      switch (tag->tagType)
      {
         case kMultibootTag_BootCommandLine:
            str = (MultibootTagString*)tag;
            kTrace("boot command line: '%s'", str->string);
            break;            
         case kMultibootTag_BootLoaderName:
            str = (MultibootTagString*)tag;
            kTrace("boot loader name: '%s'", str->string);
            break;
         case kMultibootTag_Modules:
            kTrace("modules");
            break;
         case kMultibootTag_BasicMemory:
            {
               MultibootTagBasicMemory* basicMem = (MultibootTagBasicMemory*)tag;
               kTrace("basic memory: %u, %u", basicMem->memLower,basicMem->memUpper);
            }
            break;
         case kMultibootTag_BiosBootDevice:
            kTrace("bios boot device");
            break;
         case kMultibootTag_MemoryMap:
            {
               mmap = (MultibootTagMemoryMap*)tag;
               kTrace("MMAP size: %u, ver: %u", mmap->entrySize, mmap->entryVersion);
            
               u8 headerSize = sizeof(MultibootTagMemoryMap);
               u8* addr = (u8*)tag + headerSize;
               u32 highestAddr = 0;
               u64 end = 0;

               u32 entryCount = (mmap->tagSize - headerSize) / mmap->entrySize;

               out->mmapEntryCount = min(entryCount, kMaxMemoryMapEntries);
               for (u32 i = 0; i < entryCount; i++) {
                  MemoryMapEntry* entry = (MemoryMapEntry*)addr;
                  u64 baseAddr = kLowHighToU64(entry->baseHigh, entry->baseLow);
                  u64 length   = kLowHighToU64(entry->lengthHigh, entry->lengthLow);
                  if (i < kMaxMemoryMapEntries)
                     out->mmapEntries[i] = *entry;
                     
                  end = baseAddr + length;
                  if (end > 0xFFFFFFFF)
                     end = 0xFFFFFFFF;
                  if ((u32)end > highestAddr)
                     highestAddr = (u32)end;

                  if (entry->type == kMMapEntryType_Available)
                     kTrace("[FREE ] addr: %p, len: %u, type: %u", baseAddr, length, entry->type);
                  else 
                     kTrace("[INUSE] addr: %p, len: %u, type: %u", baseAddr, length, entry->type);
                  addr += mmap->entrySize;
               }
               out->totalSystemRam = highestAddr;
            }
            break;
         case kMultibootTag_Elfi386Entry:
            kTrace("tag type: ELF i386 Entry Address");
            break;
         case kMultibootTag_ElfSymbols:
            kTrace("tag type: ELF Symbols");
            break;
         case kMultibootTag_ApmTable:
            kTrace("tag type: APM table");
            break;
         case kMultibootTag_ACPI_RSDP:
            kTrace("tag type: ACPI copy of RSDP");
            break;
         case kMultibootTag_BasePhysAddy:
            kTrace("tag type: base physical image address");
            break;
         default:
            kTrace("tag type: UNKNOWN");
            break;
      }

      tag = (MultibootTag*)((u8*)tag + ((tag->tagSize + 7) & ~7));
      if ((u32)tag >= infoBase + totalSize) { 
         kWarn("walked past MBInfo end without reading terminator");
         break;
      }
   }

   kTrace("----------------\n");
}
