#ifndef _QUEUE_H_
#define _QUEUE_H_

#define LOCAL_QUEUE
//#define POSIX_MQ_QUEUE

#if defined(LOCAL_QUEUE)

#include "stdlib.h"
#include "stdio.h"
#include "pthread.h"
#include "assert.h"
#include "string.h"

#define PRI_MAX 10
#define BUF_POOL_SIZE 65536
#define LOG
#define LOG2
#define LOG3

#define ASSERT(x) assert(x)
#define uint32_t int
#define LOCK(x) pthread_mutex_lock(&x)
#define UNLOCK(x) pthread_mutex_unlock(&x)

typedef enum bool_ {
	false,
	true
} bool;

typedef struct queue_stats_ {
	int enqueue;
	int dequeue;
	int wait_time;
} queue_stats;

int priority[PRI_MAX];

/*
 * List of nodes in a hash bucket
 */
typedef struct node_ {
	void* key;
	int priority;
	struct node_* next;
} node;

/*
 * Define the hash table
 * |p1| ->|a|->|b|->|c|
 * |p2|->|e|->|f|
 */
typedef struct ptable_entry_ {
	int priority;
	node* n;
} ptable_entry;

typedef struct ptable_ {
	ptable_entry entry[PRI_MAX];
	node* last[PRI_MAX];
	node* buf_pool;
	node* free_bbuf_pool;
	int ent_count;
	pthread_mutex_t lock;
	pthread_cond_t cv;
	bool is_available;
	queue_stats *stats;
} ptable;

void create(ptable*);

#elif defined(POSIX_MQ_QUEUE)
#include <mqueue.h>
typedef struct ptable_ {
	mqd_t mqdesc;
} ptable;
void create(ptable*, char *mq_name);
#else
#error "You must define LOCAL_QUEUE or POSIX_MQ_QUEUE to choice a message queue\n"
#endif

void put_data(ptable*, void* key, int priority);
void* get_data(ptable*, int* prio);
void cleanup(ptable *p);

#endif
