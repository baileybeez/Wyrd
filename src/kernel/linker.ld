ENTRY(_start)

kKernelVirtualBase = 0xC0000000;

SECTIONS
{
   . = 1M;
   _kernelStart = .;

   .multiboot : ALIGN(8) {
      KEEP(*(.multiboot))
   }

   .boot_text : ALIGN(4K) {
      *(.boot_text)
   }

   .boot_bss : ALIGN(4K) {
      *(.boot_bss)
   }

   . += kKernelVirtualBase;
   _kernelVirtualStart = .;

   .text ALIGN(4K) : AT(ADDR(.text) - kKernelVirtualBase) {
      *(.text)
   }

   .rodata ALIGN(4K) : AT(ADDR(.rodata) - kKernelVirtualBase) {
      *(.rodata)
   }

   .data ALIGN(4K) : AT(ADDR(.data) - kKernelVirtualBase) {
      *(.data)
   }

   .bss ALIGN(4K) : AT(ADDR(.bss) - kKernelVirtualBase) {
      *(COMMON)
      *(.bss)
   }

   _kernelEnd = .;
   _kernelPhysicalEnd = _kernelEnd - kKernelVirtualBase;
}
