#include "common.h"
#include "serial.h"
#include "util.h"

static int _open_device(struct serialsosurce *ss, char *device)
{
	assert(ss);
	assert(device);
	assert(strlen(device) < 255);

	memset(ss, 0, sizeof(*ss));
	strcpy(ss->device, device);

	ss->fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (ss->fd == -1) {
		DEBUG("fd = %d, %s\n", ss->fd, strerror(errno));
		return -1;
	}

	ss->req = malloc(sizeof(ptable));
	create(ss->req);
	
	return 0;
}

static int _set_device(struct serialsosurce *ss, int baude, char bits, char parity, char stop)
{
	int fd = ss->fd;

	struct termios options;
	if (tcgetattr(fd, &options) != 0) {
		return -1;
	}

	bzero(&options, sizeof(struct termios));

	switch (baude) {
	case 9600:
		cfsetispeed(&options, B9600);
		cfsetospeed(&options, B9600);
		break;
	default:
		return -1;
	}

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);	/*Input */
	options.c_oflag &= ~OPOST;	/*Output */

	options.c_cflag |= CLOCAL;
	options.c_cflag |= CREAD;

	switch (parity) {
	case 'N':
	case 'n':
	case 'S':
	case 's':
		options.c_cflag &= ~PARENB;
		break;
	case 'E':
	case 'e':
		options.c_cflag |= PARENB;
		options.c_cflag &= ~PARODD;
		break;
	case 'O':
	case 'o':
		options.c_cflag |= PARENB;
		options.c_cflag |= PARODD;
		break;
	default:
		return -1;
	}

	if (options.c_cflag & PARENB) {
		// Enable Parity
		options.c_iflag |= (INPCK | ISTRIP);
	}

	switch (stop) {
	case '1':
		options.c_cflag &= ~CSTOPB;
		break;
	default:
		return -1;
	}


	switch (bits) {
	case '7':
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS7;
		break;
	case '8':
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS8;
		break;
	default:
		return -1;
	}

	options.c_cc[VTIME] = 0;	/* 设置超时15 seconds */
	options.c_cc[VMIN] = 1;	/* define the minimum bytes data to be readed */
	tcflush(fd, TCIFLUSH);

	// set Attr
	if (tcsetattr(fd, TCSANOW, &options) != 0) {
		return -1;
	}

	return 0;
}

static int _close_device(struct serialsosurce *ss)
{
	assert(ss);
	
	if (ss->fd > 0) close(ss->fd);
	if (ss->req) cleanup(ss->req);

	memset(ss, 0, sizeof(struct serialsosurce));
	
	return 0;
}


static int _check_command(char *buf, int sz)
{
	// TODO: implementation totally check

	int ret = 0;
	
	ret |= (buf[0] == 0x02);
	ret |= (buf[1] == 0x30 || buf[1] == 0x31);

	return ret;
}

void *thread_serialcomm(void *parm)
{
	struct serialsosurce *ss = (struct serialsosurce*)parm;
	
RESTART:
	while (1) {
		usleep(1000);
		struct serialcommand *sc = get_data(ss->req, NULL);
		int ret;

		// clean serial
//		struct timeval tv_clean;
//		memset(&tv_clean, 0, sizeof(struct timeval));
//		tv_clean.tv_sec  = 0;
//		tv_clean.tv_usec = 0;	
//
//		fd_set rfds_clean;
//		FD_ZERO(&rfds_clean);
//		FD_SET(ss->fd, &rfds_clean);
//		
//		int ret = select(ss->fd+1, &rfds_clean, NULL, NULL, &tv_clean);	
//		if (ret == -1) {
//			DEBUG("select error\n");
//			free(sc);
//			goto RESTART;
//		} else if (ret == 0) {
//			DEBUG("never happen\n");
//			free(sc);
//			goto RESTART;
//		} else {
//			char tmp[4096];
//			read(ss->fd, tmp, 4096);
//			DEBUG("clean plc serial buffer\n");
//		}
			
		if (_check_command(sc->buf, sc->sz) == 0) {
			DEBUG("cmd error\n");
			free(sc);
			goto RESTART;
		}

		int num = 0;
		if (sc->buf[1] == 0x30) {
			// DATA size + STX(1 byte) + ETX(1 byte) + SUM(2 byte)
			num = ((sc->buf[6]-'0')*10 + (sc->buf[7]-'0'))*2+4;
		} else {
			num = 1;
		}

		if (num > 64*2+4) {
			free(sc);
			goto RESTART;
		}


		// write command
		ret = safe_write(ss->fd, sc->buf, sc->sz);
		if (ret < 0) {
			free(sc);
			goto RESTART;
		}

		ss->stats.n_send++;
		
		char resp[4096];
		memset(resp, 0, sizeof(resp));
		int sz = 0;
		char *p_resp = resp;

		for (;;) {
			struct timeval tv;
			memset(&tv, 0, sizeof(struct timeval));
			tv.tv_sec = 5;
			tv.tv_usec = 0;	

			fd_set rfds;
			FD_ZERO(&rfds);
			FD_SET(ss->fd, &rfds);
	
			ret = select(ss->fd+1, &rfds, NULL, NULL, &tv);
			if (ret == -1) {
				DEBUG("select error\n");
				free(sc);
				goto RESTART;
			} else if (ret == 0) {
				DEBUG("time expired\n");
				free(sc);
				goto RESTART;
			} else {
				int cnt = read(ss->fd, p_resp, num);
				if (cnt == 0) {
					DEBUG("serial error\n");
					free(sc);
					goto RESTART;
				}


				num -= cnt;
				p_resp += cnt;
				sz += cnt;
				if (num == 0) {
					// call cb
					ss->stats.n_recv++;
					sc->cb(sc->cxt, resp, sz);
					break;
				}
			}
		}
	
		// free something
		free(sc);
	}

	return (void *)NULL;
}

