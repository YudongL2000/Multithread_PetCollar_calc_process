#include<stdio.h>
#include<stdint.h>
#include<string.h>
#include <sys/mman.h>
#include <sys/stat.h>		 /* For mode constants */
#include <fcntl.h>			 /* For O_* constants */
#include<stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "i4s_shm.h"
#include <pthread.h>
static uint8_t *shm_buf = NULL;

static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

#define SHM_NAME "/my_shm1"
#define SHMLOG printf
struct i4s_shm_head{
	uint16_t wkeyp;
	uint16_t rkeyp;
	uint8_t data[];
}__attribute__((__packed__));
struct i4s_config{
	uint16_t begin;
	uint16_t maxlen;
};

#define SHM_LIVE_DATA_LEN 1024
#define SHM_LIVE_DATA_REPLY_LEN 1024
#define SHM_HISTORY_DATA_LEN 2048
#define SHM_HISTORY_DATA_REPLY_LEN 1024
#define SHM_CMDDATA_LEN 512
#define SHM_CMDDATA_REPLY_LEN 512

#define SHM_LIVE_DATA_LEN_BEGIN 10
#define SHM_LIVE_DATA_REPLY_LEN_BEGIN (SHM_LIVE_DATA_LEN_BEGIN + SHM_LIVE_DATA_LEN)
#define SHM_HISTORY_DATA_LEN_BEGIN (SHM_LIVE_DATA_REPLY_LEN_BEGIN + SHM_LIVE_DATA_REPLY_LEN)
#define SHM_HISTORY_DATA_REPLY_LEN_BEGIN (SHM_HISTORY_DATA_LEN_BEGIN + SHM_HISTORY_DATA_LEN)
#define SHM_CMDDATA_LEN_BEGIN (SHM_HISTORY_DATA_REPLY_LEN_BEGIN + SHM_HISTORY_DATA_REPLY_LEN)
#define SHM_CMDDATA_REPLY_LEN_BEGIN (SHM_CMDDATA_LEN_BEGIN + SHM_CMDDATA_LEN)
#define MAXSHM  (SHM_CMDDATA_REPLY_LEN_BEGIN + SHM_CMDDATA_REPLY_LEN)






static struct i4s_config shm_conf[] = {
    {.begin = SHM_LIVE_DATA_LEN_BEGIN, .maxlen = SHM_LIVE_DATA_LEN},
    {.begin = SHM_LIVE_DATA_REPLY_LEN_BEGIN, .maxlen = SHM_LIVE_DATA_REPLY_LEN},
    {.begin = SHM_HISTORY_DATA_LEN_BEGIN, .maxlen = SHM_HISTORY_DATA_LEN},
    {.begin = SHM_HISTORY_DATA_REPLY_LEN_BEGIN, .maxlen = SHM_HISTORY_DATA_REPLY_LEN},
    {.begin = SHM_CMDDATA_LEN_BEGIN, .maxlen = SHM_CMDDATA_LEN},
    {.begin = SHM_CMDDATA_REPLY_LEN_BEGIN, .maxlen = SHM_CMDDATA_REPLY_LEN},
};

int16_t i4s_shm_iscanwrite(uint8_t *mem,uint16_t maxlen,uint16_t len){
	struct i4s_shm_head *head;
	head = (struct i4s_shm_head *)mem;
	uint16_t wkeynum = head->wkeyp;
	uint16_t rkeynum = head->rkeyp;
	uint16_t freenum;
	uint16_t writelen;
	uint16_t maxbuflen;
	maxbuflen = maxlen - sizeof(struct i4s_shm_head);
	if(wkeynum == maxbuflen){
		wkeynum = 0;
	}
	if(wkeynum >= rkeynum){
		freenum = maxbuflen - wkeynum + rkeynum - 1;
	}else{
		freenum = rkeynum - wkeynum - 1;
	}
	//SHMLOG("wkeynum=%d,rkeynum=%d\n",wkeynum,rkeynum);
	//SHMLOG("freenum=%d\n",freenum);
	writelen = len +2;
	if(freenum < writelen){
		//SHMLOG("error1\n");
		return 0;
	}

	return 1;
}

