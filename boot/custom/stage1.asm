; Stage 1 bootloader — MBR, 512 bytes, loaded by BIOS at 0x0000:0x7C00
;
; loads stage2 from a fixed position on disk (LBA 1)
; after the jump: 
;     dl          BIOS boot drive number
;     cpu-mode    16bit
;     CS:IP       0x0000:0x7E00
;     DS=ES=SS    0
;     SP          0x7C00
;     Interrupts  Enabled
;     A20         not guaranteed (stage 2's job)
; 
;     * Stage1 memory (at 0x7c00 - 0x7dff) is no longer needed
; 
; Assumed Disk Layout:
;  LBA   0        stage 1 (this file, 512 bytes)
;  LBA   1        stage 2
; 
; Data Access Packet (16 bytes, 0x10 in HEX)
;  offset   size     field
;  0        byte     packet size (0x10)
;  1        bytes    reserved    (0x00)
;  2        word     sector count
;  4        word     destination offset
;  6        word     destination segment
;  8        qword    LBA

BITS 16
ORG 0x7C00

%define  kStage2LBA     1
%define  kStage2Sectors 16
%define  kStage2Segment 0x0000
%define  kStage2Offset  0x7E00

start:
   cli
   xor   ax, ax
   mov   ds, ax
   mov   es, ax
   mov   ss, ax
   mov   sp, 0x7C00
   sti

   mov   [bootDrive], dl         ; save boot drive

   ; detect LBA extensions
   mov   ah, 0x41                ; 0x41 = LBA probe option for INT 0x13
   mov   bx, 0x55AA              ; magic number
   int   0x13
   jc    .useCHS                 ; Carry bit will be clear if LBA supported
   cmp   bx, 0xAA55              ; magic will be flipped if LBA supported
   jne   .useCHS
   test  cx, 1                   ; CX will be 1 if LBA supported
   jz    .useCHS

.useLBA:
   mov   si, dap
   mov   ah, 0x42                         ; 0x42 = LBA extensions option for INT 0x13
   mov   dl, [bootDrive]
   int   0x13
   jc    .diskError
   jmp   .loadDone

.useCHS:
   ; NOTE: this assumes >=17 sectors/ track. safe for QEMU geometries this path is 
   ;       unreachable on QEMU (LBA ext present) but kept for correctness.
   mov   ah, 0x02                         ; 0x02 = read sectors option for INT 0x13
   mov   al, kStage2Sectors               ; # of sectors
   mov   ch, 0                            ; cylinder 0
   mov   cl, 2                            ; sector 2 (CHS is 1-based index)
   mov   dh, 0                            ; head 0
   mov   dl, [bootDrive]
   mov   bx, kStage2Offset                ; ES:BX = 0x0000 0x7E00
   int   0x13
   jc    .diskError

.loadDone:
   mov   dl, [bootDrive]
   jmp   kStage2Segment:kStage2Offset     ; 0x0000 0x7E00

.diskError:
   mov   si, errorMsg
.printLoop:
   lodsb
   test  al, al
   jz    .halt
   mov   ah, 0x0E
   mov   bx, 0x0007
   int   0x10
   jmp   .printLoop

.halt:
   cli
   hlt
   jmp   .halt

bootDrive:  db 0
errorMsg:   db "Disk Error", 0
dap:                                     ; data access packet
   db    0x10
   db    0x00
   dw    kStage2Sectors
   dw    kStage2Offset
   dw    kStage2Segment
   dq    kStage2LBA

times 510 - ($ - $$) db 0
dw 0xAA55
