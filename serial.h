#ifndef _SERIAL_H_
#define _SERIAL_H_
#include "priqueue/queue.h"

typedef int (*serial_cb)(void *cxt, char *msg, int sz);

struct serialcommand {
	void *cxt;
	serial_cb cb;
	int sz;
	char buf[4096];
};

struct serialsosurce {
	char device[255];
	struct {
		int baude;
		char bits;
		char parity;
		char stop;
	} config;
	
	int fd;

	struct {
		int n_send;
		int n_recv;
		int n_err;
	} stats;

	ptable *req; // queue
	pthread_t tid_serial;
};

int serial_start(struct serialsosurce *ss);
int serial_stop(struct serialsosurce *ss);

// opt
int serial_command(struct serialsosurce *ss, struct serialcommand *sc);

// Util
int getReadCommandFrame (char *buf, int *sz, int address, int num);
int getWriteCommandFrame(char *buf, int *sz, int address, int num, char *data);

#endif
