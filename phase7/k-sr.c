// k-sr.c, 159

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"
#include "k-lib.h"
#include "k-sr.h"
#include "proc.h"
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
    char c;

    if(QisEmpty(&term[term_no].out_q) == TRUE && QisEmpty(&term[term_no].echo_q) == TRUE)
    {
        term[term_no].tx_missed = TRUE;
        return;
    }
    
    if(QisEmpty(&term[term_no].echo_q) == FALSE)
    {
        c = DeQ(&term[term_no].echo_q);
    }
    else
    {
        c = DeQ(&term[term_no].out_q);
	MuxOpSR(term[term_no].out_mux, UNLOCK);  
    }
    
    outportb(term[term_no].io_base + DATA, c);
    term[term_no].tx_missed = FALSE;
}

void TermRxSR(int term_no)
{
    char c;
    int suspendedPid;
    int terminal; 
    int handler;


    c = inportb(term[term_no].io_base + DATA);

    // if ctrl-C is pressed do this
    if(c == 3)
    {	
        if(!(QisEmpty(&mux[term[term_no].in_mux].suspend_q)))
        {
            suspendedPid = mux[term[term_no].in_mux].suspend_q.q[0];

	    if(pcb[suspendedPid].sigint_handler != '0')
            {
     	        if(term_no == 0)
                {
   		    terminal = TERM0_INTR;
                }
                else
                {
		    terminal = TERM1_INTR;
                }

		handler = pcb[suspendedPid].sigint_handler;

                WrapperSR(suspendedPid,handler, terminal);
            }
        }
    }

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

int ForkSR(void)
{
    int child_pid;
    int diff;
    int *p;

    if(QisEmpty(&pid_q))
    {
        cons_printf("Panic: no more process!\n");
        return NONE;
    }

    child_pid = DeQ(&pid_q);
    
    Bzero((char *)&pcb[child_pid], sizeof(pcb_t));
    
    pcb[child_pid].state = READY;
    pcb[child_pid].ppid = run_pid;
    EnQ(child_pid, &ready_q);

    diff = PROC_STACK_SIZE * (child_pid - run_pid);

    pcb[child_pid].trapframe_p = (trapframe_t *)((int) pcb[run_pid].trapframe_p + diff);
    MemCpy((char *)&proc_stack[child_pid], (char *)&proc_stack[run_pid], PROC_STACK_SIZE);

    pcb[child_pid].trapframe_p->eax = 0;
    pcb[child_pid].trapframe_p->esp = pcb[run_pid].trapframe_p->esp + diff;
    pcb[child_pid].trapframe_p->ebp = pcb[run_pid].trapframe_p->ebp + diff;
    pcb[child_pid].trapframe_p->esi = pcb[run_pid].trapframe_p->esi + diff;
    pcb[child_pid].trapframe_p->edi = pcb[run_pid].trapframe_p->edi + diff;

    p = (int *) pcb[child_pid].trapframe_p->ebp;

    while(*p != 0)
    {
        *p += diff;
        p = (int *) *p;
    }

    return child_pid;
}

int WaitSR(void)
{
    int exit_code;
    int x;

    for(x = 0; x < PROC_SIZE; x++)
    {
        if((pcb[x].ppid == run_pid) && pcb[x].state == ZOMBIE)
            break;
    }

    if(x == PROC_SIZE)
    {
        pcb[run_pid].state = WAIT;
        run_pid = NONE;
        return NONE;
    }

    exit_code = pcb[x].trapframe_p -> eax;

    pcb[x].state = UNUSED;
    EnQ(x, &pid_q);

    return exit_code;
}

void ExitSR(int exit_code)
{
    int ppid = pcb[run_pid].ppid;
    if(pcb[ppid].state != WAIT)
    {
        pcb[run_pid].state = ZOMBIE;
        run_pid = NONE;
        return;
    }

    pcb[ppid].state = READY;
    EnQ(ppid, &ready_q);
    pcb[ppid].trapframe_p -> eax = exit_code;

    pcb[run_pid].state = UNUSED;
    EnQ(run_pid, &pid_q);
    run_pid = NONE;
}

void ExecSR(int code, int arg)
{
    int x, tf_location, state = 0;
    char *code_page;
    char *stack_page;

    for(x = 0; x < PAGE_NUM; x++)
    {
        if(state == 2)
            break;

        if(page_user[x] == NONE)
        {
            if(state == 0)
            {
                code_page = (char *) (x * PAGE_SIZE + RAM);
                page_user[x] = run_pid;
                state++;
            }
            else if(state == 1)
            {
                stack_page = (char *) (x * PAGE_SIZE + RAM);
                page_user[x] = run_pid;
                state++;
            }
        }
    }

    MemCpy(code_page, (char *) code, PAGE_SIZE);
    Bzero(stack_page, PAGE_SIZE);

    MemCpy(stack_page + PAGE_SIZE - sizeof(int), (char *) &arg, sizeof(int));

    tf_location = (int) stack_page + PAGE_SIZE - (sizeof(int)*2) - sizeof(trapframe_t);

    MemCpy((char *) tf_location, (char *)pcb[run_pid].trapframe_p, sizeof(trapframe_t));
    pcb[run_pid].trapframe_p = (trapframe_t *) tf_location;

    pcb[run_pid].trapframe_p->efl = EF_DEFAULT_VALUE|EF_INTR;           // enables intr
    pcb[run_pid].trapframe_p->cs = get_cs();                            // dupl from CPU
    pcb[run_pid].trapframe_p->eip = (int) code_page; 
}

void SignalSR(int sig_num, int handler)
{
    pcb[run_pid].sigint_handler = handler;
}

void WrapperSR(int pid, int handler, int arg)
{
    trapframe_t temp_tf;

    MemCpy((char *) &temp_tf, (char *) pcb[pid].trapframe_p, sizeof(trapframe_t));

    (char *) pcb[pid].trapframe_p -= sizeof(int) * 3;
    MemCpy((char *) pcb[pid].trapframe_p, (char *) &temp_tf, sizeof(trapframe_t));
    MemCpy((char *) pcb[pid].trapframe_p + sizeof(trapframe_t) + (sizeof(int)*2), (char *) &arg, sizeof(int));
    MemCpy((char *) pcb[pid].trapframe_p + sizeof(trapframe_t) + sizeof(int), (char *) &handler, sizeof(int));
    MemCpy((char *) pcb[pid].trapframe_p + sizeof(trapframe_t), (char *) &pcb[pid].trapframe_p->eip, sizeof(unsigned int));

    pcb[pid].trapframe_p -> eip = (unsigned int) Wrapper;
}
