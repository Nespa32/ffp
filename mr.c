#include <stdlib.h>
#include <stdatomic.h>
#include <mr.h>

#define LIST_SIZE 1024

struct mr_entry {
	atomic_flag claim;
	void **tls;
	int index;
};

struct mr_entry *init_mr(int max_threads)
{
	struct mr_entry *array = calloc(max_threads, sizeof(struct mr_entry));
	for(int i=0; i<max_threads; i++){
		atomic_flag_clear(&(array[i].claim));
		array[i].tls = NULL;
	}
	return array;
}

int mr_thread_acquire(
		struct mr_entry *array,
		int max_threads)
{
	int i = 0;
	while(1){
		if(!atomic_flag_test_and_set(&(array[i].claim))){
			if(array[i].tls == NULL)
				array[i].tls = malloc(LIST_SIZE*sizeof(void *));
			return i;
		}
		i = (i+1) % max_threads;
	}
}

void mr_thread_release(
		struct mr_entry *array,
		int thread_id)
{
	atomic_flag_clear(&(array[thread_id].claim));
}

void mr_reclaim_node(
		void *node,
		struct mr_entry *array,
		int thread_id){
	free(array[thread_id].tls[array[thread_id].index]);
	array[thread_id].tls[array[thread_id].index] = node;
	array[thread_id].index = (array[thread_id].index + 1) % LIST_SIZE;
}

