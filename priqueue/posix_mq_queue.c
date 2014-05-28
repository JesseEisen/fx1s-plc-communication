#include "queue.h"

#ifdef POSIX_MQ_QUEUE

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_QMSG_SIZE       4

void create(ptable*p, char *mq_name)
{
	assert(p);
	assert(mq_name);

	int ret = mq_unlink(mq_name);

//	struct mq_attr attrs;
//	attrs.mq_flags = 0;
//	attrs.mq_maxmsg = 1024;
//	attrs.mq_msgsize = MAX_QMSG_SIZE;

//	p->mqdesc = mq_open(mq_name, O_RDWR | O_CREAT /*| O_EXCL*/, S_IRWXU | S_IRWXG, &attrs);
	p->mqdesc = mq_open(mq_name, O_RDWR | O_CREAT /*| O_EXCL*/, S_IRWXU | S_IRWXG, NULL);
	if (p->mqdesc < 0) {
		perror("mq_open error\n");
		free(p);
		exit(1);
	}
}

void put_data(ptable*p, void* key, int priority)
{
	assert(p);
	assert(key);
	
	char buf[4];
	uint32_t x = (uint32_t)key;	

	buf[0] = (uint8_t)x&0xFF;
	buf[1] = (uint8_t)(x >> 8)&0xFF;
	buf[2] = (uint8_t)(x >> 16)&0xFF;
	buf[3] = (uint8_t)(x >> 24)&0xFF;
	
//	usleep(1000);
	
	int ret = mq_send(p->mqdesc, buf, 4, /*priority*/10);
	printf("put_data:%p\n", (void *)key);
	if (ret < 0) {
		perror("mq_send error\n");
		if (p->mqdesc > 0)
			mq_close(p->mqdesc);
		free(p);
		exit(1);
	}

}

void* get_data(ptable*p, int* prio)
{
	assert(p);
	struct mq_attr attr;
	mq_getattr(p->mqdesc, &attr);


	char buf[attr.mq_msgsize];
//	int ret = mq_receive(p->mqdesc, buf, MAX_QMSG_SIZE, prio!=NULL?prio:NULL);
	int ret = mq_receive(p->mqdesc, buf, attr.mq_msgsize, prio!=NULL?prio:NULL);
	if (ret < 0) {
		perror("mq_receive error\n");
		if (p->mqdesc > 0)
			mq_close(p->mqdesc);
		free(p);
		exit(1);
	}

	uint32_t x = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
	printf("get_data:%p\n", (void *)x);
	return (void *)x;
}

void cleanup(ptable *p)
{
	assert(p);
		
	// 1. first read all left msg, free them
	char buf[4];

	while (1) {
		struct timespec time_out;
		memset(&time_out, 0, sizeof(struct timespec));
	
		time_out.tv_sec = 1;
		time_out.tv_nsec = 0;
	
		int ret = mq_timedreceive(p->mqdesc, buf, MAX_QMSG_SIZE, NULL, &time_out);
		if (ret < 0) {
			break;
		}

		int x = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | (buf[3]&0xFF);
		free((void *)x);
	}

	// 2. close
	if (p->mqdesc > 0) {
		mq_close(p->mqdesc);
	}

	// TODO: thinking
	free(p);
}

#endif
