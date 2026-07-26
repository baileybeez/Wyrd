[bits 32]
[org 0x00401000]

kSysWrite equ 1

_start:
   mov   eax, kSysWrite
   mov   ebx, msg
   mov   ecx, msgLen
   int   0x80

   mov   ecx, 0x02000000
.delay:
   dec   ecx
   jnz   .delay

   jmp   _start

msg:    db "ring3 tick", 10
msgLen  equ $ - msg