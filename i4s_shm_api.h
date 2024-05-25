#ifndef I4S_SHM_API_H
#define I4S_SHM_API_H
#ifdef __cplusplus
extern "C" {
#endif

#include<stdint.h>
enum {
	SHM_CMD_SHUTDOWN = 0,
	SHM_CMD_UPDATE_MODULE = 1,
	REQUEST_MODEL_LINK = 100,
};

typedef struct _i4s_cmd{
	char cmdid;
	int index;
	int datalen;
	char data[];
}__attribute__((__packed__))i4s_cmd;


typedef struct _i4s_cmd_reply{
	unsigned char cmdid;
	int index;
	char buf[512];
	int ret;
}__attribute__((__packed__))i4s_cmd_reply;


typedef struct _i4s_imu_data{
	char day;
	unsigned int count;
	short data[6];
}__attribute__((__packed__))i4s_imu_data;

typedef struct _i4s_imu_data_reply{
	char day;
	int count;
	int latency;
	int ret;
}__attribute__((__packed__))i4s_imu_data_reply;


void i4s_shm_api(void);




#ifdef __cplusplus
}
#endif

#endif

