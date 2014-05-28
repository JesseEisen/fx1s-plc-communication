#include "common.h"
#include <termios.h>
#include <unistd.h>
#include "util.h"
#include <sys/ioctl.h>

int open_socket_client(char *ip, int port)
{
	int fd;
	struct sockaddr_in serv_addr; 
	if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		DEBUG("Error : Could not create socket \n");
		return -1;
	}
	memset(&serv_addr, '0', sizeof(serv_addr)); 

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port); 

	if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0) {
		DEBUG("inet_pton error occured\n");
		close(fd);
		return -1;
	} 

	DEBUG("...\n");
	// set nonblocking
	// select fd 1
	// connect
	if(connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		//		DEBUG("error : connect failed, %d\n", fd);
		close(fd);
		return -1;
	}
	// set blocking
	DEBUG("...\n");

	return fd;
}

int open_socket_server(int port)
{
	int listenfd = 0;
	struct sockaddr_in serv_addr; 

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	int reuse = 1;

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port); 

	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

	listen(listenfd, 10); 

	return listenfd;
}

int safe_read(int fd, void *buffer, int n)
{
	fd_set fds;
	int cnt;

	if (fd < 0) {
		return -1;
	}

	for (;;) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		cnt = select(fd+1, &fds, NULL, NULL, NULL);
		if (cnt < 0) {
			return -1;
		}

		//		int size;
		//		int ret = ioctl(fd, FIONREAD, &size);
		//		if (ret < 0) return -1;
		//		if (size == 0) continue;

		cnt = read(fd, buffer, n);
		if (cnt != 0) {
			return cnt;
		}

		if (cnt == 0) return -1;
	}
}

int safe_write(int fd, void *buffer, int count)
{
	int actual = 0;

	if (fd < 0) return -1;

	while (count > 0) {
		int n = write(fd, buffer, count);
		if (n < 0 && errno == EINTR) {
			continue;
		}

		if (n <= 0) {
			actual = -1;
			break;
		}

		count  -= n;
		actual += n;
		buffer += n;
	}

	return actual;
}

//int safe_read(int fd, void *buffer, int n)
//{
//	int nread;
//	int actual = 0;
//	int sz = 0;
//	char *p = buffer;
//	
//	for (;;) {
//		nread = read(fd, p, n);
//		if (nread < 0) {
//			switch(errno) {
//				case EINTR:
//					continue;
//				case EAGAIN:
//					return -1;
//			}
//			return -1;
//		} else if (nread == 0) {
//			return -1;
//		} else {
//			n -= nread;
//			p += nread; 
//			actual += nread;
//			if (n <= 0) break;
//		}
//	}
//
//	return actual;
//}
//
//int safe_write(int fd, void *buffer, int count)
//{
//	int nwrite;
//	int sz = 0;
//	char *p = buffer;
//	int actual = 0;
//	for (;;) {
//		nwrite = write(fd, p, count);
//		if (nwrite < 0) {
//			switch(errno) {
//				case EINTR:
//					continue;
//				case EAGAIN:
//					return -1;
//			}
//			return -1;
//		} else if (nwrite == 0) {
//			return -1;
//		} else {
//			count -= nwrite;
//			p += nwrite; 
//			actual += nwrite;
//			if (count <= 0) break;
//		}
//	}
//
//	return actual;
//}

int read_byte(int fd, struct recv *recv, char *ch)
{
	if (recv->bufpos >= recv->bufused) {
		for (;;) {
			int n = safe_read(fd, recv->buffer, 
					sizeof(recv->buffer));
			if (n < 0) {
				switch (errno) {
					case EAGAIN:
						return -1;
					case EINTR:
						continue;
				}
				return -1;
			} else if (n > 0) {
				recv->bufpos = 0;
				recv->bufused = n;
				break;
			}
		}
	}

	*ch = recv->buffer[recv->bufpos++];
	return 0;
}

int read_stream(int fd, struct recv *recv, char *dest, int sz)
{
	int preread, left;
	int i;

	if (sz <= (recv->bufused - recv->bufpos)) {
		memcpy(dest, &recv->buffer[recv->bufpos], sz);
		recv->bufpos += sz;
	} else {
		preread = recv->bufused - recv->bufpos;
		memcpy(dest, &recv->buffer[recv->bufpos], preread);
		recv->bufpos = recv->bufused;

		left = sz - preread;
		for (i = 0; i < left; i++) {
			char ch;
			int ret = read_byte(fd, recv, &ch);
			if (ret < 0) return -1;
			(dest+preread)[i] = ch;
		}
	}	
	return 0;
}

int get_rand(int start, int end)
{

	struct timeval tpstart;
	gettimeofday(&tpstart, NULL);
	srand(tpstart.tv_usec);
	//	srand((unsigned)(time(NULL)));

	int offsize = end - start;
	int n = (int)(offsize*1.0*rand()/(RAND_MAX+1.0));	

	n += start;

	return n;
}

void int24_encode(uint32_t x, unsigned char buf[3])
{
	buf[0] = (uint8_t)(x >> 16 & 0xff);
	buf[1] = (uint8_t)(x >> 8  & 0xff);
	buf[2] = (uint8_t)(x & 0xff);
}

void int24_decode(unsigned char buf[3], uint32_t *x)
{
	*x = (uint32_t)(buf[0] << 16 | buf[1] << 8 | buf[2]);
}

ssize_t tread(int fd, void *buf, size_t nbytes, unsigned int timeout)
{
	int nfds;
	fd_set readfds;
	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	nfds = select(fd+1, &readfds, NULL, NULL, &tv);
	if (nfds <= 0) {
		if (nfds == 0) 
			errno = ETIME;
		return -1;
	}

	return (read(fd, buf, nbytes));
}

ssize_t treadn(int fd, void *buf, size_t nbytes, unsigned int timeout)
{
	size_t nleft;
	ssize_t nread;

	nleft = nbytes;
	while (nleft > 0) {
		if ((nread == tread(fd, buf, nleft, timeout)) < 0) {
			if (nleft == nbytes) {
				return -1;
			} else {
				break;
			}
		} else if (nread == 0) {
			break;
		}
		nleft -= nread;
		buf += nread;
	}

	return (nbytes - nleft); // return >= 0
}

void read_file (const char *filename , struct slice *slice) {
	FILE *f = fopen(filename, "rb");
	if (f == NULL) {
		fprintf(stderr, "Can't open file %s\n", filename);
		exit(1);
	}
	fseek(f,0,SEEK_END);
	slice->len = ftell(f);
	fseek(f,0,SEEK_SET);
	slice->buffer = malloc(slice->len);
	fread(slice->buffer, 1 , slice->len , f);
	fclose(f);
}
