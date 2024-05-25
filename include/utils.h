#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/globals.h"
#include "communication.h"
#include "assert.h"
#include "model_float.h"
#include "ml_utils.h"
#include <unistd.h>

int move_file(char* src, char* dest);

int download_file(char* link, char* dest);

int unzip_to_dest(char* source, char* dest);

int remove_file(char* file_path);

int remove_dir(char* src_dir);

void saveToDATFile(char* filename, Hyperparameter_t* hyperparameters);

void readFromDATFile(char* filename, Hyperparameter_t* product);

void load_Model_From_Folder(char* src_dir, LSTM_Service_T* lstm_model);

int load_Model_Service(char* download_link, LSTM_Service_T* lstm_service);

void save_lstm_service_to_SingleFile(char* dest_file, LSTM_Service_T* lstm_service);

BIN_FILE_RESULTS bin_file_complete(const char* filename);

BIN_FILE_RESULTS read_param_single_file(char* bin_filename, LSTM_Service_T* lstm_service);

cal_data_struct* load_input_seq_from_float_bin(char* src_file, int len, int dim);

cal_data_struct_test* load_test_input_seq_from_float_bin(char* src_file, int len, int dim);


//testing util functions using labeled data

cal_data_struct extract_input_seq_from_labeled_data(cal_data_struct_test input_with_label);

int extract_cnt_from_labeled_data(cal_data_struct_test input_with_label);

int extract_label_from_labeled_data(cal_data_struct_test input_with_label);


void save_lstm_input_to_float_bin(char* dest_file, cal_data_struct* input, int len, int dim);