// k-type.h, 159

#ifndef __K_TYPE__
#define __K_TYPE__

#include "k-const.h"

typedef void (*func_p_t)(void); // void-return function pointer type

typedef enum {UNUSED, READY, RUN, SLEEP, SUSPEND} state_t;

typedef struct
{
    unsigned int edi;
    unsigned int esi;
    unsigned int ebp;
    unsigned int esp;
    unsigned int ebx;
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;

    unsigned int entry_id;
    unsigned int eip;
    unsigned int cs;
    unsigned int efl;
   
} trapframe_t;

typedef struct
{
    state_t state;
    int run_count, total_count;
    int wake_centi_sec;
    trapframe_t *trapframe_p;
} pcb_t;                     

typedef struct
{
    // generic queue type
    int q[Q_SIZE], tail;
} q_t;

typedef struct
{
    int flag;
    int creator;
    q_t suspend_q; 
} mux_t;

#endif
