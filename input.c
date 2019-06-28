#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ck_pr.h>
#include "my_rb.h"
#include "my_ht.h"

#define THD_NUM 2

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static int finished;

static void *reader_fun(void *arg)
{
	int status, result;
	char *word;

	// wait for the cond
	pthread_mutex_lock(&mutex);
	// to ensure the thread is registered to cond
	finished++;
	pthread_cond_wait(&cond, &mutex);
	pthread_mutex_unlock(&mutex);

	// start the processing
	word = NULL;

	while (ck_pr_load_int(&finished) == 0) {
		status = ring_buffer_dequeue(&word);
		if (!status) {
			result = hashtable_add(word);
			if (result) return (void *)-3;
		}
	}

	return 0;
}

static void file_process(char *fname, int first)
{
	FILE *f;
	char tbuffer[1000], *word;
	int status;

	char delim[] = "\n\t ,.!?\"\'";

	f = fopen(fname, "r");
	if (f == NULL) {
		fprintf(stderr, "cannot open %s\n", fname);
		exit(1);
	}
	memset(tbuffer, 0, 1000);

	if (first) {
		// wait for processing threads to register
		while (ck_pr_load_int(&finished) != THD_NUM);
		finished = 0;
		// wake up processing threads
		pthread_mutex_lock(&mutex);
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mutex);
	}


	while (fgets(tbuffer, 1000, f)) {
		char *pt = strtok(tbuffer, delim);
		while (pt) {
			word = malloc(sizeof(char) * strlen(pt));
			if (word == NULL) {
				fprintf(stderr, "word malloc error %s\n", pt);
				fclose(f);
				exit(1);
			}
			strncpy(word, pt, strlen(pt));
			status = ring_buffer_enqueue(word);
			if (status) {
				fprintf(stderr, "word rb enqueue error %s\n", word);
				fclose(f);
				exit(1);
			}
			pt = strtok(NULL, delim);
		}
	}
	if (fclose(f)) exit(1);
}


int main(int argc, char **argv)
{
	int status, i;
	char *file1, *file2;
	pthread_t readers[THD_NUM];

	if (argc != 3) {
		fprintf(stderr, "usage: ./common_finder <file1> <file2>\n");
		exit(1);
	}

	file1 = argv[1];
	file2 = argv[2];

	if (access(file1, R_OK)) {
		fprintf(stderr, "file not exist: %s\n", file1);
		exit(1);
	}
	if (access(file2, R_OK)) {
		fprintf(stderr, "file not exist: %s\n", file2);
		exit(1);
	}

	finished = 0;

	status = ring_buffer_init();
	if (status) {
		fprintf(stderr, "error: (rb) %d\n", status);
		exit(1);
	}

	status = hashtable_init();
	if (status) {
		fprintf(stderr, "error: (ht) %d\n", status);
		exit(1);
	}

	for (i = 0; i < THD_NUM; i++) {
		pthread_create(&readers[i], NULL, reader_fun, NULL);
	}

	file_process(file1, 1);
	file_process(file2, 0);

	// while size > 0, spin
	while (ring_buffer_size() > 0);

	// send signal to kill the threads
	ck_pr_add_int(&finished, 1);

	for (i = 0; i < THD_NUM; i++) {
		pthread_join(readers[i], NULL);
	}

	// cleanup code
	hashtable_print_result();
	ring_buffer_cleanup();

	return 0;
}
