; Stage 2 Bootloader - real-mode shim
;
; Loaded by Stage 1 at 0x0000:0x7E00 in 16-bit real mode.
; DL contains the boot drive number on entry (passed through by Stage 1).
; 
; Entry : BITS 16, DL = boot drive (from stage 1)
; Exit  : BITS 32 protected mode, calls stage2Main(bootDrive)
; 
; Deferred: 
;  - E280 collection (step 5)
;  - A20 enable      (step 6)
;  - BootInfo writes (step 9) - bootDrive passed as arg for now

BITS 16

extern stage2Main

section .text.entry
global _start
_start:
   cli
   xor   ax, ax
   mov   ds, ax
   mov   es, ax
   mov   ss, ax
   mov   sp, 0x7C00

   mov   [bootDrive], dl
   lgdt  [gdtDescriptor]

   mov   eax, cr0
   or    eax, 1
   mov   cr0, eax

   jmp   0x08:protected

BITS 32
protected:
   mov   ax, 0x10
   mov   ds, ax
   mov   es, ax
   mov   ss, ax
   mov   fs, ax
   mov   gs, ax

   mov   esp, 0x10000

   movzx eax, byte [bootDrive]
   push  eax
   call  stage2Main
   add   esp, 4

.halt:
   cli
   hlt
   jmp .halt

section .data
bootDrive   db 0

section .rodata
align 8
gdt:
   dq 0x0000000000000000      ; null descriptor
   dq 0x00CF9A000000FFFF      ; code: base=0, limit=0xFFFFF, 4K gran, 32-bit, ring 0, exec/read
   dq 0x00CF92000000FFFF      ; data: base=0, limit=0xFFFFF, 4K gran, 32-bit, ring 0, read/write
gdtEnd:

gdtDescriptor:
   dw gdtEnd - gdt - 1
   dd gdt
