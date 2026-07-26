; void enterUserMode(u32 entry, u32 userStack)
global enterUserMode
enterUserMode:
   mov   eax, [esp + 4]      ; entry
   mov   ebx, [esp + 8]      ; userStack

   mov   cx, 0x23            ; user data selector, RPL 3
   mov   ds, cx
   mov   es, cx
   mov   fs, cx
   mov   gs, cx

   push  0x23                ; SS  = user data | 3
   push  ebx                 ; ESP = top of user stack
   push  0x202               ; EFLAGS: reserved bit 1 + IF
   push  0x1B                ; CS  = user code | 3
   push  eax                 ; EIP = entry
   iret