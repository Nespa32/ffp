struct mr_entry *init_mr(int max_threads);

int mr_thread_acquire(
		struct mr_entry *array,
		int max_threads);

void mr_thread_release(
		struct mr_entry *array,
		int thread_id);

void mr_quiescent_state(
		struct mr_entry *array,
		int thread_id,
		int max_threads);

void mr_reclaim_node(
		struct mr_entry *array,
		int thread_id,
		int max_threads,
		void *ffp_node);
