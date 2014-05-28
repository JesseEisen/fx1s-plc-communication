#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DEBUG_PRINT

#include <pthread.h>
extern pthread_mutex_t LOCK;

#if defined(DEBUG_PRINT)
#define DEBUG(...) \
	do { \
		pthread_mutex_lock(&LOCK);\
		fprintf(stderr, "[DEBUG]:%s:%d:%s(): ", __FILE__, __LINE__, __FUNCTION__); \
		fprintf(stderr, __VA_ARGS__); \
		pthread_mutex_unlock(&LOCK);\
	} while(0)
#else
void no_op();
#define DEBUG(...) no_op()
#endif /* NO_DEBUG */

#endif