int serial_start(struct serialsosurce *ss)
{
	int ret;
	ret = _open_device(ss, "/dev/ttyUSB0");
	assert(ret == 0);
	
	ret = _set_device(ss, 9600, '7', 'E', '1');
	assert(ret == 0);
	
	pthread_t tid_serial;
	ret = pthread_create(&tid_serial, NULL, thread_serialcomm, (void *)ss);
	assert(ret == 0);

	ss->tid_serial = tid_serial;
	
	return 0;
}

int serial_stop(struct serialsosurce *ss)
{
	pthread_cancel(ss->tid_serial);
	pthread_join(ss->tid_serial, NULL);
	_close_device(ss);

	return 0;
}

int serial_command(struct serialsosurce *ss, struct serialcommand *sc)
{
	assert(ss);
	assert(sc);

	struct serialcommand *local_sc = malloc(sizeof(struct serialcommand));
	assert(sc);	
	
	local_sc->cxt = sc->cxt;
	local_sc->cb = sc->cb;
	
	local_sc->sz = sc->sz;
	memcpy(local_sc->buf, sc->buf, sizeof(local_sc->buf));

	// TODO: let write command has high priority
	put_data(ss->req, (void *)local_sc, 1/*Priority*/);
	return 0;
}


static char _getAscii(int i) 
{
	if (i >=0 && i <= 9) return i+'0';
	else if (i>=10 && i <=15) return (i-10)+'A';
}

char _getAddressAscii(int address, char buf[4])
{
	int x = address * 2 + 0x1000;

	int i, j, m, n;  
	i = x / (16 * 16 * 16); 
	j = (x / (16 * 16)) % 16; 
	m = x - i*(16*16*16) - j*(16*16); 
	m = m / 16; 
	n = x % 16; 

	buf[0] = _getAscii(i);
	buf[1] = _getAscii(j);
	buf[2] = _getAscii(m);
	buf[3] = _getAscii(n);

	//printf("add2ascii:%d->%02x, %02x, %02x, %02x\n", address, buf[0], buf[1], buf[2], buf[3]);
}

int getReadCommandFrame (char *buf, int *sz, int address, int num)
{
	if (buf == NULL || sz == NULL ||  
			(address < 0 || address > 255) || num < 0)  
		return -1; 

	buf[0] = 0x02;
	buf[1] = '0';

	_getAddressAscii(address, &buf[2]);

	num = num *2;

	buf[6] = _getAscii(num/10);
	buf[7] = _getAscii(num%10);

	buf[8] = 0x03;


	int i, sum = 0;
	for (i = 1; i <= 8; i++)
		sum += buf[i];

	i = sum&0xFF;
	buf[9]  = _getAscii(i/16);
	buf[10] = _getAscii(i%16);

	*sz = 11;

	for (i = 0; i < *sz; i++) {
//		printf("%02x ", buf[i]);
	}

//	printf("\n");

	return 0;
}

int getWriteCommandFrame(char *buf, int *sz, int address, int num, char *data)
{
	if (buf == NULL || sz == NULL ||  
			(address < 0 || address > 255) || num < 0)  
		return -1; 

	buf[0] = 0x02;
	buf[1] = '1';

	_getAddressAscii(address, &buf[2]);

	num = num*2;

	buf[6] = _getAscii(num/10);
	buf[7] = _getAscii(num%10);

	int i;
	for (i = 0; i < num*2; i+=4) {
		buf[8+i]   = data[i+2];
		buf[8+i+1] = data[i+3];
		buf[8+i+2] = data[i];
		buf[8+i+3] = data[i+1];
	}

	buf[8+num*2] = 0x03;

	int sum = 0;
	for (i = 1; i <= 8+num*2; i++)
		sum += buf[i];

	i = sum&0xFF;
	buf[8+num*2+1]  = _getAscii(i/16);
	buf[8+num*2+2]  = _getAscii(i%16);

	*sz = 8+num*2+3;

	for (i = 0; i < *sz; i++) {
//		printf("%02x ", buf[i]);
	}

//	printf("\n");

	return 0;
}
