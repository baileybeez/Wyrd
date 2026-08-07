; crt0
;
; reads argc/argv off the ring-3 stack, calls `main`, then `exit(ret)`

bits 32
section .text

global _start
extern main
extern exit

_start:
   xor   ebp, ebp
   pop   eax
   mov   ecx, esp
   and   esp, 0xfffffff0   ; align to 16 (round down)
   sub   esp, 8
   push  ecx
   push  eax
   call  main
   push  eax
   call  exit

.hang:
   hlt
   jmp   .hang
