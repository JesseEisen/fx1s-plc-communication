#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdint.h>

int open_socket_client(char *ip, int port);
int open_socket_server(int port);

#define MTU 4096

struct recv {
	char buffer[MTU];
	int bufpos, bufused;
};


ssize_t tread(int fd, void *buf, size_t nbytes, unsigned int timeout);
ssize_t treadn(int fd, void *buf, size_t nbytes, unsigned int timeout);

int  safe_read (int fd, void *buffer, int n);
int  safe_write(int fd, void *buffer, int count);
int  read_byte (int fd, struct recv *recv, char *ch);
int  read_stream(int fd, struct recv *recv, char *dest, int sz);

int get_rand(int start, int end);

void int24_encode(uint32_t x, unsigned char buf[3]);
void int24_decode(unsigned char buf[3], uint32_t *x);

struct slice {
	void *buffer;
	int len;
};

// 将一个文件加载到一块buffer中， 这个函数用来加载sql文件， 然后让sqlite3加载
void read_file (const char *filename , struct slice *slice);
#endif
