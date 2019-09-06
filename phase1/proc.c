// proc.c, 159
// all user processes are coded here
// processes do not R/W kernel data or code, must use syscalls

#include "k-const.h"
#include "k-data.h"

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

void InitProc(void)
{
   while(1)
   {
      //show a dot at upper left corner on PC
      //wait for about half second
      ShowChar(0, 0, '.');
      Delay();

      //erase dot
      //wait for about half second
      ShowChar(0, 0, ' ');
      Delay();
   }
}

void UserProc(void)
{
   while(1)
   {
      ShowChar(run_pid, 0, run_pid/10 + '0');
      ShowChar(run_pid, 1, run_pid%10 + '0');
      Delay();

      ShowChar(run_pid, 0, ' ');
      ShowChar(run_pid, 1, ' ');
      Delay();
     
      //show 1st digit of my PID at row run_pid+1, 1st col
      //show 2nd digit of my PID at row run_pid+1, 2nd col
      //wait for about half second

      //erase above writing (one digit at a time)
      //wait for about half second
   }
}
