#include "my_ht.h"

static struct ht_info {
	int total;
	ck_ht_t hashtable;
	pthread_mutex_t lock;
} *table_info;

static void * ht_malloc(size_t sz)
{
	return malloc(sz);
}

static void ht_free(void *pt, size_t sz, bool b)
{
	(void)sz;
	(void)b;
	free(pt);
	return;
}

static struct ck_malloc allocator = {
	.malloc = ht_malloc,
	.free = ht_free,
};

// return 0 for success, -1 for failure
int hashtable_init()
{
	table_info = malloc(sizeof(struct ht_info));
	if (table_info == NULL)
		goto cleanup;

	if (pthread_mutex_init(&table_info->lock, NULL))
		goto cleanup;

	if (!ck_ht_init(&table_info->hashtable, CK_HT_MODE_BYTESTRING, NULL, &allocator, 500, HT_SEED))
		goto cleanup_lock;

	table_info->total = 0;

	return 0;

cleanup_lock:
	pthread_mutex_destroy(&table_info->lock);
cleanup:
	if (table_info != NULL) free(table_info);
	return -1;
}

int hashtable_add(char *word)
{
	int wlen;
	ck_ht_hash_t hash;
	ck_ht_entry_t entry;
	void *value;

	if (word == NULL)
		return -1;

	// calculate hash value
	wlen = strlen(word);
	ck_ht_hash(&hash, &table_info->hashtable, word, wlen);

	// set entry for lookup
	ck_ht_entry_key_set(&entry, word, wlen);
	if (!ck_ht_get_spmc(&table_info->hashtable, hash, &entry)) {
		// word not found, need to add a new entry

		pthread_mutex_lock(&table_info->lock);

		// double check if still not exists
		if (!ck_ht_get_spmc(&table_info->hashtable, hash, &entry)) {
			int *count;
			count = malloc(sizeof(int));
			if (count == NULL) {
				pthread_mutex_unlock(&table_info->lock);
				return -2;
			}
			*count = 1;
			// store the new entry
			ck_ht_entry_set(&entry, hash, word, wlen, count);
			if (!ck_ht_put_spmc(&table_info->hashtable, hash, &entry)) {
				// cannot store the new entry
				free(count);
				pthread_mutex_unlock(&table_info->lock);
				return -3;
			}
			// increment the total by 1
			ck_pr_add_int(&table_info->total, 1);
			ck_pr_barrier();
			pthread_mutex_unlock(&table_info->lock);
			return 0;
		}

		pthread_mutex_unlock(&table_info->lock);
	}

	// word found, atomically increment by 1
	value = ck_ht_entry_value(&entry);
	ck_pr_add_int(value, 1);
	ck_pr_barrier();
	// increment the total by 1
	ck_pr_add_int(&table_info->total, 1);
	ck_pr_barrier();
	return 0;
}

int hashtable_get_count(char *word)
{
	int wlen;
	ck_ht_entry_t entry;
	ck_ht_hash_t hash;
	if (word == NULL)
		return -2;
	wlen = strlen(word);
	ck_ht_hash(&hash, &table_info->hashtable, word, wlen);
	ck_ht_entry_key_set(&entry, word, wlen);

	if (ck_ht_get_spmc(&table_info->hashtable, hash, &entry)) {
		// word found
		return *(int *)ck_ht_entry_value(&entry);
	} else {
		// not in the table
		return 0;
	}
}

int hashtable_get_total()
{
	return table_info->total;
}

// not thread safe
void hashtable_print_all()
{
	int i;
	ck_ht_entry_t *cursor;
	ck_ht_iterator_t iterator = CK_HT_ITERATOR_INITIALIZER;
	ck_ht_iterator_init(&iterator);
	for (i = 0; ck_ht_next(&table_info->hashtable, &iterator, &cursor) == true; i++) {
			void *k, *v;
			k = ck_ht_entry_key(cursor);
			v = ck_ht_entry_value(cursor);
			printf("%s: ", (char *)k);
			printf("%d\n", *(int *)v);
	}
	printf("num of words = %d\n", i);
	printf("total count = %d\n", table_info->total);
}

// print the most common word
// not thread safe
void hashtable_print_result()
{
	int i, max, *v;
	char *word, *k;
	ck_ht_entry_t *cursor;
	ck_ht_iterator_t iterator = CK_HT_ITERATOR_INITIALIZER;

	max = 0;
	word = NULL;

	ck_ht_iterator_init(&iterator);
	for (i = 0; ck_ht_next(&table_info->hashtable, &iterator, &cursor) == true; i++) {
			k = ck_ht_entry_key(cursor);
			v = ck_ht_entry_value(cursor);
			if (*v > max) {
				max = *v;
				word = k;
			}
	}
	printf("most common: %s (%d)\n", word, max);
}

void hashtable_cleanup()
{
	// free(k);
	// free(v);
	ck_ht_destroy(&table_info->hashtable);
	pthread_mutex_destroy(&table_info->lock);
	free(table_info);
}
