#include "wyrd.h"
#include "paging.h"
#include "cpu.h"
#include "lib/logger.h"
#include "lib/mem.h"
#include "lib/panic.h"
#include "mm/heap.h"
#include "mm/memory.h"
#include "mm/pmm.h"
#include "isr.h"

#define kPagingDirectory      0xFFFFF000
#define kPagingTableRoot      0xFFC00000

#define kPageDirEntries       1024
#define kPageTableEntries     1024

#define kScratchVirtualStart  0xE0000000
#define kScratchDirectory     (kScratchVirtualStart)
#define kScratchTable         (kScratchVirtualStart + kPageSize)
#define kScratchVirtualLimit  (kScratchVirtualStart + (2 * kPageSize))

#define kKernelPdeFirst       (kKernelVirtualBase >> 22)
#define kRecursivePde         (kPageDirEntries - 1)

static u32 g_kernelDirectory[kPageDirEntries]   __attribute__((aligned(4096)));
static u32 g_kernelTable[kPageTableEntries]     __attribute__((aligned(4096)));

static AddressSpace  g_bootSpace;
static AddressSpace* g_currentSpace = &g_bootSpace;

static bool          g_kernelPDESealed = false;

static inline void _pagingLoadDirectory(u32 physicalDir) 
{
   __asm__ volatile("mov %0, %%cr3" :: "r"(physicalDir) : "memory");
}

static inline u32 _pagingReadFaultAddress(void)   // CR2, for the fault handler
{
   u32 addr;
   __asm__ volatile("mov %%cr2, %0" : "=r"(addr));
   return addr;
}

static inline u32* _pagingDirectory() 
{ 
   return (u32*)kPagingDirectory;
}

static u32* _pagingTable(u32 dirIndex)
{
   return (u32*)(kPagingTableRoot + (dirIndex * kPageSize));
}

static u32 _pagingLookupEntry(u32 virtualAddr)
{
   u32 dirIndex   = virtualAddr >> 22;
   u32 tableIndex = (virtualAddr >> 12) & 0x3FF;

   u32* dir = _pagingDirectory();
   if (!(dir[dirIndex] & kPageFlag_Present))
      return 0;
   
   return _pagingTable(dirIndex)[tableIndex];
}

static bool _pagingEnsureTable(u32 dirIndex, u32 flags)
{
   u32 userBit = flags & kPageFlag_User;
   u32* dir    = _pagingDirectory();
   if (dir[dirIndex] & kPageFlag_Present) {
      if (userBit && !(dir[dirIndex] & kPageFlag_User))
         dir[dirIndex] |= kPageFlag_User;

      return true;
   }

   if (g_kernelPDESealed && dirIndex >= kKernelPdeFirst)
      kernelPanic("paging: kernel PDE %u not preallocated", dirIndex);

   u32 frame = pmmAllocFrame();
   if (frame == kInvalidFrame)
      return false;

   dir[dirIndex] = frame | kPageFlag_Present | kPageFlag_Writable | userBit;
   kTrace("mapping new PDE :: %x", dirIndex);
   u32* table = _pagingTable(dirIndex);
   pagingInvalidatePage((u32)table);
   memset(table, 0x00, kFrameSize);
   return true;
}

static void _pagingFaultHandler(Registers* regs)
{
   u32 faultAddr = _pagingReadFaultAddress();
   kError("PAGE FAULT at %x  errorCode=%x", faultAddr, regs->errCode);
   kError("  %s, %s, %s",
      (regs->errCode & 0x1) ? "protection" : "not-present",
      (regs->errCode & 0x2) ? "write" : "read",
      (regs->errCode & 0x4) ? "user" : "kernel");
   kHalt();
}

static u32* _pagingScratchMap(u32 scratchVa, u32 physicalFrame)
{
   if (!pagingMapPage(scratchVa, physicalFrame, kPageFlag_Writable))
      kernelPanic("paging: unable to map page for scratch");

   return (u32*)scratchVa;
}

static void _pagingScratchUnmap(u32 scratchVa)
{
   pagingUnmapPage(scratchVa);
}

void pagingInit()
{
   memset(g_kernelDirectory, 0x00, kPageDirEntries * sizeof(u32));
   for (u32 i = 0; i < kPageTableEntries; i++)
      g_kernelTable[i] = (i * kPageSize) | kPageFlag_Present | kPageFlag_Writable;

   u32 kernelDir = virtualToPhys((u32)g_kernelDirectory);
   g_kernelDirectory[kKernelPdeFirst] = virtualToPhys((u32)g_kernelTable) | kPageFlag_Present | kPageFlag_Writable;
   g_kernelDirectory[kRecursivePde]   = kernelDir | kPageFlag_Present | kPageFlag_Writable;
   g_bootSpace.physicalDirectory = kernelDir;

   kTrace("kernelDir phys = %x", kernelDir);
   kTrace("kKernelPdeFirst is %u", kKernelPdeFirst);
   kTrace("kRecursivePde is %u", kRecursivePde);
   kTrace("PDE[%u] (kernDir) = %x, PDE[%u] (recursive) = %x   (kernel should place 0x1000 ahead of recursive)", 
         kKernelPdeFirst, g_kernelDirectory[kKernelPdeFirst], kRecursivePde, g_kernelDirectory[kRecursivePde]);
   isrRegister(14, _pagingFaultHandler);
   _pagingLoadDirectory(kernelDir);
   kTrace("survived CR3 load");

   if (!pagingReserveRange(kScratchVirtualStart, kScratchVirtualLimit))
      kernelPanic("pagingInit: unable to reserve scratch range");
}

void pagingSealKernelPDEs()
{
   kTrace("paging: sealing the kernel PDEs");
   g_kernelPDESealed = true;
}

AddressSpace* pagingBootSpace()
{
   return &g_bootSpace;
}

// reserve all pages between start and end virtual addresses
bool pagingReserveRange(u32 virtualStart, u32 virtualEnd)
{
   u32 pdeStart = virtualStart >> 22;
   u32 pdeEnd   = (virtualEnd + 0x3FFFFF) >> 22;   // round up to the next page boundary

   kTrace("paging: reserving %u PDEs from %x to %x", (pdeEnd - pdeStart), pdeStart, pdeEnd);
   for (u32 pde = pdeStart; pde < pdeEnd; pde++) {
      if (!_pagingEnsureTable(pde, kPageFlag_None))
         return false;
   }

   return true;
}

bool pagingIsMapped(u32 virtualAddr)
{
   return (_pagingLookupEntry(virtualAddr) & kPageFlag_Present) != 0;
}

bool pagingMapPage(u32 virtualAddr, u32 physicalAddr, u32 flags)
{   
   u32 dirIndex   = virtualAddr >> 22;
   u32 tableIndex = (virtualAddr >> 12) & 0x3FF;
   
   if (!_pagingEnsureTable(dirIndex, flags))
      return false;

   u32* tbl = _pagingTable(dirIndex);
   tbl[tableIndex] = (physicalAddr & ~0xFFF) | flags | kPageFlag_Present;
   pagingInvalidatePage(virtualAddr);
   return true;
}

bool pagingUnmapPage(u32 virtualAddr)
{
   u32 dirIndex   = virtualAddr >> 22;
   u32 tableIndex = (virtualAddr >> 12) & 0x3FF;
   
   u32* dir = _pagingDirectory();
   if (!(dir[dirIndex] & kPageFlag_Present))
      return false;

   u32* tbl = _pagingTable(dirIndex);
   if (!(tbl[tableIndex] & kPageFlag_Present))
      return false;

   tbl[tableIndex] = 0;
   pagingInvalidatePage(virtualAddr);
   if (dirIndex >= kKernelPdeFirst)
      return true;
   
   // TODO: release table if empty (deferred #6)
   return true;
}

u32  pagingGetPhysical(u32 virtualAddr)
{
   u32 offset = virtualAddr & 0xFFF;
   u32 entry  = _pagingLookupEntry(virtualAddr);
   if (!(entry & kPageFlag_Present))
      return kInvalidPhysical;

   return (entry & ~0xFFF) | offset;
}

void pagingInvalidatePage(u32 virtualAddr)
{
   __asm__ volatile("invlpg (%0)" :: "r"(virtualAddr) : "memory");
}

bool pagingIsUserAccessible(u32 addr, bool needsWrite)
{
   u32 required = kPageFlag_Present | kPageFlag_User;
   if (needsWrite)
      required |= kPageFlag_Writable;

   u32 dirIndex = addr >> 22;
   u32 pde      = _pagingDirectory()[dirIndex];
   if ((pde & required) != required)
      return false;

   u32 tableIndex = (addr >> 12) & 0x3FF;
   u32 pte = _pagingTable(dirIndex)[tableIndex];
   if ((pte & required) != required)
      return false;
   
   return true;
}

