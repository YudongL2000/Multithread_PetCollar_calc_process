#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "globals.h"
#include "stdbool.h"
 


typedef struct BIN_FILE_HEADER {
    char STARTIO;
    int hyperparameter_offset;
    int transitional_matrix_offset;
    int model_parameter_offset;
    int filesize;
} BIN_FILE_HEADER;

typedef struct BIN_FILE_FOOTER {
    int filesize;
    char ENDIO;
} BIN_FILE_FOOTER;

typedef enum BIN_FILE_RESULTS {
    FILE_READING_VALID = 0,
    FILE_NOT_FOUND = -1,
    FILE_INCOMPLETE = -2,
    FILEREADER_FAILURE = -3,
    FILE_OFFSET_ERROR = -4,
    DOWNLOAD_NOT_EXIST = -5
} BIN_FILE_RESULTS;


typedef enum INIT_RETURN {
    INIT_SUCCESSFUL = 0,
    NEW_INVALID_DOWNLOAD = -1,
    PAST_INVALID_DOWNLOAD = -2,
    NEW_DOWNLOAD_FAILURE = -3,
} INIT_RETURN;

typedef struct cal_data_struct {
    uint32_t cnt;
    short data[6];
} cal_data_struct;

typedef struct cal_data_struct_test {
    uint32_t cnt;
    short data[6];
    uint32_t label;
} cal_data_struct_test;

typedef enum CAT_Prediction_T {
    CATWALK = 0,
    CATSLEEP = 1,
    CATRUN = 2,
    CATLICK = 3,
    CATPLAY = 4,
    CATJUMP = 5,
    CATFEED = 6,
    CATROLL = 7,
    CATSCRATCH = 8,
    CATREST = 9
} CAT_Prediction_T;


typedef enum DOG_Prediction_T {
    DOGTOY = 0,
    DOGJUMP = 1,
    DOGREST = 2,
    DOGWALK = 3,
    DOGSLEEP = 4,
    DOGFEED = 5,
    DOGRUN = 6,
    DOGTAIL = 7,
    DOGROLL = 8
} DOG_Prediction_T;


typedef enum Model_Type_T {
    CAT = 0,
    DOG_SMALL = 1,
    DOG_LARGE = 2,
} Model_Type_T;


typedef enum Status_T {
    SUCCESS = 0,
    MODEL_NOTFOUND = -1,
    DOWNLOAD_FAILURE = -2,
} Status_T;


typedef struct Hyperparameter_t {
    Model_Type_T model_type;
    int HISTORY_LEN;
    int LSTM_INPUT_LEN;
    int LSTM_LAYERS;
    int LSTM_INPUT_DIM;
    int LSTM_HIDDEN_DIM;
    int LSTM_OUTPUT_DIM;

    //if we have more than a certain number of rests, then we directly go to sleep
    int SLEEP_MODE_THRESH;
    bool USE_DIFF;

    //download and update model path
    char REL_MODEL_SAVE_PATH[BUFFLEN];
    char REL_TRANSITIONAL_SAVE_PATH[BUFFLEN];


    // char MODEL_DOWNLOAD_LINK[BUFFLEN];
    // char REL_SAVE_PATH[BUFFLEN];
    // char REL_ZIP_PATH[BUFFLEN];
    // char REL_UNZIP_PATH[BUFFLEN];
} Hyperparameter_t;

typedef enum CalculationError_T {
    INSUFFICIENTDATA = -2,
    UNINITIALIZED = -3,
    INITIALIZATIONFAILURE = -4,
    DISCONTINUOUSCNT = -5
} CalculationError_T;

typedef struct LATEST_INIT {
    char latest_link[BUFFLEN];
    BIN_FILE_RESULTS res;
} LATEST_INIT;

#endif