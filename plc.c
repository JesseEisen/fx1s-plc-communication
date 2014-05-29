#include "common.h"
#include "plc.h"
#include <sys/select.h>

static int _cb_async(void *cxt, char *buf, int sz)
{	
	int fd = (int)cxt;
	write(fd, buf, sz);
	return 0;
}

int controller_set(struct serialsosurce *ss, int id, int status)
{
	struct serialcommand sc;
	
	switch(status) {
	case 0:
		getWriteCommandFrame(sc.buf, &sc.sz, id, 1, "0000");
		break;
	case 1:
		getWriteCommandFrame(sc.buf, &sc.sz, id, 1, "0001");
		break;
	default:
		fprintf(stderr, "device status number error\n");
		return -1;
	}	

	int fd[2];
	pipe(fd);

	sc.cxt = (void *)fd[1];	
	sc.cb = _cb_async;
	serial_command(ss, &sc);
	
	char buf[255];
	int sz;
	int ret; /*select return value*/
	struct timeval tv;
	fd_set readset;

	FD_ZERO(&readset);
	FD_SET(fd[0],&readset);
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	
	if((ret = select((fd[0]+1),&readset,NULL,NULL,&tv)) < 0){
		printf("select error\n");
		return -1;
	}else if(ret == 0){
		printf("filedes not ready\n");
		return -1;
	} 

	sz = read(fd[0], buf, 255);
	int i;
	for (i = 0; i < sz; i++) {
		printf("%02x ", buf[i]);
	}

	printf("\n");
	close(fd[0]);
	close(fd[1]);

	if (buf[0] == 0x06) {
		return 0;
	} else {
		return -1;
	}	
} 

int controller_get(struct serialsosurce *ss, int id, int *status)
{
	struct serialcommand sc;
	getReadCommandFrame(sc.buf, &sc.sz, id, 1);

	int fd[2];
	pipe(fd);

	sc.cxt = (void *)fd[1];	
	sc.cb = _cb_async;
	serial_command(ss, &sc);
	
	char buf[255];
	int  sz;
	int  ret; /*select return*/
	struct timeval tv;
	fd_set readset;
	
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	FD_ZERO(&readset);
	FD_SET(fd[0],&readset);/*let fd[0] seted in readset*/
	if((ret = select(( fd[0]+1),&readset,NULL,NULL,&tv))< 0){
		printf("select error\n");
		return -1;
	}else if(ret == 0){
		printf("filedes are not ready for read within 5s\n");
		return -1;
	}
	sz = read(fd[0], buf, 255);
	
	int i;
	for (i = 0; i <  sz; i++) {
		printf("%02x ", buf[i]);
	}

	printf("\n");
	close(fd[0]);
	close(fd[1]);

//	printf("buf[2] = %x\n", buf[2]);
	if (buf[2] == 0x31) {
		*status = 1;
	} else if (buf[2] == 0x30) {
		*status = 0;
	} else {
		return -1;
	}

	return 0;
}

static void buf4_to_integer(char *buf, int *x)
{
	int x1, x2, x3, x4;
	x1 = (buf[2] - '0');
	x2 = (buf[3] - '0');
	x3 = (buf[0] - '0');
	x4 = (buf[1] - '0');

	*x = x1*1000+x2*100+x3*10+x4;
}

static void integer_to_buf4(int x, char *buf)
{
	int x1, x2, x3, x4;
	x4 = x%10;
	x3 = (x%100)/10;
	x2 = (x/100)%10;
	x1 = x/1000;

	buf[0] = x1 + '0';
	buf[1] = x2 + '0';
	buf[2] = x3 + '0';
	buf[3] = x4 + '0';
}

int sensor_set(struct serialsosurce *ss, int id, int data)
{
	struct serialcommand sc;
	char buf[4];
	integer_to_buf4(data, buf);
	getWriteCommandFrame(sc.buf, &sc.sz, id, 1, buf);

	int fd[2];
	pipe(fd);

	sc.cxt = (void *)fd[1];	
	sc.cb = _cb_async;
	serial_command(ss, &sc);
	
	char buf2[255];
	int sz;
	int ret; /*select return value*/
	struct timeval tv;
	fd_set readset;

	FD_ZERO(&readset);
	FD_SET(fd[0],&readset);
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	
	if((ret = select((fd[0]+1),&readset,NULL,NULL,&tv)) < 0){
		printf("select error\n");
		return -1;
	}else if(ret == 0){
		printf("filedes not ready\n");
		return -1;
	} 

	sz = read(fd[0], buf2, 255);
	int i;
//	for (i = 0; i < sz; i++) {
//		printf("%02x ", buf2[i]);
//	}

//	printf("\n");
	close(fd[0]);
	close(fd[1]);

	return 0;
}

int sensor_get(struct serialsosurce *ss, int id, int *data)
{
	struct serialcommand sc;
	getReadCommandFrame(sc.buf, &sc.sz, id, 1);

	int fd[2];
	pipe(fd);

	sc.cxt = (void *)fd[1];	
	sc.cb = _cb_async;
	serial_command(ss, &sc);
	
	char buf[255];
	int sz;
	int ret; /*select return value*/
	struct timeval tv;
	fd_set readset;

	FD_ZERO(&readset);
	FD_SET(fd[0],&readset);
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	
	if((ret = select((fd[0]+1),&readset,NULL,NULL,&tv)) < 0){
		printf("select error\n");
		return -1;
	}else if(ret == 0){
		printf("filedes not ready\n");
		return -1;
	} 

	sz = read(fd[0], buf, 255);
//	int i;
//	for (i = 0; i < sz; i++) {
//		printf("%02x ", buf[i]);
//	}
//
//	printf("\n");
	close(fd[0]);
	close(fd[1]);

	int x;
	buf4_to_integer(&buf[1], &x);
	*data = x;

	return 0;	
}

int sensorX_init(struct sensorX *sx, int n_id, int *id, int *data)
{
	sx->n = n_id;
	int i;
	for (i = 0; i < n_id; i++) {
		sx->id[i] = id[i];
		if (data != NULL) {
			sx->data[i] = data[i];
		}
	}
	return 0;
}

int sensorX_get(struct serialsosurce *ss, struct sensorX *sx)
{ 
	int i;

	for (i = 0; i < sx->n; i++) {
		if(sensor_get(ss, sx->id[i], &sx->data[i]) == -1)
			return -1;			
	}
	return 0;
}

int sensorX_set(struct serialsosurce *ss, struct sensorX *sx)
{
	int i;
	for (i = 0; i < sx->n; i++) {
		if(sensor_set(ss, sx->id[i], sx->data[i]) == -1)
			return -1;
	}
	return 0;
}
