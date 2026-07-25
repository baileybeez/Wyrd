; i686 interrupt handling

bits 32

extern isrHandler

section .text

%macro ISR_NOERR 1
global isr%1
isr%1: 
   cli
   push dword 0
   push dword %1
   jmp isrCommonStub
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
   cli
   push dword %1
   jmp isrCommonStub
%endmacro

; 0 - 31: per Intel SDM, 
; 8, 10, 11, 12, 13, 14, 17, 21 push error codes, the rest do not
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR   21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

; 0x80 (128) - syscall entry; reached via int 0x80 from ring 3 (gate DPL=3)
ISR_NOERR 128

; default stub for any interrupt we haven't wired up specifically
global isrDefault
isrDefault:
   cli
   push dword 0
   push dword 0xFF
   jmp isrCommonStub

; ISR common stub
; + save state, unwinds intNo+errorCode, returns via iret
isrCommonStub:
   pusha                ; edi, esi, ebp, esp, ebx, edx, ecx, eax

   xor   eax, eax
   mov   ax, ds
   push  eax            ; save ds

   mov   ax, 0x10       ; kernel data selector
   mov   ds, ax
   mov   es, ax
   mov   fs, ax
   mov   gs, ax

   push  esp            ; pointer to `struct Registers`
   call  isrHandler
   add   esp, 4

   pop   eax            ; restore ds
   mov   ds, ax
   mov   es, ax
   mov   fs, ax
   mov   gs, ax

   popa
   add esp, 8           ; drop intNo + errorCode
   sti
   iret