AddressSpace* addressSpaceCreate()
{
   AddressSpace* space = kmalloc(sizeof(AddressSpace));
   if (space == nil)
      return nil;

   u32 frame = pmmAllocFrame();
   if (frame == kInvalidFrame) {
      kfree(space);
      return nil;
   }

   space->physicalDirectory = frame;
   u32 flags      = irqSave();
   u32* dir       = _pagingScratchMap(kScratchDirectory, frame);
   u32* kernelDir = _pagingDirectory();

   memset(dir, 0x00, kFrameSize);
   for (u32 i = kKernelPdeFirst; i < kRecursivePde; i++)
      dir[i] = kernelDir[i];

   dir[kRecursivePde] = frame | kPageFlag_Present | kPageFlag_Writable;
   _pagingScratchUnmap(kScratchDirectory);
   irqRestore(flags);

   return space;
}

void addressSpaceDestroy(AddressSpace* space)
{
   if (space == nil || space == &g_bootSpace)
      return;

   if (space == g_currentSpace)
      kernelPanic("addressSpaceDestroy: destroying the loaded space");

   u32 flags = irqSave();
   u32* dir  = _pagingScratchMap(kScratchDirectory, space->physicalDirectory);   
   for (u32 dirIndex = 0; dirIndex < kKernelPdeFirst; dirIndex++) {
      if (!(dir[dirIndex] & kPageFlag_Present))
         continue;

      u32  tableFrame = dir[dirIndex] & ~0xFFF;
      u32* tbl        = _pagingScratchMap(kScratchTable, tableFrame);
      for (u32 i = 0; i < kPageTableEntries; i++) {
         if (tbl[i] & kPageFlag_Present)
            pmmFreeFrame(tbl[i] & ~0xFFF);
      }

      _pagingScratchUnmap(kScratchTable);
      pmmFreeFrame(tableFrame);
   }

   _pagingScratchUnmap(kScratchDirectory);
   irqRestore(flags);
   
   pmmFreeFrame(space->physicalDirectory);
   kfree(space);
}

void addressSpaceLoad(AddressSpace* space)
{  
   if (space == nil)
      kernelPanic("addressSpaceLoad: nil address space");
      
   g_currentSpace = space;
   _pagingLoadDirectory(space->physicalDirectory);
}

#ifdef kIncludeSelfTests
void pagingTestUserMapping()
{
   const u32 scratchVa = 0x00400000;

   u32 frame = pmmAllocFrame();
   if (frame == kInvalidFrame)
   {
      kError("paging test: no free frame");
      return;
   }

   if (!pagingMapPage(scratchVa, frame, kPageFlag_Writable | kPageFlag_User))
   {
      kError("paging test: map failed");
      pmmFreeFrame(frame);
      return;
   }

   u32 uDir = scratchVa >> 22;
   u32 uTbl = (scratchVa >> 12) & 0x3FF;
   u32 userPde = _pagingDirectory()[uDir];
   u32 userPte = _pagingTable(uDir)[uTbl];

   u32 kDir = kKernelVirtualBase >> 22;
   u32 kTbl = (kKernelVirtualBase >> 12) & 0x3FF;
   u32 kernPde = _pagingDirectory()[kDir];
   u32 kernPte = _pagingTable(kDir)[kTbl];

   kTrace("user   PDE=%x PTE=%x", userPde, userPte);
   kTrace("kernel PDE=%x PTE=%x", kernPde, kernPte);

   bool ok = true;
   ok &= (userPde & kPageFlag_User) != 0;
   ok &= (userPte & kPageFlag_User) != 0;
   ok &= (kernPde & kPageFlag_User) == 0;
   ok &= (kernPte & kPageFlag_User) == 0;

   if (ok)
      kTrace("paging test: U/S correct (user set, kernel clear)");
   else
      kError("paging test: U/S WRONG");

   pagingUnmapPage(scratchVa);
   pmmFreeFrame(frame);
}

void pagingTestDumpHigherFrames()
{
   for (u32 i = kKernelPdeFirst; i < kPageDirEntries; i++) {
      if (_pagingDirectory()[i] & kPageFlag_Present)
         kTrace("PDE %u = %x", i, _pagingDirectory()[i]);
   }
}
#endif
