// main.c, 159
// OS Phase 1
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
q_t pid_q, ready_q;                             // avail PID and those created/ready to run
pcb_t pcb[PROC_SIZE];                           // Process Control Blocks
char proc_stack[PROC_SIZE][PROC_STACK_SIZE];    // process runtime stacks
struct i386_gate *intr_table;                   // intr table's DRAM location

void InitKernelData(void)
{
    // init kernel data
    int i;
      
    intr_table = get_idt_base();            // get intr table location

    Bzero((char *)&pid_q, sizeof(q_t));     // clear 2 queues
    Bzero((char *)&ready_q, sizeof(q_t));   // clear 2 queues
    
    for(i=0; i<PROC_SIZE; i++)
        EnQ(i, &pid_q);

    run_pid = NONE;
}

void InitKernelControl(void)      // init kernel control
{
   fill_gate(&intr_table[TIMER_INTR], (int) TimerEntry, get_cs(), ACC_INTR_GATE, 0);                  // fill out intr table for timer
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
   Loader(pcb[run_pid].trapframe_p); // load/run it

   return 0; // statement never reached, compiler asks it for syntax
}

void Kernel(trapframe_t *trapframe_p)           // kernel runs
{
    char ch;

    pcb[run_pid].trapframe_p = trapframe_p; // save it

    TimerSR();                     // handle timer intr

    if(cons_kbhit())
    {
        ch = cons_getchar();
        if (ch == 'b')
            breakpoint();
        else if (ch == 'n')         // 'n' for new process
            NewProcSR(UserProc);     // create a UserProc
    }
   
    Scheduler();   // may need to pick another proc
    Loader(pcb[run_pid].trapframe_p);
}
