[bits 32]
[org 0x00401000]              ; blob is mapped here; string address resolves correctly

_start:
   mov   eax, 1               ; kSys_Write
   mov   ebx, msg             ; pointer - absolute, correct because of [org]
   mov   ecx, msg_len         ; length
   xor   edx, edx
   int   0x80

.hang:
   jmp   .hang                ; relative jump, spin forever

msg:      db "Hello from ring 3!", 0x0A
msg_len:  equ $ - msg
