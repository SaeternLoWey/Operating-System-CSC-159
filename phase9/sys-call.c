// sys-call.c
// calls to kernel services

#include "k-const.h"
#include "k-data.h"
#include "k-lib.h"

int GetPidCall(void)
{
   int pid;

   asm("int %1;             // interrupt!
        movl %%eax, %0"     // after, copy eax to variable 'pid'
       : "=g" (pid)         // output
       : "g" (GETPID_CALL)  // input
       : "eax"              // used registers
   );

   return pid;
}

void ShowCharCall(int row, int col, char ch)
{
   asm("movl %0, %%eax;     // send in row via eax
        movl %1, %%ebx;            // send in col via ebx
        movl %2, %%ecx;            // send in ch via ecx
        int %3"             // initiate call, %3 gets entry_id
       :                    // no output
       : "g" (row), "g" (col), "g" (ch), "g" (SHOWCHAR_CALL)
       : "eax", "ebx", "ecx"         // affected/used registers
   );
}

void SleepCall(int centi_sec)  // # of 1/100 of a second to sleep
{
   asm("movl %0, %%eax;
        int %1"
        :
        : "g" (centi_sec), "g" (SLEEP_CALL)
        : "eax"
   );       
}

int MuxCreateCall(int flag)
{   
    int mux_id;

    asm("movl %1, %%eax;    // send in flag
         int %2;             // call interupt
         movl %%eax, %0"    // return mux id (set by SR)
        : "=g" (mux_id)     // output the mux ID
        : "g" (flag), "g" (MUX_CREATE_CALL) //input
        : "eax"
    );

    return mux_id;
}

void MuxOpCall(int mux_id, int opcode)
{
    asm("movl %0, %%eax;    //send in mux_id
         movl %1, %%ebx;    //send in opcode
         int %2"            //call interupt
        :                                               //output
        : "g" (mux_id), "g" (opcode), "g" (MUX_OP_CALL) //input
        : "eax", "ebx"
    );
}

void WriteCall(int device, char* str)
{    
    int col;
    int row;
    row = GetPidCall();
    col = 0;
    if(device == STDOUT)
    {          
        while(*str != '\0')
        {    
            ShowCharCall(row, col, *str);
            col++;
            str++;
        }
    }
    else
    {
        int term_no;
        if(device == TERM0_INTR)
        {
            term_no = 0;
        }
        else if(device == TERM1_INTR)
        {
            term_no = 1;
        }

        while(*str != '\0')
        {
            MuxOpCall(term[term_no].out_mux, LOCK);
            EnQ((int) *str, &term[term_no].out_q);

            if(device == TERM0_INTR)
            {
                asm("int $35");
            }
            else if(device == TERM1_INTR)
            {
                asm("int $36");
            }

            str++;
        }
    }
}

void ReadCall(int device, char *str)
{
    int term_no;
    int chcount;

    if(device == TERM0_INTR)
    {
        term_no = 0;
    }

    else if(device ==TERM1_INTR)
    {
        term_no = 1;
    }

    chcount = 0;

    while(1)
    {
        char ch;
        MuxOpCall(term[term_no].in_mux, LOCK);
        ch = DeQ(&term[term_no].in_q);
        *str = ch;
        
        if(ch == '\0')
        {
	    return;
        }
        
        str++;
        chcount++;

	if(chcount == STR_SIZE -1)
	{
	    *str = '\0';
            return;
	}
    }
}

int ForkCall(void)
{
   int child_pid;

   asm("int %1;             // interrupt!
        movl %%eax, %0"     // after, copy eax to variable 'child_pid'
       : "=g" (child_pid)   // output
       : "g" (FORK_CALL)    // input
       : "eax"              // used registers
   );

   return child_pid;    
}

int WaitCall(void)
{
   int exit_code;

   asm("int %1;             // interrupt!
        movl %%eax, %0"     // after, copy eax to variable 'pid'
       : "=g" (exit_code)   // output
       : "g" (WAIT_CALL)    // input
       : "eax"              // used registers
   );
   
   return exit_code;

}

void ExitCall(int exit_code)
{    
     asm("movl %0, %%eax;
          int %1"
        :                                   //no output
        : "g" (exit_code), "g" (EXIT_CALL)
        : "eax"
     );
}

void ExecCall(int code, int arg)
{
    asm("movl %0, %%eax;
         movl %1, %%ebx;
         int %2"
       :
       : "g" (code),
         "g" (arg),
         "g" (EXEC_CALL)
       : "eax", "ebx"
    );
}

void SignalCall(int sig_num, int handler)
{
    asm("movl %0, %%eax;
         movl %1, %%ebx;
         int %2"
       :
       : "g" (sig_num),
         "g" (handler),
         "g" (SIGNAL_CALL)
       : "eax", "ebx"
    );
}

void PauseCall(void)
{
    asm("int %0"
       :
       : "g" (PAUSE_CALL)
    );
}

void KillCall(int pid, int sig_num)
{
    asm("movl %0, %%eax;
         movl %1, %%ebx;
         int %2"
       :
       : "g" (pid),
         "g" (sig_num),
         "g" (KILL_CALL)
       : "eax", "ebx"
    );
}

unsigned RandCall(void)
{
    unsigned ret;

    asm("int %1;
         movl %%eax, %0"
       : "=g" (ret)
       : "g" (RAND_CALL)
       : "eax"
    );

    return ret;
}
