// k-sr.c, 159

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"
#include "k-lib.h"
#include "k-sr.h"

// to create a process: alloc PID, PCB, and process stack
// build trapframe, initialize PCB, record PID to ready_q
void NewProcSR(func_p_t p)
{
    // arg: where process code starts
    int pid;

    if(QisEmpty(&pid_q))
    {   
        // may occur if too many been created
        cons_printf("Panic: no more process!\n");
        return;                     // cannot continue, alternative: breakpoint();
    }

    pid = DeQ(&pid_q);                                   // alloc PID (1st is 0)
    Bzero((char *)&pcb[pid], sizeof(pcb_t));             // clear PCB
    Bzero((char *)&proc_stack[pid][0], PROC_STACK_SIZE); // clear stack
    pcb[pid].state = READY;                              // change process state

    if(pid > 0)
        EnQ(pid, &ready_q);           // queue to ready_q if > 0

    //point trapframe_p to stack & fill it out
    //pcb[pid].trapframe_p = (trapframe_t *) &proc_stack[pid][PROC_STACK_SIZE];                        // point to stack top
    //pcb[pid].trapframe_p--;                                                          // lower by trapframe size
    pcb[pid].trapframe_p = (trapframe_t *)&proc_stack[pid][PROC_STACK_SIZE-sizeof(trapframe_t)];
    pcb[pid].trapframe_p->efl = EF_DEFAULT_VALUE|EF_INTR;                            // enables intr
    pcb[pid].trapframe_p->cs = get_cs();                                             // dupl from CPU
    pcb[pid].trapframe_p->eip = (unsigned) p;                                                   // set to code
}

void CheckWakeProc(void)
{
    int i;
    int pid;
    for(i = 0; i < sleep_q.tail; i++)
    {
        pid = DeQ(&sleep_q);

        if(pcb[pid].wake_centi_sec == sys_centi_sec)
        {
            EnQ(pid, &ready_q);
            pcb[pid].state = READY;
        }
        else
        {
            EnQ(pid, &sleep_q);
        }
    }
}

// count run_count and switch if hitting time slice
void TimerSR(void)
{
    outportb(PIC_CONTROL, TIMER_DONE);        // notify PIC timer done

    sys_centi_sec++;
    CheckWakeProc();

    if(run_pid == 0)
        return;

    pcb[run_pid].run_count++;
    pcb[run_pid].total_count++;

    if(pcb[run_pid].run_count == TIME_SLICE)
    {
        EnQ(run_pid, &ready_q);
        pcb[run_pid].state = READY;
        run_pid = NONE;
    }
}

void SleepSR(int centi_sec)
{
    pcb[run_pid].wake_centi_sec = sys_centi_sec + centi_sec;
    pcb[run_pid].state = SLEEP;
    EnQ(run_pid, &sleep_q);
    run_pid = -1;
}

int GetPidSR(void)
{
    return run_pid;
}

void ShowCharSR(int row, int col, char ch)
{
    unsigned short *p = VID_HOME;   // upper-left corner of displa    y
    p += row * 80;
    p += col;
    *p = ch + VID_MASK;  
}

