; Stage 2 Bootloader - stub
; 
; Loaded by Stage 1 at 0x0000:0x7E00 in 16-bit real mode.
; DL contains the boot drive number on entry (passed through by Stage 1).
;

BITS 16
ORG 0x7E00

start:
   cli
   xor   ax, ax
   mov   ds, ax
   mov   es, ax
   mov   ss, ax
   mov   sp, 0x7C00
   sti

   mov   si, msgStage2
   call  printString

.halt:
   cli
   hlt
   jmp .halt

printString:
   mov   ah, 0x0E
   mov   bh, 0x00 
   mov   bl, 0x07
.loop:
   lodsb
   test  al, al
   jz    .done
   int   0x10
   jmp   .loop
.done:
   ret

msgStage2:  db "S2", 0
