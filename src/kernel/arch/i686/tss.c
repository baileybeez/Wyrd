#include "wyrd.h"
#include "tss.h"
#include "gdt.h"
#include "lib/mem.h"

#define kRing0StackSize 4096

typedef struct {
	u32 prev_tss;
	u32 esp0;
	u32 ss0;
	u32 esp1;
	u32 ss1;
	u32 esp2;
	u32 ss2;
	u32 cr3;
	u32 eip;
	u32 eflags;
	u32 eax;
	u32 ecx;
	u32 edx;
	u32 ebx;
	u32 esp;
	u32 ebp;
	u32 esi;
	u32 edi;
	u32 es;
	u32 cs;
	u32 ss;
	u32 ds;
	u32 fs;
	u32 gs;
	u32 ldt;
	u32 trap;
	u32 iomap_base;
}__attribute__((packed)) TSSEntry;

static TSSEntry _tss;
static u8       _ring0Stack[kRing0StackSize] __attribute__((aligned(16)));

// this assignment of `_ring0Stack` is now vestigal for scheduled threads. it's 
// only the pre-scheduler esp0. 
// 
// ** Every context switch reassigns it (see scheduler.c: `schedule()`)
void tssInit()
{
   memset((void*)&_tss, 0, sizeof(TSSEntry));

   _tss.ss0        = kGDT_KernelDataSelector;
   _tss.esp0       = (u32)_ring0Stack + kRing0StackSize;
   _tss.iomap_base = sizeof(TSSEntry);

   gdtInstallTss((u32)&_tss, sizeof(TSSEntry) - 1);
   __asm__ volatile("ltr %%ax" :: "a"((u16)kGDT_TssSelector));
}

void tssSetKernelStack(u32 esp0)
{
   _tss.esp0 = esp0;
}
