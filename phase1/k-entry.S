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
   #return from interrupt
   iret

// build proc trapframe, and go into kernel
ENTRY(TimerEntry)                  // will have efl, cs, eip pushed by hardware
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

