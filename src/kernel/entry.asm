; Multiboot2 header + entry stub.
; Sets up a stack and jumps into C (kernelMain).

section .multiboot
align 8
header_start:
   dd 0xE85250D6                                         ; magic 
   dd 0                                                  ; i386 protected mode
   dd header_end - header_start                          ; header len
   dd -(0xE85250D6 + 0 + (header_end - header_start))    ; checksum

   ; end tag
   dw 0
   dw 0
   dd 8
header_end:

section .bss
align 16
stack_bottom:
   resb 16384                                            ; 16 KiB kernel stack
stack_top:

section .text
global _start
extern kernelMain
_start:
   mov esp, stack_top
   cld
   call kernelMain

.hang:
   cli
   hlt
   jmp .hang
