// proc.c, 159
// all user processes are coded here
// processes do not R/W kernel data or code, must use syscalls

#include "k-const.h"
#include "sys-call.h" // all service calls used below
#include "k-data.h"
#include "k-lib.h"
#include "k-include.h"

/*
void Delay(void)  // delay CPU for half second by 'inb $0x80'
{
   int i;
   //for...  // loop to try to delay CPU for about half second
   for(i = 0; i < LOOP/2; i++)
   {
       asm("inb $0x80");
   }
}

void ShowChar(int row, int col, char ch) // show ch at row, col
{
    unsigned short *p = VID_HOME;   // upper-left corner of display
    p += row * 80;
    p += col;
    *p = ch + VID_MASK;
}
*/

int vid_mux;


void Aout(int device)
{
    int my_pid = GetPidCall();
    int x;
    int rand_int;

    char str[] = "xx ( ) Hello, World!\n\r";
    str[0] = '0' + my_pid / 10;
    str[1] = '0' + my_pid % 10;
    str[4] = 'a' + (my_pid-1);

    WriteCall(device, str);

    PauseCall();

    for(x = 0; x < 69; x++)
    {
        ShowCharCall(my_pid, x, 'a' + (my_pid-1));

        rand_int = (RandCall() % 19) + 5;

        SleepCall(rand_int);
        ShowCharCall(my_pid, x, ' ');
    }

    ExitCall(my_pid * 100);
}

void Ouch(int device)
{
    WriteCall(device, "Can't touch that!\n\r");
}

void Wrapper(int handler, int arg)
{
    func_p_t2 func = (func_p_t2)handler;

    asm("pushal");
    func(arg);
    asm("popal");
    asm("movl %%ebp, %%esp; popl %%ebp; ret $8"::);
}

void InitTerm(int term_no)
{
    int i, j;

    Bzero((char *)&term[term_no].out_q, sizeof(q_t));
    Bzero((char *)&term[term_no].in_q, sizeof(q_t));
    Bzero((char *)&term[term_no].echo_q, sizeof(q_t));
    term[term_no].out_mux = MuxCreateCall(Q_SIZE);
    term[term_no].in_mux = MuxCreateCall(0);

    outportb(term[term_no].io_base+CFCR, CFCR_DLAB);                // CFCR_DLAB is 0x80
    outportb(term[term_no].io_base+BAUDLO, LOBYTE(115200/9600));    // period of each of 9600 bauds
    outportb(term[term_no].io_base+BAUDHI, HIBYTE(115200/9600));
    outportb(term[term_no].io_base+CFCR, CFCR_PEVEN|CFCR_PENAB|CFCR_7BITS);

    outportb(term[term_no].io_base+IER, 0);
    outportb(term[term_no].io_base+MCR, MCR_DTR|MCR_RTS|MCR_IENABLE);
    
    for(i=0; i<LOOP/2; i++)asm("inb $0x80");
    
    outportb(term[term_no].io_base+IER, IER_ERXRDY|IER_ETXRDY); // enable TX & RX intr
    
    for(i=0; i<LOOP/2; i++)asm("inb $0x80");

    for(j=0; j<25; j++)
    {   
        // clear screen, sort of
        outportb(term[term_no].io_base+DATA, '\n');
        for(i=0; i<LOOP/30; i++)asm("inb $0x80");
        outportb(term[term_no].io_base+DATA, '\r');
        for(i=0; i<LOOP/30; i++)asm("inb $0x80");
    }

    /*
    inportb(term_term_no].io_base); // clear key cleared PROCOMM screen
    for(i=0; i<LOOP/2; i++)asm("inb $0x80");
    */
}

void InitProc(void)
{
    int i;
        
    vid_mux = MuxCreateCall(1);

    InitTerm(0);
    InitTerm(1);

    while(1)
    {
        //show a dot at upper left corner on PC
        //wait for about half second
        ShowCharCall(0, 0, '.');
        for(i = 0;i <LOOP/2; i++) 
        {
            asm("inb $0x80");
        } 

        //erase dot
        //wait for about half second
        ShowCharCall(0, 0, ' ');
        for(i = 0; i<LOOP/2; i++)
        {
            asm("inb $0x80");
        }
    }
}

void UserProc(void)
{  
    int device;
    int my_pid = GetPidCall();   // get my PID
    int x;
    int y;

    char str1[STR_SIZE] = "PID    > ";
    char str2[STR_SIZE];

    str1[4] = '0' + my_pid / 10;
    str1[5] = '0' + my_pid % 10;

    device = my_pid % 2 == 1? TERM0_INTR : TERM1_INTR;

    SignalCall(SIGINT, (int) Ouch);

    while(1)
    {
        int fork_return;
        int exit_code;
        char child_pid_str[STR_SIZE];
        char exit_code_str[STR_SIZE];
        char fork_err_str[STR_SIZE] = "Couldn't fork!";
        char child_str[STR_SIZE] = "Child PID: ";
        char nr_str[STR_SIZE] = "\n\r";
        char child_exit_str[STR_SIZE] = "Child Exit Code: ";
        char alphabet[STR_SIZE];

        WriteCall(device, str1);  // prompt for terminal input
        ReadCall(device, str2);   // read terminal input
        
        if(!StrCmp(str2, "race\0"))
            continue;
        
        for(x = 0; x < 5; x++)
        {
            fork_return = ForkCall();
            if(fork_return == NONE)
            {
                WriteCall(device, fork_err_str);
                continue;
            }

            if(fork_return == 0)
            {
                ExecCall((int) Aout, device);
            }
            else
            {
                // Print "Child PID: 00" to terminal
                //child_str[11] = '0' + fork_return / 10;
                //child_str[12] = '0' + fork_return % 10;
                Itoa(child_pid_str, fork_return);
                WriteCall(device, child_str);
                WriteCall(device, child_pid_str);
                WriteCall(device, nr_str);
            
                // Print "Child Exit Code: 000" to terminal
            }
        }

        SleepCall(300);
        KillCall(0, SIGGO);

        for(y = 0; y < 5; y++)
        {
            exit_code = WaitCall();
             
            //child_exit_str[17] = '0' + exit_code / 100;
            //child_exit_str[18] = '0' + exit_code / 10;
            //child_exit_str[19] = '0' + exit_code % 10;
            Itoa(exit_code_str, exit_code);
            WriteCall(device, child_exit_str);
            WriteCall(device, exit_code_str);

            alphabet[0] = 'a' + ((exit_code/100)-1);

            WriteCall(device, " ");
            WriteCall(device, alphabet);
            WriteCall(device, " arrives!");
            WriteCall(device, nr_str);
        }
    }
}
