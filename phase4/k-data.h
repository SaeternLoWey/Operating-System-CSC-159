// k-data.h, 159
// kernel data are all declared in main.c during bootstrap
// kernel .c code reference them as 'extern'

#ifndef __K_DATA__
#define __K_DATA__

#include "k-const.h"           // defines PROC_SIZE, PROC_STACK_SIZE
#include "k-type.h"            // defines q_t, pcb_t, ...

extern int run_pid;            // PID of running process
extern int sys_centi_sec;
// prototype the rest
extern q_t pid_q, ready_q, sleep_q;                 		// avail PID and those created/ready to run
extern pcb_t pcb[PROC_SIZE];               		// Process Control Blocks
extern char proc_stack[PROC_SIZE][PROC_STACK_SIZE];   	// process runtime stacks
extern mux_t mux[MUX_SIZE];
extern q_t mux_q;
extern int vid_mux;

extern term_t term[TERM_SIZE];

/////////

#endif                         // endif of ifndef
