#include <stdio.h>
#include "plc.h"

int main(int argc, char *argv[]) 
{
	struct serialsosurce ss;

	serial_start(&ss);

#define OPEN  1
#define CLOSE 0
	int status = 0;
//	while (1) {
//		controller_set(&ss, 1, status == 0?1:0);
//		controller_get(&ss, 1, &status);
//		printf("controller status:%s\n", status == 0?"Close":"Open");
//		usleep(100000);
//	}

	int count = 0;
	int err = 0;
	while (count < 100) {
		count++;
//		sensor_set(&ss, count, count);
		int x;
		sensor_get(&ss, count, &x);
		printf("sensor %d D value:%d\n", count, x);
		if (x != count) err++;
	}

	printf("err = %d\n", err);
	while (1) getchar();

//	int id[] =   {1, 2, 3, 4, 5, 6, 7, 10, 123, 56, 111};
//	int data[] = {1, 2, 4, 4, 5234, 6, 789, 10, 123, 56, 111};

//	int id[255];
//	int data[255];
//
//	int i;
//	for (i = 1; i < 255; i++) {
//		id[i-1]   = i;
//		data[i-1] = i;
//	}
//
//	struct sensorX sx;
//	sensorX_init(&sx, sizeof(id)/sizeof(int), id, data);
//	sensorX_set(&ss, &sx);
//
//	struct sensorX sx_test;
//	sensorX_init(&sx_test, sizeof(id)/sizeof(int), id, NULL);
//	sensorX_get(&ss, &sx_test);
//	for (i = 0; i < sx_test.n; i++) {
//		printf("sensor %d D value:%d\n", sx_test.id[i], sx_test.data[i]);
//	}

	return 0;
}
