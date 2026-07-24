; switchContext(u32* oldEspSlot, u32 newEsp)
;
; Saves cdecl callee-saved registers (ebx, esi, edi, ebp) on the current stack,
; writes the resulting esp into *oldEspSlot, loads esp from newEsp, restores
; callee-saved regs, and returns. On return, execution continues wherever the
; new context was last suspended.
;
; Caller-saved regs (eax, ecx, edx) don't need preservation across a cdecl call.
; EFLAGS is inherited from the current context — if you yield with interrupts
; enabled, the new thread starts with interrupts enabled.

section .text
global switchContext

switchContext:
   pushfd                     ; save EFLAGS
   push  ebp
   push  ebx
   push  esi
   push  edi

   ; stack at this point:
   ;   [esp +  0]  edi
   ;   [esp +  4]  esi
   ;   [esp +  8]  ebx
   ;   [esp + 12]  ebp
   ;   [esp + 16]  eflags
   ;   [esp + 20]  return address
   ;   [esp + 24]  oldEspSlot   (arg 0)
   ;   [esp + 28]  newEsp       (arg 1)

   mov   eax, [esp + 24]      ; eax = oldEspSlot
   mov   [eax], esp           ; *oldEspSlot = esp
   mov   esp, [esp + 28]      ; esp = newEsp

   pop   edi
   pop   esi
   pop   ebx
   pop   ebp
   popfd                      ; restore flags
   ret
