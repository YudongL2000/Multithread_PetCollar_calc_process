#ifndef I4S_SHM_H
#define I4S_SHM_H
#ifdef __cplusplus
extern "C" {
#endif

#include<stdint.h>
enum {
	SHM_LIVE_DATA = 0,
	SHM_LIVE_DATA_REPLY = 1,
	SHM_HISTORY_DATA = 2,
	SHM_HISTORY_DATA_REPLY = 3,
	SHM_CMDDATA = 4,
	SHM_CMDDATA_REPLY = 5,
};
int16_t i4s_shm_init(int initflag);
// id like SHM_LIVE_DATA 
int16_t i4s_shm_read_api(int id,uint8_t*buf,uint16_t maxreadlen);
int16_t i4s_shm_write_api(int id,const uint8_t*buf,uint16_t len);
int16_t i4s_shm_iscanwrite_api(int id,uint16_t len);




#ifdef __cplusplus
}
#endif

#endif

