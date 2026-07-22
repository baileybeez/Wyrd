OUTPUT_FORMAT(binary)
ENTRY(_start)

kBootInfoAddr   = 0x00001000;
kKernelLoadAddr = 0x00100000;

SECTIONS
{
   . = 0x7E00;

   .text : { 
      *(.text.entry)
      *(.text .text.*)
   }

   .rodata : {
      *(.rodata .rodata.*)
   }

   .data : {
      *(.data .data.*)
   }

   .bss (NOLOAD) : {
      _bssStart = .;
      *(.bss .bss.*)
      *(COMMON)
      _bssEnd = .;
   }

   _stage2End = .;

   /DISCARD/ : {
      *(.comment)
      *(.note*)
      *(.eh_frame)
   }
}
