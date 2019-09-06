// k-sr.h, 159

#ifndef __K_SR__
#define __K_SR__

#include "k-type.h"

void NewProcSR(func_p_t p);  // prototype those in k-sr.c here
void CheckWakeProc(void);
void TimerSR(void);
void SleepSR(int centi_sec);
int GetPidSR();
void ShowCharSR(int row, int col, char ch);
int MuxCreateSR(int flag);
void MuxOpSR(int mux_id, int opcode);
#endif
