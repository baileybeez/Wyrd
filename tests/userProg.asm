[bits 32]
[org 0x00401000]

kSysWrite   equ 1
kSysExit    equ 2

_start:
   mov   esi, 10
   
.loop:
   mov   eax, kSysWrite
   mov   ebx, msg
   mov   ecx, msgLen
   int   0x80

   mov   ecx, 0x02000000
.delay:
   dec   ecx
   jnz   .delay

   dec   esi
   jnz   .loop

   mov   eax, kSysExit
   mov   ebx, 0
   mov   ecx, 0
   int   0x80

.hang:
   jmp   .hang             ; sysExit should NEVER return to here

msg:    db "ring3 tick", 10
msgLen  equ $ - msg
