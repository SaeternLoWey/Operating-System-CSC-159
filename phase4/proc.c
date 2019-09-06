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


void InitTerm(int term_no)
{
    int i, j;

    Bzero((char *)&term[term_no].out_q, sizeof(q_t));
    term[term_no].out_mux = MuxCreateCall(Q_SIZE);

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
    int which_term;
    int my_pid = GetPidCall();   // get my PID

    char str1[] = "PID    process is running exclusively using the video display...";
    char str2[] = "                                                                                "; 

    str1[4] = '0' + my_pid / 10;
    str1[5] = '0' + my_pid % 10;

    which_term = my_pid % 2 == 1? TERM0_INTR : TERM1_INTR;

    while(1)
    {
	MuxOpCall(vid_mux, LOCK);
	WriteCall(STDOUT, str1);
        WriteCall(which_term, str1);
        WriteCall(which_term, "\n\r");
        SleepCall(50);

        WriteCall(STDOUT, str2);
        SleepCall(50);
        MuxOpCall(vid_mux, UNLOCK);
	
        //ShowCharCall(my_pid, 0, my_pid/10 + '0');
        //ShowCharCall(my_pid, 1, my_pid%10 + '0');
        //SleepCall(50);

        //ShowCharCall(my_pid, 0, ' ');
        //ShowCharCall(my_pid, 1, ' ');
        //SleepCall(50);
    }
}
