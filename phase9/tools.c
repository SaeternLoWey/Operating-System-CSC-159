// k-lib.c, 159

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"

// clear DRAM data block, zero-fill it
void Bzero(char *p, int bytes)
{
    int i;
    for(i = 0; i < bytes; i++)
    {
        p[i] = 0;
        //*p = '\0';
	//p++;
    }
}

int QisEmpty(q_t *p)
{ 
    // return 1 if empty, else 0
    if(p -> tail == 0)
        return 1;
    else
        return 0;
    
}

int QisFull(q_t *p)
{
    // return 1 if full, else 0
    if(p -> tail == Q_SIZE)
        return 1;
    else
        return 0;
}

// dequeue, 1st # in queue; if queue empty, return -1
// move rest to front by a notch, set empty spaces -1
int DeQ(q_t *p)
{
    // return -1 if q[] is empty
    int i, to_return;

    if(QisEmpty(p))
    {
        cons_printf("Panic: queue is empty, cannot DeQ\n");
        return NONE; // or  breakpoint();
    }

    to_return = p->q[0];
    p->tail--;
    
    for(i = 0; i < p -> tail; i++)
        p -> q[i] = p -> q[i+1];

    for(i = p -> tail; i < Q_SIZE; i++)
        p -> q[i] = NONE;
    
    return to_return;
}   

// if not full, enqueue # to tail slot in queue
void EnQ(int to_add, q_t *p)
{
    if(QisFull(p))
    {
        cons_printf("Panic: queue is full, cannot EnQ!\n");
        return;
    }

    p -> q[p -> tail] = to_add;
    p -> tail++;
    
    // this was in the professor's pseudo code but causes a segfault...
    // p -> q[p -> tail] = NONE;
}

void MemCpy(char *dest, char *source, int size)
{
    int x;
    for(x = 0; x < size; x++)
    {
        dest[x] = source[x];
    }            
}

int StrCmp(char *str1, char *str2)
{
    while(1)
    {
        if(*str1 != *str2)
        {
            return FALSE;
        }

        if(*str1 == '\0')
        {
            return TRUE;
        }
        
        str1++;
        str2++;
    }
}

void Itoa(char *str, int num)
{
    int digit;
    int x = 0;
    int y = 0;
    int tmp;

    if(num == 0)
    {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    while(num != 0)
    {
        digit = num % 10;
        str[x] = '0' + digit;
        x++;

        num = num / 10;
    }

    str[x] = '\0';
    x -= 1;
    while(y < x)
    {
        tmp = str[y];
        str[y] = str[x];
        str[x] = tmp;

        y++;
        x--;
    }
}
