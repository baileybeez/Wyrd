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
%include "boot.inc"

BITS 16

extern stage2Main

; ----   Entry Point   ---- 
section .text.entry
global _start
_start:
   cli
   xor   ax, ax                  ; clear segment registers
   mov   ds, ax
   mov   es, ax
   mov   ss, ax
   mov   sp, 0x7C00              ; setup stack pointer

   mov   [bootDrive], dl         ; save boot drive
   call  collectE820             ; collect E820 mmap data from BIOS
   call  enableA20               ; enable the A20 line
   lgdt  [gdtDescriptor]         ; setup basic gdt

   mov   eax, cr0
   or    eax, 1                  ; set protected mode bit
   mov   cr0, eax
   
   jmp   0x08:protected

; ----   Enable A20    ---- 
enableA20:
   call  checkA20
   test  ax, ax
   jnz   .done                   ; already enabled by BIOS/loader

   mov   ax, 0x2401              ; BIOS: Enable A20
   int   0x15
   call  checkA20
   test  ax, ax
   jnz   .done

   call  enableA20Kbc            ; 8042 keyboard controller path
   call  checkA20
   test  ax, ax
   jnz   .done

   mov   ax, 0x0E41              ; teletype 'A' on failure
   int   0x10
.halt:
   cli
   hlt
   jmp   .halt
.done:
   ret

; Segment-wraparound test. Returns AX=1 if A20 on, AX=0 if off.
; Writes 0x00 to 0x0000:0x0500 and 0xFF to 0xFFFF:0x0510 (linear 0x100500
; with A20, wraps to 0x000500 without). Restores original bytes.
checkA20:
   push  ds
   push  es
   push  si
   push  di

   xor   ax, ax
   mov   ds, ax
   mov   si, 0x0500              ; DS:SI = 0x0000:0x0500

   not   ax                      ; AX = 0xFFFF
   mov   es, ax
   mov   di, 0x0510              ; ES:DI = 0xFFFF:0x0510

   mov   al, [ds:si]             ; save originals on stack
   push  ax
   mov   al, [es:di]
   push  ax

   mov   byte [ds:si], 0x00
   mov   byte [es:di], 0xFF
   mov   al, [ds:si]             ; if wrapped, AL = 0xFF
   cmp   al, 0xFF                ; sets ZF; restores below do not touch flags

   pop   bx
   mov   [es:di], bl             ; restore original bytes
   pop   bx
   mov   [ds:si], bl

   mov   ax, 0
   je    .off                    ; ZF set -> wrapped -> A20 disabled
   mov   ax, 1
.off:
   pop   di
   pop   si
   pop   es
   pop   ds
   ret

; 8042 KBC method: disable kbd, read output port, set bit 1, write back, re-enable.
enableA20Kbc:
   call  .waitInput
   mov   al, 0xAD                ; disable kbd interface
   out   0x64, al

   call  .waitInput
   mov   al, 0xD0                ; read controller output port
   out   0x64, al
   call  .waitOutput
   in    al, 0x60
   push  ax                      ; save output port value

   call  .waitInput
   mov   al, 0xD1                ; write controller output port
   out   0x64, al
   call  .waitInput
   pop   ax
   or    al, 0x02                ; set A20 gate
   out   0x60, al

   call  .waitInput
   mov   al, 0xAE                ; re-enable kbd interface
   out   0x64, al
   call  .waitInput
   ret

.waitInput:                      ; spin until input buffer empty (status bit 1 = 0)
   in    al, 0x64
   test  al, 0x02
   jnz   .waitInput
   ret
.waitOutput:                     ; spin until output buffer full (status bit 0 = 1)
   in    al, 0x64
   test  al, 0x01
   jz    .waitOutput
   ret

; ---- E820 Collection ---- 
collectE820:
   xor   ax, ax
   mov   es, ax
   mov   di, kBootInfoAddr + kBootInfoMmapOffset
   xor   ebx, ebx                ; continuation = 0
   xor   bp, bp                  ; entry count
.next:
   mov   eax, 0xE820
   mov   edx, 0x534D4150         ; 'SMAP'
   mov   ecx, kMmapEntrySize  
   mov   dword [es:di + 20], 1   ; default ACPI ext-attr = valid
   int   0x15
   jc    .cfSet
   cmp   eax, 0x534D4150         ; signature must echo back
   jne   .fail

   test  ecx, ecx                ; BIOS wrote 0 bytes -> skip
   jz    .maybeMore
   cmp   dword [es:di + 8], 0    ; length low
   jne   .keep
   cmp   dword [es:di + 12], 0   ; length high
   je    .maybeMore
.keep:
   inc   bp
   add   di, kMmapEntrySize
.maybeMore:
   test  ebx, ebx                ; ebx == 0 -> that was the last entry
   jz    .done
   cmp   bp, kMaxMmapEntries
   jae   .done
   jmp   .next

.cfSet:
   test  bp, bp                  ; CF on first call -> Unsupported, CF later -> valid term
   jz    .fail
.done:
   movzx eax, bp
   mov   [kBootInfoAddr + kBootInfoMmapCountOffset], eax
   ret

.fail:
   mov   ax, 0x0E45              ; teletype 'E'
   int   0x10
.halt:
   cli
   hlt
   jmp   .halt

; ---- Protected  Mode ---- 
BITS 32
protected:
   mov   ax, 0x10                ; setup segment registers
   mov   ds, ax
   mov   es, ax
   mov   ss, ax
   mov   fs, ax
   mov   gs, ax

   mov   esp, 0x10000            ; setup stack 

   movzx eax, byte [bootDrive]   ; call -> stage2Main(u8 bootDrive)
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
