// k-lib.h, 159

#ifndef __K_LIB__
#define __K_LIB__

#include "k-type.h"

void Bzero(char *p, int bytes);     // prototype those in k-lib.c here
int QisEmpty(q_t *p);
int QisFull(q_t *p);
int DeQ(q_t *p);
int EnQ(int to_add, q_t *p);

#endif
