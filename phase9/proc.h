// proc.h, 159

#ifndef __PROC__
#define __PROC__

//void Delay(void);  // prototype those in proc.c here
//void ShowChar(int row, int col, char ch);
void InitTerm(int term_no);
void InitProc(void);
void UserProc(void);
void Aout(int device);
void Ouch(int device);
void Wrapper(int handler, int arg);
#endif
