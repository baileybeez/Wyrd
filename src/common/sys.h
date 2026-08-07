#pragma once

// Wyrd System Call ABI
// 
// Register Convention
//    int 0x80
//
//    eax      :: syscall #
//    ebx      :: arg 0
//    ecx      :: arg 1
//    edx      :: arg 2
//    esi      :: arg 3
//    edi      :: arg 4
//
// Callee-saved across the trap: ebx, esi, edi, ebp, esp.
// Clobbered: eax (return value). All others preserved by the kernel.
//

#define kSysErr_Fault      -1
#define kSysErr_TooLong    -2
#define kSysErr_NoSys      -3

#define kExitReservedFloor -1000
#define kExitFault         -1001
#define kExitOutOfMem      -1002
#define kExitKilled        -1003

#define kSyscall_Invalid   0
#define kSyscall_Read      1
#define kSyscall_Write     2
#define kSyscall_Exit      3

#define kSyscall_Count     4

#define kFd_StdIn          1
#define kFd_StdOut         2
#define kFd_StdErr         3