int16_t i4s_shm_iscanwrite_api(int id,uint16_t len){
	return i4s_shm_iscanwrite(&shm_buf[shm_conf[id].begin],shm_conf[id].maxlen,len);
}



int16_t i4s_shm_write(uint8_t *mem,uint16_t maxlen,const uint8_t*buf,uint16_t len){
	struct i4s_shm_head *head;
	head = (struct i4s_shm_head *)mem;
	uint16_t wkeynum = head->wkeyp;
	uint16_t rkeynum = head->rkeyp;
	uint16_t freenum;
	uint16_t writelen;
	uint16_t maxbuflen;
	maxbuflen = maxlen - sizeof(struct i4s_shm_head);
	if(wkeynum == maxbuflen){
		wkeynum = 0;
	}
	if(wkeynum >= rkeynum){
		freenum = maxbuflen - wkeynum + rkeynum - 1;
	}else{
		freenum = rkeynum - wkeynum - 1;
	}
	//SHMLOG("wkeynum=%d,rkeynum=%d\n",wkeynum,rkeynum);
	//SHMLOG("freenum=%d\n",freenum);
	writelen = len +2;
	if(freenum < writelen){
		//SHMLOG("error1\n");
		return -1;
	}
	uint8_t *lenp = (uint8_t *)&len;
	uint16_t tmplen;
	if(wkeynum + writelen > maxbuflen){
		if(wkeynum == maxbuflen - 1){
			memcpy(&head->data[wkeynum],&lenp[0],1);
			//printf("wkeynum=%d\n",wkeynum);

			memcpy(&head->data[0],&lenp[1],1);
			//printf("head->data[wkeynum]=%d\n",head->data[wkeynum]);
			//printf("head->data[0]=%d\n",head->data[0]);

			memcpy(&head->data[1],buf,len);
			wkeynum = len + 1;
			
		}else {
			memcpy(&head->data[wkeynum],&lenp[0],2);
			tmplen = maxbuflen - wkeynum - 2;
			if(tmplen > 0){
				memcpy(&head->data[wkeynum+2],buf,tmplen);
			}

			memcpy(&head->data[0],&buf[tmplen],len - tmplen);
			wkeynum = len - tmplen;

		
		}
		
	}else{
		memcpy(&head->data[wkeynum],&len,2);
		memcpy(&head->data[wkeynum+2],buf,len);
		if(wkeynum+writelen == maxbuflen){
			wkeynum = 0;
		}else{
			wkeynum = wkeynum+writelen;
		}
		

		
	}
	head->wkeyp = wkeynum;
	//SHMLOG("end wkeynum=%d,rkeynum=%d\n",head->wkeyp,rkeynum);

	return 0;
}
uint16_t i4s_shm_is_havedata(uint8_t *mem){
	struct i4s_shm_head *head;
	head = (struct i4s_shm_head *)mem;
	uint16_t wkeynum = head->wkeyp;
	uint16_t rkeynum = head->rkeyp;
	if(wkeynum == rkeynum){
		return 0;
	}
	return 1;
}

uint16_t i4s_shm_read(uint8_t *mem,uint16_t maxlen,uint8_t*buf,uint16_t maxreadlen){
	struct i4s_shm_head *head;
	head = (struct i4s_shm_head *)mem;
	uint16_t wkeynum = head->wkeyp;
	uint16_t rkeynum = head->rkeyp;
	uint16_t datanum;
	uint16_t readlen;
	uint16_t maxbuflen;
	uint8_t *rewadlenp;
	uint16_t tmplen;
	maxbuflen = maxlen - sizeof(struct i4s_shm_head);

	if(wkeynum >= rkeynum){
		datanum = wkeynum - rkeynum;
	}else{
		datanum = maxbuflen - rkeynum + wkeynum;
	}


	if(datanum == 0){
		return 0;
	}


	if(datanum <= 2){
		SHMLOG("datanum error\n");
		//exit(-1);
		return 0;
	}
	if(rkeynum == maxbuflen -1){
		rewadlenp = (uint8_t*)&readlen;
		
		memcpy(&rewadlenp[0],&head->data[rkeynum],1);
		memcpy(&rewadlenp[1],&head->data[0],1);
		if(readlen > maxreadlen){
			SHMLOG("readlen=%d,maxreadlen=%d\n",readlen,maxreadlen);
			return 0;
		}
		if(readlen > datanum){
			SHMLOG("datanum error2\n");
			return 0;
		}else{
			memcpy(buf,&head->data[1],readlen);
			head->rkeyp = readlen+1;
			
		}


	}else{
		rewadlenp = (uint8_t*)&readlen;
		memcpy(&rewadlenp[0],&head->data[rkeynum],2);
		if(readlen > maxreadlen){
			SHMLOG("readlen=%d,maxreadlen=%d\n",readlen,maxreadlen);
			return 0;
		}
		if(readlen + 2 + rkeynum <= maxbuflen){
			memcpy(buf,&head->data[rkeynum + 2],readlen);
			if(rkeynum + 2 + readlen == maxbuflen){
				head->rkeyp = 0;
			}else{
				head->rkeyp = rkeynum + 2 + readlen;
			}
			
		}else{
			tmplen = maxbuflen -rkeynum - 2;
			if(tmplen > 0){
				memcpy(buf,&head->data[rkeynum + 2],tmplen);
			}
			memcpy(&buf[tmplen],&head->data[0],readlen-tmplen);
			head->rkeyp = readlen-tmplen;
		}	
	}
	return readlen;

}








