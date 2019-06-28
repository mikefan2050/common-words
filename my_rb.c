#include <stdio.h>
#include "my_rb.h"

static struct rb_info {
	ck_ring_t rb;
	ck_ring_buffer_t buffer[BUF_SIZE];
} *ring_buffer_info;

int ring_buffer_init()
{
	ring_buffer_info = malloc(sizeof(struct rb_info));
	if (ring_buffer_info == NULL)
		return -1;

	ck_ring_init(&ring_buffer_info->rb, BUF_SIZE);

	memset(ring_buffer_info->buffer, 0, sizeof(ring_buffer_info->buffer));

	return 0;
}

int ring_buffer_enqueue(char *word)
{
	if (ring_buffer_info == NULL)
		return -1;
	if (ck_ring_enqueue_spmc(&ring_buffer_info->rb, ring_buffer_info->buffer, word))
		return 0;
	while (ck_ring_enqueue_spmc(&ring_buffer_info->rb, ring_buffer_info->buffer, word) == false)
		ck_pr_stall();
	return 0;
}

int ring_buffer_dequeue(char **result)
{
	if (ck_ring_trydequeue_spmc(&ring_buffer_info->rb, ring_buffer_info->buffer, result) == false)
		return -1;

	return 0;
}

int ring_buffer_size()
{
	if (ring_buffer_info == NULL)
		return -1;
	return ck_ring_size(&ring_buffer_info->rb);
}

void ring_buffer_cleanup()
{
	free(ring_buffer_info);
}
