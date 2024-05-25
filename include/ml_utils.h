#ifndef MODEL_API_H
#define MODEL_API_H

#include <stdio.h>
#include "communication.h"
#include "../include/model_float.h"
// #include "queue.h"
#include "lstmInputList.h"
#include "historyLinkedList.h"
#include <unistd.h>

typedef int** Inertial_Matrix_T;
//extern int MAX_QUEUE_LEN;



typedef struct {
    Model_Type_T model_type;
    LSTM_float* LSTM_float;
    HistoryList* pred_history;
    int** inertial_matrix;
    DataList* lstm_input;
    Hyperparameter_t* hyperparameter;

    bool sleep_mode;
    //check the previous cnt for examining the state of the lstm
    int prev_cnt;
    //label that has been executing in the past
    int continous_mode;
    //the duration of the current displayed label
    int persistent_action_counter;
    //label that has just been predicted
    int new_action;
    //the duration of the new label
    int new_action_counter;
} LSTM_Service_T;

// 加载或者更新实时解算模型，如果需要从输入的链接进行下载
LSTM_Service_T* init_service(char* download_link, pthread_mutex_t* download_mutex_addr);

//利用模型进行解算
int perform_prediction(LSTM_Service_T* ML_Frame, cal_data_struct input);


//清除相应模型信息
void free_service(LSTM_Service_T* ML_Frame);

void retrieve_model_version (char* output_line);

void retrieve_model_local_path (char* output_line);

#endif