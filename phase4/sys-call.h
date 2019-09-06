// sys-call.h

#ifndef __SYS_CALL__
#define __SYS_CALL__

int GetPidCall(void);
void ShowCharCall(int row, int col, char ch);
void SleepCall(int centi_sec);
int MuxCreateCall(int flag);
void MuxOpCall(int mux_id, int opcode);
void WriteCall(int device, char* str);

#endif