uint8_t* createShm(int size,int initflag)
{
	int shmfd;
	uint8_t *mem;
	int ret;
	int i;
	shmfd = shm_open(SHM_NAME,O_CREAT | O_RDWR,0666);
	if(shmfd == -1){
		SHMLOG("shm_open error\n");
		return NULL;
	}

    ret = ftruncate(shmfd,size);
	if(ret == -1){
		SHMLOG("ftruncate error\n");
		return NULL;
	}
	mem = (uint8_t*)mmap(NULL,size,PROT_READ | PROT_WRITE,MAP_SHARED,shmfd,0);
	if(mem == MAP_FAILED){
		SHMLOG("ftruncate error\n");
		return NULL;
	}
	if(initflag == 0){
		while(1){
			for(i=0;i<10;i++){
				if(mem[i] != 0xaa+i){
					SHMLOG("errori=%d,mem[i]=%d\n",i,mem[i]);
					break;
				}
			}
			if(i == 10){
				break;
			}
			SHMLOG("to sleep\n");
			sleep(1);
		}
		return mem;
	}
#if 1
	for(i=0;i<10;i++){
		if(mem[i] != 0xaa+i){
			SHMLOG("check not ok\n");
			break;
		}
	}
#endif
	if(i != 10){
		memset(mem,0,size);
		for(i=0;i<10;i++){
			mem[i] = 0xaa+i;
			SHMLOG("mem[%d]=%d\n",i,mem[i]);
		}		
	}
	for(i=0;i<10;i++){
		if(mem[i] != 0xaa+i){
			
			SHMLOG("check not ok\n");
			SHMLOG("mem[%d]=%d\n",i,mem[i]);
			break;
		}
	}

	return mem;
	
}

int16_t i4s_shm_init(int initflag){
	shm_buf = createShm(MAXSHM,initflag);
	if(shm_buf == NULL){
		SHMLOG("shm_create error\n");
		return -1;
	}
	SHMLOG("shm_create ok1\n");
	return 0;
	
}


int16_t i4s_shm_write_api(int id,const uint8_t*buf,uint16_t len){
	//printf("write id=%d,shm_buf=%x,begin=%d,maxlen=%d\n",id,shm_buf,shm_conf[id].begin,shm_conf[id].maxlen);
	int16_t ret;
	pthread_mutex_lock(&mut);
	ret = i4s_shm_write(&shm_buf[shm_conf[id].begin],shm_conf[id].maxlen,buf,len);
	pthread_mutex_unlock(&mut);
	return ret;
}



int16_t i4s_shm_read_api(int id,uint8_t*buf,uint16_t maxreadlen){
	uint16_t ret;
	ret = i4s_shm_is_havedata(shm_buf + shm_conf[id].begin);
	if(ret == 0){
		return 0;
	}
	pthread_mutex_lock(&mut);
	ret = i4s_shm_read(shm_buf + shm_conf[id].begin,shm_conf[id].maxlen,buf,maxreadlen);
	pthread_mutex_unlock(&mut);
	return ret;
}




