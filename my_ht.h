#ifndef MY_HT
#define MY_HT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ck_ht.h>
#include <ck_pr.h>

#define HT_SEED 4040230

int hashtable_init();
int hashtable_add(char *);
int hashtable_get_count(char *);
int hashtable_get_total();
void hashtable_print_all();
void hashtable_print_result();
void hashtable_cleanup();

#endif /* MY_HT */
