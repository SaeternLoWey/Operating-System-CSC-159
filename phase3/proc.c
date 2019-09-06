// proc.c, 159
// all user processes are coded here
// processes do not R/W kernel data or code, must use syscalls

#include "k-const.h"
#include "sys-call.h" // all service calls used below
#include "k-data.h"
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



void InitProc(void)
{
    int i;
        
    vid_mux = MuxCreateCall(1);
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
    int my_pid = GetPidCall();   // get my PID

    char str1[] = "PID    process is running exclusively using the video display...";
    char str2[] = "                                                                                "; 

    str1[4] = '0' + my_pid / 10;
    str1[5] = '0' + my_pid % 10;

    while(1)
    {
	MuxOpCall(vid_mux, LOCK);
	WriteCall(STDOUT, str1);
  SleepCall(50);

  WriteCall(STDOUT, str2);
  SleepCall(50);
  MuxOpCall(vid_mux, UNLOCK);
	
      //  ShowCharCall(my_pid, 0, my_pid/10 + '0');
      //  ShowCharCall(my_pid, 1, my_pid%10 + '0');
      //  SleepCall(500);

      //  ShowCharCall(my_pid, 0, ' ');
      //  ShowCharCall(my_pid, 1, ' ');
      //  SleepCall(500);
    }
}
