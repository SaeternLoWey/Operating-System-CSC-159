#include "k-const.h"
#include <spede/machine/asmacros.h> 
#define K_STACK_SIZE (PROC_STACK_SIZE * 8)

.comm k_stack, K_STACK_SIZE, 1
.text

// load proc trapframe to run it
ENTRY(Loader)
   #copy 4 bytes from 4B above esp to ebx
   movl 4(%esp), %ebx
   #copy ebx to esp (point esp to process trapframe)
   movl %ebx, %esp
   #pop all (8) registers
   popa
   #add 4 to esp
   add $4, %esp
   #return from interrupt
   iret


ENTRY(TimerEntry)
   pushl $TIMER_INTR
   #pushes the timer_interrupt 
   jmp PreKernel
   #after pushing it jump to the prekernel 

ENTRY(GetPidEntry)
   pushl $GETPID_CALL
   jmp PreKernel

ENTRY(ShowCharEntry)
   pushl $SHOWCHAR_CALL
   jmp PreKernel

ENTRY(SleepEntry)
   pushl $SLEEP_CALL
   jmp PreKernel

ENTRY(MuxCreateEntry)
    pushl $MUX_CREATE_CALL
    jmp PreKernel

ENTRY(MuxOpEntry)
    pushl $MUX_OP_CALL
    jmp PreKernel

ENTRY(Term0Entry)
    pushl $TERM0_INTR
    jmp PreKernel

ENTRY(Term1Entry)
    pushl $TERM1_INTR
    jmp PreKernel

ENTRY(ForkEntry)
    pushl $FORK_CALL
    jmp PreKernel

ENTRY(WaitEntry)
    pushl $WAIT_CALL
    jmp PreKernel

ENTRY(ExitEntry)
    pushl $EXIT_CALL
    jmp PreKernel

ENTRY(ExecEntry)
    pushl $EXEC_CALL
    jmp PreKernel

ENTRY(SignalEntry)
    pushl $SIGNAL_CALL
    jmp PreKernel

// build proc trapframe, and go into kernel
PreKernel:                  // will have efl, cs, eip pushed by hardware
   #push all (8) registers
   pusha
  
   #copy esp to ebx
   # // -v- what this for? -v- //
   movl %esp, %ebx

   movw $0x10, %ax                 // set kernel data segment registers
   mov %ax, %ds                    // set kernel data segment registers
   mov %ax, %es                    // set kernel data segment registers
   leal k_stack + K_STACK_SIZE, %esp  // set esp to top of kernel stack
   #push ebx (onto the kernel stack)
   push %ebx
   #clear the direction flag of efl
   cld
   #call C function Kernel
   call CNAME(Kernel)

