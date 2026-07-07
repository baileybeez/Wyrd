; lgdt / lidt wrappers

bits 32

global gdtFlush
global idtFlush

section .text

gdtFlush:
   mov   eax, [esp + 4]
   lgdt  [eax]

   mov   eax, 0x10
   mov   ds, ax
   mov   es, ax
   mov   fs, ax
   mov   gs, ax
   mov   ss, ax
   jmp   0x08:.reload
.reload
   ret

idtFlush:
   mov   eax, [esp + 4]
   lidt  [eax]
   ret
   