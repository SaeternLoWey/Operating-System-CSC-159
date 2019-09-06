// main.c, 159
// OS Phase 3
//
// Team Name: SMURF_OS (Members: Aleksey Zasorin, Lowey Saetern)

#include "k-include.h"  // SPEDE includes
#include "k-entry.h"    // entries to kernel (TimerEntry, etc.)
#include "k-type.h"     // kernel data types
#include "k-lib.h"      // small handy functions
#include "k-sr.h"       // kernel service routines
#include "proc.h"       // all user process code here

// kernel data are all here:
int run_pid;                                    // current running PID; if -1, none selected
int sys_centi_sec;
q_t pid_q, ready_q, sleep_q, mux_q;             // avail PID and those created/ready to run
pcb_t pcb[PROC_SIZE];                           // Process Control Blocks
char proc_stack[PROC_SIZE][PROC_STACK_SIZE];    // process runtime stacks
struct i386_gate *intr_table;                   // intr table's DRAM location

mux_t mux[MUX_SIZE];

term_t term[TERM_SIZE] = {
    { TRUE, TERM0_IO_BASE },
    { TRUE, TERM1_IO_BASE }
};

int page_user[PAGE_NUM];

unsigned rand = 0;

int kernel_main_table;


void InitKernelData(void)
{
    // init kernel data
    int i;
      
    intr_table = get_idt_base();            // get intr table location

    Bzero((char *)&pid_q, sizeof(q_t));     // clear 2 queues
    Bzero((char *)&ready_q, sizeof(q_t));   // clear 2 queues
    Bzero((char *)&sleep_q, sizeof(q_t));   // clear the sleep queue
    Bzero((char *)&mux_q, sizeof(q_t));     // clear the mutex queue

    for(i=0; i<PROC_SIZE; i++)
        EnQ(i, &pid_q);

    for(i=0; i<MUX_SIZE; i++)
        EnQ(i, &mux_q);
    
    for(i=0; i<PAGE_NUM; i++)
        page_user[i] = NONE;

    run_pid = NONE;

    sys_centi_sec = 0;

    kernel_main_table = get_cr3();
}

void InitKernelControl(void)      // init kernel control
{
    // fill out intr table for timer
    fill_gate(&intr_table[TIMER_INTR], (int) TimerEntry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[GETPID_CALL], (int) GetPidEntry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[SHOWCHAR_CALL], (int) ShowCharEntry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[SLEEP_CALL], (int) SleepEntry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[MUX_CREATE_CALL], (int) MuxCreateEntry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[MUX_OP_CALL], (int) MuxOpEntry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[TERM0_INTR], (int) Term0Entry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[TERM1_INTR], (int) Term1Entry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[FORK_CALL], (int) ForkEntry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[WAIT_CALL], (int) WaitEntry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[EXIT_CALL], (int) ExitEntry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[EXEC_CALL], (int) ExecEntry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[SIGNAL_CALL], (int) SignalEntry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[PAUSE_CALL], (int) PauseEntry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[KILL_CALL], (int) KillEntry, get_cs(), ACC_INTR_GATE, 0);
    fill_gate(&intr_table[RAND_CALL], (int) RandEntry, get_cs(), ACC_INTR_GATE, 0);

    outportb(PIC_MASK, MASK);                   // mask out PIC for timer
}

void Scheduler(void)      // choose run_pid
{
    //ask about this?
    if(run_pid > 0)
        return;

    if(QisEmpty(&ready_q))
    {
        run_pid = 0;     // pick InitProc
    }
    else
    {
        pcb[0].state = READY;
        run_pid = DeQ(&ready_q);
    }
    pcb[run_pid].run_count = 0;
    pcb[run_pid].state = RUN;
}

int main(void)          // OS bootstraps
{
    InitKernelData();
    InitKernelControl();

    NewProcSR(InitProc);
    Scheduler();
 
    set_cr3(pcb[run_pid].main_table);
    Loader(pcb[run_pid].trapframe_p); // load/run it

    return 0; // statement never reached, compiler asks it for syntax
}

void Kernel(trapframe_t *trapframe_p)           // kernel runs
{
    char ch;

    pcb[run_pid].trapframe_p = trapframe_p; // save it

    switch(trapframe_p -> entry_id)
    {
	case SLEEP_CALL:
	    SleepSR(trapframe_p -> eax);
	    break;
	case GETPID_CALL:
	    trapframe_p -> eax = GetPidSR();
	    break;
	case SHOWCHAR_CALL:
	    ShowCharSR(trapframe_p->eax, trapframe_p->ebx, trapframe_p->ecx);
	    break;
	case TIMER_INTR:
	    TimerSR();                     // handle timer intr
	    break;
        case MUX_CREATE_CALL:        
            trapframe_p -> eax = MuxCreateSR(trapframe_p->eax);
            break;
        case MUX_OP_CALL:
            MuxOpSR(trapframe_p->eax, trapframe_p->ebx);
            break;
        case TERM0_INTR:
            TermSR(0);
            outportb(PIC_CONTROL, TERM0_DONE);
            break;
        case TERM1_INTR:
            TermSR(1);
            outportb(PIC_CONTROL, TERM1_DONE);
            break;
        case FORK_CALL:
            trapframe_p -> eax = ForkSR();
            break;
        case WAIT_CALL:
            trapframe_p -> eax = WaitSR();
            break;
        case EXIT_CALL:
            ExitSR(trapframe_p->eax);
            break;
        case EXEC_CALL:
            ExecSR(trapframe_p->eax, trapframe_p->ebx);
            break;
        case SIGNAL_CALL:
            SignalSR(trapframe_p->eax, trapframe_p->ebx);
            break;
        case PAUSE_CALL:
            PauseSR();
            break;
        case KILL_CALL:
            KillSR(trapframe_p->eax, trapframe_p->ebx);
            break;
        case RAND_CALL:
            trapframe_p -> eax = RandSR();
            break;
	default:
	    cons_printf("This print statement should never be hit");
	    break;
    }

    if(cons_kbhit())
    {
        ch = cons_getchar();
        if (ch == 'b')
        {
            breakpoint();
        }
        else if (ch == 'n')          // 'n' for new process
        {
            if(rand == 0)
                rand = sys_centi_sec;

            NewProcSR(UserProc);     // create a UserProc
        }
    }
   
    Scheduler();   // may need to pick another proc

    set_cr3(pcb[run_pid].main_table);
    Loader(pcb[run_pid].trapframe_p);
}
