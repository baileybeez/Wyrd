#include "bee.h"
#include "lib/logger.h"
#include "lib/mem.h"
#include "mm/memory.h"
#include "mm/pmm.h"
#include "isr.h"
#include "paging.h"

#define kPagingDirectory   0xFFFFF000
#define kPagingTableRoot   0xFFC00000

#define kPageDirEntries    1024
#define kPageTableEntries  1024

static u32 g_kernelDirectory[kPageDirEntries]   __attribute__((aligned(4096)));
static u32 g_kernelTable[kPageTableEntries]     __attribute__((aligned(4096)));

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

static bool _pagingEnsureTable(u32 dirIndex)  // pmmAllocFrame + install PDE + zero via recursive window
{
   if (g_kernelDirectory[dirIndex] & kPageFlag_Present)
      return true;

   u32 frame = pmmAllocFrame();
   if (frame == kInvalidFrame)
      return false;

   g_kernelDirectory[dirIndex] = frame | kPageFlag_Present | kPageFlag_Writable;

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
   for (;;) __asm__ volatile("cli; hlt");
}

void pagingInit()
{
   memset(g_kernelDirectory, 0x00, kPageDirEntries * sizeof(u32));
   for (u32 i = 0; i < kPageTableEntries; i++)
      g_kernelTable[i] = (i * kPageSize) | kPageFlag_Present | kPageFlag_Writable;

   u32 kernelDir = virtualToPhys((u32)g_kernelDirectory);
   g_kernelDirectory[768]  = virtualToPhys((u32)g_kernelTable) | kPageFlag_Present | kPageFlag_Writable;
   g_kernelDirectory[1023] = kernelDir | kPageFlag_Present | kPageFlag_Writable;

   kTrace("kernelDir phys = %x", kernelDir);
   kTrace("PDE768 = %x, PDE1023 = %x", g_kernelDirectory[768], g_kernelDirectory[1023]);
   isrRegister(14, _pagingFaultHandler);
   _pagingLoadDirectory(kernelDir);
   kTrace("survived CR3 load");   // <-- does this print?
}

bool pagingMapPage(u32 virtualAddr, u32 physicalAddr, u32 flags)
{   
   u32 dirIndex   = virtualAddr >> 22;
   u32 tableIndex = (virtualAddr >> 12) & 0x3FF;
   
   if (!_pagingEnsureTable(dirIndex))
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
   return true;
}

u32  pagingGetPhysical(u32 virtualAddr)
{
   u32 dirIndex   = virtualAddr >> 22;
   u32 tableIndex = (virtualAddr >> 12) & 0x3FF;
   u32 offset     = virtualAddr & 0xFFF;

   u32* dir = _pagingDirectory();
   if (!(dir[dirIndex] & kPageFlag_Present))
      return kInvalidPhysical;

   u32* tbl = _pagingTable(dirIndex);
   if (!(tbl[tableIndex] & kPageFlag_Present))
      return kInvalidPhysical;

   return (tbl[tableIndex] & ~0xFFF) | offset;
}

void pagingInvalidatePage(u32 virtualAddr)
{
   __asm__ volatile("invlpg (%0)" :: "r"(virtualAddr) : "memory");
}
