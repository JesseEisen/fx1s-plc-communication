#ifndef _PLC_H_
#define _PLC_H_

#include "serial.h"

int controller_set(struct serialsosurce *ss, int id, int status);
int controller_get(struct serialsosurce *ss, int id, int *status);
int sensor_set(struct serialsosurce *ss, int id, int data);
int sensor_get(struct serialsosurce *ss, int id, int *data);

struct sensorX {
	int id[255];
	int data[255];
	int n;
};


int sensorX_init(struct sensorX *sx, int n_id, int *id, int *data);
int sensorX_get(struct serialsosurce *ss, struct sensorX *sx);
int sensorX_set(struct serialsosurce *ss, struct sensorX *sx);

#endif
