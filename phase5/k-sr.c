// k-sr.c, 159

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"
#include "k-lib.h"
#include "k-sr.h"
#include "sys-call.h"

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
    //pcb[pid].trapframe_p--;                                       // lower by trapframe size
    pcb[pid].trapframe_p = (trapframe_t *)&proc_stack[pid][PROC_STACK_SIZE-sizeof(trapframe_t)];
    pcb[pid].trapframe_p->efl = EF_DEFAULT_VALUE|EF_INTR;           // enables intr
    pcb[pid].trapframe_p->cs = get_cs();                            // dupl from CPU
    pcb[pid].trapframe_p->eip = (unsigned) p;                       // set to code
}

void CheckWakeProc(void)
{
    int i;
    int pid;
    int tail;

    tail = sleep_q.tail;
    for(i = 0; i < tail; i++)
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
    run_pid = NONE;
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

int MuxCreateSR(int flag)
{
    int mux_id;
    //mux_t new_mux;	
    mux_id = DeQ(&mux_q);   //Allocate mux from OS mux queue
    
    // Unsure, potential spot of failure
    
    Bzero((char*)&mux[mux_id], sizeof(mux_t));

    mux[mux_id].flag = flag;
    mux[mux_id].creator = run_pid;

   // mux[mux_id] = new_mux;

    return mux_id;
}

void MuxOpSR(int mux_id, int opcode)
{
    if(opcode == LOCK)
    {
	if((mux[mux_id].flag > 0))     //if the flag is 1 then 0 it.
	{
	    mux[mux_id].flag--; 
	}
	else 
	{	
	    //queue the PID of calling process to suspension queue in mutex
            EnQ(run_pid,&mux[mux_id].suspend_q);
	    pcb[run_pid].state = SUSPEND;
	    run_pid = NONE;
	}	
    }
    else if(opcode == UNLOCK)
    {
	if(QisEmpty(&mux[mux_id].suspend_q))
	{
	    mux[mux_id].flag++;
	}
    	else
        {
            int pid;
	    pid = DeQ(&mux[mux_id].suspend_q);
	    EnQ(pid, &ready_q);
	    pcb[pid].state = READY;
	}
    }	
}

void TermSR(int term_no)
{
    int r;
    r = inportb(term[term_no].io_base + IIR);
    
    if(r == TXRDY)
        TermTxSR(term_no);
    else if (r == RXRDY)
        TermRxSR(term_no);

    if(term[term_no].tx_missed == TRUE)
        TermTxSR(term_no);
}

void TermTxSR(int term_no)
{
    int c;

    if(QisEmpty(&term[term_no].out_q) == TRUE && QisEmpty(&term[term_no].echo_q) == TRUE)
    {
        term[term_no].tx_missed = TRUE;
        return;
    }
    else 
    {
        if(QisEmpty(&term[term_no].echo_q) == FALSE)
        {
            c = DeQ(&term[term_no].echo_q);
	}
        else
        {
            c = DeQ(&term[term_no].out_q);
        }
            
        outportb(term[term_no].io_base + DATA, (char) c);
        term[term_no].tx_missed = FALSE;
	MuxOpSR(term[term_no].out_mux, UNLOCK);
    }
}

void TermRxSR(int term_no)
{
    char c;
    c = inportb(term[term_no].io_base + DATA);

    EnQ(c, &term[term_no].echo_q);
    if(c == '\r')
    {
        EnQ('\n', &term[term_no].echo_q);
        EnQ('\0', &term[term_no].in_q);
    }
    else
    {
        EnQ(c, &term[term_no].in_q);
    }

    MuxOpSR(term[term_no].in_mux, UNLOCK);
}



