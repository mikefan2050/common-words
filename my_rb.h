#ifndef MY_RB
#define MY_RB

#include <stdlib.h>
#include <ck_ring.h>
#include <ck_pr.h>

#define BUF_SIZE 4194304

int ring_buffer_init();
int ring_buffer_enqueue(char *);
int ring_buffer_dequeue(char **);
int ring_buffer_size();
void ring_buffer_cleanup();

#endif /* MY_RB */
