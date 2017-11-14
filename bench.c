#include <ffp.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

unsigned long long limit_sf,
                   limit_r,
                   limit_i,
                   test_size;

int n_threads;

struct ffp_node *head;

void *prepare_worker(void *entry_point)
{
	for(int i=0; i<test_size/n_threads; i++){
		unsigned long long value = nrand48(entry_point);
		if(value < limit_r)
			search_insert_hash(head, value, (void*)value);
	}
	return NULL;
}

void *bench_worker(void *entry_point)
{
	for(int i=0; i<test_size/n_threads; i++){
		unsigned long long value = nrand48(entry_point);
		if(value < limit_sf){
			if((unsigned long long)search_hash(head, value)!=value)
				printf("failiure: item not match %lld\n", value);
		}
		else if(value < limit_r){
			search_remove_hash(head, value);
		}
		else if(value < limit_i){
			search_insert_hash(head, value, (void*)value);
		}
		else{
			if(search_hash(head, value)!=NULL)
				printf("failiure: found item %lld\n", value);
		}
	}
	return NULL;
}

void *test_worker(void *entry_point)
{
	for(int i=0; i<test_size/n_threads; i++){
		unsigned long long value = nrand48(entry_point);
		if(value < limit_sf){
			if((unsigned long long)search_hash(head, value)!=value)
				return (void*)1;
		}
		else if(value < limit_r){
			if(search_hash(head, value)!=NULL)
				return (void*)1;
		}
		else if(value < limit_i){
			if((unsigned long long)search_hash(head, value)!=value)
				return (void*)1;
		}
		else{
			if(search_hash(head, value)!=NULL)
				return (void*)1;
		}
	}
	return NULL;
}

int main(int argc, char **argv)
{
	if(argc < 7){
		printf("usage: bench <treads> <nodes> <inserts> <removes> <searches found> <searches not found>\nAdd 't' at the end to verify integrity\n");
		return -1;
	}
	printf("preparing data.\n");
	n_threads = atoi(argv[1]);
	test_size = atoi(argv[2]);
	unsigned long long inserts = atoi(argv[3]),
	                   removes = atoi(argv[4]),
	                   searches_found = atoi(argv[5]),
	                   searches_not_found = atoi(argv[6]),
	                   total = inserts + removes + searches_found + searches_not_found;
	limit_sf = RAND_MAX*searches_found/total;
	limit_r = limit_sf + RAND_MAX*removes/total;
	limit_i = limit_r + RAND_MAX*inserts/total;
	struct timespec start_monoraw,
			end_monoraw,
			start_process,
			end_process;
	double time;
	pthread_t *threads = malloc(n_threads*sizeof(pthread_t));
	unsigned short **seed = malloc(n_threads*sizeof(unsigned short*));
	head = init_ffp();
	for(int i=0; i<n_threads; i++)
		seed[i] = malloc(3*sizeof(unsigned short));
	if(limit_r!=0){
		for(int i=0; i<n_threads; i++){
			seed[i][0] = i;
			seed[i][1] = i;
			seed[i][2] = i;
			pthread_create(&threads[i], NULL, prepare_worker, seed[i]);
		}
		for(int i=0;i<n_threads; i++){
			pthread_join(threads[i], NULL);
		}
	}
	printf("starting test\n");
	clock_gettime(CLOCK_MONOTONIC_RAW, &start_monoraw);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_process);
	for(int i=0; i<n_threads; i++){
		seed[i][0] = i;
		seed[i][1] = i;
		seed[i][2] = i;
		pthread_create(&threads[i], NULL, bench_worker, seed[i]);
	}
	for(int i=0; i<n_threads; i++)
		pthread_join(threads[i], NULL);
	clock_gettime(CLOCK_MONOTONIC_RAW, &end_monoraw);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_process);
	time = end_monoraw.tv_sec - start_monoraw.tv_sec + ((end_monoraw.tv_nsec - start_monoraw.tv_nsec)/1000000000.0);
	printf("Real time: %lf\n", time);
	time = end_process.tv_sec - start_process.tv_sec + ((end_process.tv_nsec - start_process.tv_nsec)/1000000000.0);
	printf("Process time: %lf\n", time);
	if(argc == 8 && argv[7][0]=='t'){
		for(int i=0; i<n_threads; i++){
			seed[i][0] = i;
			seed[i][1] = i;
			seed[i][2] = i;
			pthread_create(&threads[i], NULL, test_worker, seed[i]);
		}
		void *retval;
		for(int i=0; i<n_threads; i++){
			pthread_join(threads[i], &retval);
			if(retval != NULL){
				printf("Failed!\n");
				return -2;
			}
		}
		printf("Correct!\n");
	}
	return 0;
}
