#include <stdlib.h>
#include <stdatomic.h>
#include <sys/queue.h>
#include <mr.h>

#define CACHE_LINE_SIZE 64

STAILQ_HEAD(stailhead, entry);

struct entry {
	void *node;
	unsigned long long rm_time;
	STAILQ_ENTRY(entry) entries;
};

struct mr_entry {
	atomic_flag claim;
	atomic_ullong clock;
	struct stailhead *head;
} __attribute__((aligned(CACHE_LINE_SIZE)));

struct mr_entry *init_mr(int max_threads)
{
	struct mr_entry *array = calloc(max_threads, sizeof(struct mr_entry));
	for(int i=0; i<max_threads; i++){
		atomic_store(&(array[i].clock), 0);
		array[i].head = malloc(sizeof(struct stailhead));
		STAILQ_INIT(array[i].head);
		atomic_flag_clear(&(array[i].claim));
	}
	return array;
}

unsigned long long get_lamport_time(
		struct mr_entry *array,
		int max_threads)
{
	unsigned long long max = 0,
			   tmp;
	for(int i=0; i<max_threads; i++){
		if(atomic_flag_test_and_set_explicit(
					&(array[i].claim),
					memory_order_relaxed)){
			tmp = atomic_load_explicit(
					&(array[i].clock),
					memory_order_acquire);
			if(tmp > max)
				max = tmp;
		}
		else{
			atomic_flag_clear_explicit(
					&(array[i].claim),
					memory_order_relaxed);
		}
	}
	return max + 1;
}

unsigned long long get_min_time(
		struct mr_entry *array,
		int max_threads)
{
	unsigned long long min = ~0,
			   tmp;
	for(int i=0; i<max_threads; i++){
		if(atomic_flag_test_and_set_explicit(
					&(array[i].claim),
					memory_order_relaxed)){
			tmp = atomic_load_explicit(
					&(array[i].clock),
					memory_order_acquire);
			if(tmp < min)
				min = tmp;
		}
		else{
			atomic_flag_clear_explicit(
					&(array[i].claim),
					memory_order_relaxed);
		}
	}
	return min;
}

void free_nodes(
		struct stailhead *head,
		unsigned long long time)
{
	struct entry *iter,
		     *iter_tmp = NULL;
	STAILQ_FOREACH(iter, head, entries){
		if(iter_tmp){
			free(iter_tmp);
			iter_tmp = NULL;
		}
		if(iter->rm_time < time){
			free(iter->node);
			STAILQ_REMOVE(head, iter, entry, entries);
			iter_tmp = iter;
		}
	}
}

int mr_thread_acquire(
		struct mr_entry *array,
		int max_threads)
{
	int i = 0;
	while(1){
		if(!atomic_flag_test_and_set(&(array[i].claim))){
			atomic_store_explicit(
					&(array[i].clock),
					get_lamport_time(array, max_threads),
					memory_order_release);
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

void mr_quiescent_state(
		struct mr_entry *array,
		int thread_id,
		int max_threads)
{
	atomic_store_explicit(
			&(array[thread_id].clock),
			get_lamport_time(array, max_threads),
			memory_order_release);
}

void mr_reclaim_node(
		struct mr_entry *array,
		int thread_id,
		int max_threads,
		void *ffp_node)
{
	struct entry *mr_node = malloc(sizeof(struct entry));
	mr_node->rm_time = get_lamport_time(array, max_threads);
	mr_node->node = ffp_node;
	free_nodes(
			array[thread_id].head,
			get_min_time(array, max_threads));
	STAILQ_INSERT_TAIL(array[thread_id].head, mr_node, entries);
	atomic_store_explicit(
			&(array[thread_id].clock),
			mr_node->rm_time,
			memory_order_release);
}
