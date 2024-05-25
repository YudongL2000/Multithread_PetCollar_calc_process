#ifndef ASYNC_H
#define AYNC_H

#include <pthread.h>
#include "../i4s_shm_api.h"
#include "ml_utils.h"

//Backend Calculations Type Definitions

typedef enum BackEnd_Status_T {
    BACKENDUNINTIALIZED = -1,
    BACKENDRUNNING = 0,
    BACKENDTERMINATING = 1,
    BACKENDTERMINATED = 2,
} BackEnd_Status_T;


typedef enum Aync_Calc_Status {
    InputUnitialized = -1,
    CalcWaiting = 0,
    CalcInProgress = 1,
    CalcFinished = 2,
} Aync_Calc_Status;

typedef enum Aysnc_Result_Status {
    ResultUnitialized = -1,
    ResultInUse = 0,
    ResultTaken = 1,
} Async_Result_Status;

typedef struct Async_Input {
    Aync_Calc_Status status; 
    cal_data_struct input;
} Async_Input;


typedef struct Async_Result {
    Async_Result_Status status;
    int cnt;
    int label;
} Async_Result;

typedef enum Calculation_Type{
	REALTIME_THREAD = 0,
	HISTORY_THREAD = 1
} Calculation_Type;



//mutexes and variables for controlling backend calculation
typedef struct Calculation_Thread_T {
    Calculation_Type type;
    BackEnd_Status_T status;
    LSTM_Service_T* service;
    pthread_t thread_id;
    pthread_mutex_t calculation_mutex;
    Async_Input input;
    pthread_mutex_t input_mutex;
    Async_Result result;
    pthread_mutex_t output_mutex;
} Calculation_Thread_T;


//CMD Thread Type Definitions


typedef int cmd_id_t;

typedef enum CMD_REPLY_T {
    CMD_EXEC_SUCCESS = 0,
    CMD_EXEC_FAILURE = -1,
    CMD_TYPE_UNDEFINED= -2,
} CMD_REPLY_T;

typedef enum _i4s_cmd_request_status_t {
	CMD_EMPTY = 0,
	CMD_PENDING = 1,
	CMD_FINISHED = 2,
	CMD_RESULT_TAKEN = 3,
	CMD_TERMINATED = -1
} __attribute__((__packed__)) i4s_cmd_request_status_t;

typedef struct i4s_cmd_thread_t {
	unsigned char cmdid_in_progress;
	int cmd_index_in_progress;
    char cmd_content_in_progress[BUFFLEN];
	i4s_cmd_request_status_t cmd_status;
	i4s_cmd cmd_input;
	i4s_cmd_reply cmd_output;
	pthread_mutex_t input_mutex;
	pthread_mutex_t output_mutex;
    pthread_mutex_t cmd_mutex;
	pthread_t thread_id;
}i4s_cmd_thread_t;










#endif