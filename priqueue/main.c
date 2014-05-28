#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "queue.h"
#include "pthread.h"
/*
 * Test code
 *   Use two threads, acting as producer and consumer
 */

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void* producer(void* p)
{
	assert(p);

	ptable *table = (ptable*)p;

	printf("Thread producer\n");
	int i = 0;
	usleep(100000);
	while(1) {
		/*
		 * We break the producer after enqueuing 16 messages
		 */
		if (i == 1024) {
			break;
		}
		printf("Calling put_data %d\n\t", i);
		/*
		 * Using max bucket as (MAX_PRI - 1)
		 */ 
		int *j = malloc(sizeof(int));
		assert(j);
		*j = i++;
		put_data(p, j, (i % 9));
		
		usleep(1000);
	}
}

void* consumer(void* p)
{
	//  sleep(2);
	ptable *table = (ptable*)p;

	int* key, priority;

	printf("Thread consumer\n");
	int i = 0;
	while(1) {
		int x;
		printf("Calling get_data\n");
		key = get_data(p, &priority);
		x = *key;
		free(key);
		printf("\nSearch-> Priority=%d key= %d\n", priority, x);
		/*
		 * We break the consumer after dequeuing 16 messages.
		 * The next call to get_data will block since there
		 * will be no data from the producer
		 */
		if (x >= 1023) {
			break;
		}
	}
}

int main()
{
	ptable *p = malloc(sizeof(ptable));
	assert(p);
	create(p);

//	ptable *p2 = malloc(sizeof(ptable));
//	create(p2, "/hello2");
//	cleanup(p2);

	pthread_t thread1, thread2;

	int iret1, iret2;

	iret1 = pthread_create( &thread1, NULL, producer, (void*) p);


	iret2 = pthread_create( &thread2, NULL, consumer, (void*) p);

	//  display(p);

	pthread_join( thread1, NULL);
	pthread_join( thread2, NULL);

	cleanup(p);
}
