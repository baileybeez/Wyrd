; Multiboot2 header + entry stub.
; 
; bootstrap a temporary identity and higher-half 4MB page, 
; enable paging, and jump into the higher-half before calling
; into `kernelBootstrap` for Magic / BootInfo parsing before
; calling `kernelMain`
;
; NOTE: _start MUST be the first instruction in the output binary.
; stage2 loads kernel.bin (raw) and jumps to its base address; if the
; multiboot header sits there instead, the CPU executes it as code —
; the first byte is 0xD6 (SALC), which silently zeroes AL and corrupts
; the boot magic in eax before it ever reaches paging setup.
;

kMultibootMagic   equ 0x36D76289
kCustomBootMagic  equ 0xB007B33F                          ; must match bootInfo.h and boot.inc

global _start
global boot_page_directory
global stack_bottom
global stack_top

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

kKernelVirtualBase      equ 0xC0000000
kPageDirEntryIdentity   equ 0
kPageDirEntryHighHalf   equ (kKernelVirtualBase >> 22)
kPdeFlags               equ 0x83                         ; present | writable | PS (4MB)

section .boot_bss nobits alloc noexec write align=4096
boot_page_directory:
   resd  1024

align 16
boot_stack_bottom:
   resb  4096
boot_stack_top:

section .boot_text progbits alloc exec nowrite align=16
_start:
   cld 
   mov   esp, boot_stack_top

   push  ebx                              ; bootInfo pointer
   push  eax                              ; boot magic 
   
   mov   edi, boot_page_directory
   xor   eax, eax
   mov   ecx, 1024
   rep   stosd                            ; zero the page directory

   mov   dword [boot_page_directory + kPageDirEntryIdentity * 4], kPdeFlags
   mov   dword [boot_page_directory + kPageDirEntryHighHalf * 4], kPdeFlags

   mov   eax, boot_page_directory
   mov   cr3, eax

   mov   eax, cr4 
   or    eax, 0x00000010                  ; CR4.PSE
   mov   cr4, eax
   
   mov   eax, cr0
   or    eax, 0x80000000                  ; CR0.PG 
   mov   cr0, eax

   jmp   higher_half_entry

section .text
extern kernelBootstrap
higher_half_entry:
   pop   eax                              ; restore magic 
   pop   ebx                              ; restore pointer
   
   mov   esp, stack_top                   ; setup real stack
   push  ebx
   push  eax
   cld
   call  kernelBootstrap

.hang:
   cli
   hlt
   jmp .hang

section .bss
align 16
stack_bottom:
   resb 16384                             ; 16 KiB kernel stack
stack_top:
