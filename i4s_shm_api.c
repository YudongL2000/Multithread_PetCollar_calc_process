#include "i4s_shm.h"
#include "stdio.h"
#include <stdlib.h>
#include "string.h"
#include <unistd.h>
#include "include/async.h"
#include "include/communication.h"
#include "i4s_shm_api.h"
#include "include/utils.h"
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <time.h>


static pthread_mutex_t cmd_buffer_mutex;
static pthread_mutex_t realtime_input_buffer_mutex;
static pthread_mutex_t history_input_buffer_mutex;


//Calculation Thread information
static i4s_cmd_thread_t cmd_processor;
static Calculation_Thread_T realtime_calculation;
static Calculation_Thread_T history_calculation;

static pthread_mutex_t download_history_mutex;
static LATEST_INIT* latest_download;

static pthread_mutex_t IO_mutex;
static bool mutex_initialized = false;

//////////////////////////////////////////
//                                      //
//     Measurement Helper Functions     // 
//                                      //
//////////////////////////////////////////
typedef struct {
	int day;
	int sec;
	int milisec;
} local_time_t;


static local_time_t get_current_time(int day) {
	local_time_t current_time;
	struct timeval tv;
    gettimeofday(&tv, NULL);
	current_time.day = day;
    current_time.sec = tv.tv_sec;
	current_time.milisec = tv.tv_usec / 1000;
    //printf("current time is day: %d, sec: %d, milisec: %d\n", current_time.day, current_time.sec, current_time.milisec);
    return current_time;
}

static local_time_t time_difference(local_time_t time_end, local_time_t time_start) {
	local_time_t diff;
	diff.day = time_end.day - time_start.day;
	diff.sec = time_end.sec - time_start.sec;
	diff.milisec = time_end.milisec - time_start.milisec;
	if (diff.milisec < 0) {
		diff.sec -= 1;
		diff.milisec += 1000;
	}
    //printf("time difference is day: %d, sec: %d, milisec: %d\n", diff.day, diff.sec, diff.milisec);
    return diff;
}

static long long local_time_to_milisec (local_time_t time) {
	long long res = 0;
	res += time.sec * 1000;
	res += time.milisec;
	return res;
}







//////////////////////////////////////////
//                                      //
//       Initialization Functions       // 
//                                      //
//////////////////////////////////////////
//********************************************************* Global Initialization and Termination Functions  ***************************************************

static void init_cmd_processor() {
    cmd_processor.cmdid_in_progress = '\0';
    cmd_processor.cmd_index_in_progress = -1;

    cmd_processor.cmd_input.cmdid = '\0';
    cmd_processor.cmd_input.index = -1;

    cmd_processor.cmd_output.cmdid = '\0';
    cmd_processor.cmd_output.index = -1;

    cmd_processor.cmd_status = CMD_EMPTY;
    cmd_processor.cmd_content_in_progress[0] = '\0';
    pthread_mutex_init(&(cmd_processor.input_mutex), NULL);
    pthread_mutex_init(&(cmd_processor.output_mutex), NULL);
    pthread_mutex_init(&(cmd_processor.cmd_mutex), NULL);
    cmd_processor.thread_id = -1;
}

static void destroy_cmd_processor_info() {
    pthread_mutex_destroy(&(cmd_processor.input_mutex));
    pthread_mutex_destroy(&(cmd_processor.output_mutex));
    pthread_mutex_destroy(&(cmd_processor.cmd_mutex));
    cmd_processor.thread_id = -1;
    cmd_processor.cmdid_in_progress = '\0';
    cmd_processor.cmd_index_in_progress = -1;
    cmd_processor.cmd_status = CMD_TERMINATED;
}

static void reset_backend_status(Calculation_Thread_T* calculation_thread) {
    calculation_thread -> status = BACKENDUNINTIALIZED;
    calculation_thread -> input.status = InputUnitialized;
    calculation_thread -> result.status = ResultUnitialized;
    calculation_thread -> service = NULL;
}

static void destroy_calculation_thread_info(Calculation_Thread_T* thread_info) {
    if (thread_info != NULL) {
		pthread_mutex_destroy(&(thread_info->input_mutex));
		pthread_mutex_destroy(&(thread_info->output_mutex));
		pthread_mutex_destroy(&(thread_info->calculation_mutex));
		thread_info = NULL;
	}
}


static void reset_realtime_thread_info() {
	realtime_calculation.type = REALTIME_THREAD;
    realtime_calculation.service = NULL;
    realtime_calculation.status = BACKENDUNINTIALIZED;
	realtime_calculation.status = -1;
	realtime_calculation.input.status = InputUnitialized;
	realtime_calculation.result.status = ResultUnitialized;
	pthread_mutex_init(&(realtime_calculation.input_mutex), NULL);
	pthread_mutex_init(&(realtime_calculation.output_mutex), NULL);
	pthread_mutex_init(&(realtime_calculation.calculation_mutex), NULL);
}


static void reset_history_thread_info() {
    history_calculation.type = HISTORY_THREAD;
    history_calculation.service = NULL;
	history_calculation.status = BACKENDUNINTIALIZED;
	history_calculation.status = -1;
	history_calculation.input.status = InputUnitialized;
	history_calculation.result.status = ResultUnitialized;
	pthread_mutex_init(&(history_calculation.input_mutex), NULL);
	pthread_mutex_init(&(history_calculation.output_mutex), NULL);
	pthread_mutex_init(&(history_calculation.calculation_mutex), NULL);
}



static void reset_download_history() {
    latest_download ->latest_link[0] = '\0';
    latest_download -> res = DOWNLOAD_NOT_EXIST;
}


static void init_global() {
    init_cmd_processor();
	reset_realtime_thread_info();
	reset_history_thread_info();
    latest_download = malloc(sizeof(LATEST_INIT));
    reset_download_history();
	mutex_initialized = true;
}






//************************************************************* Calculation Thread Functions  **********************************************

typedef struct NewThreadArgs_T {
    Calculation_Thread_T* thread_info;
} NewThreadArgs_T;


static void* backend_thread_calc(void* args) {
    //NewThreadArgs_T* local_thread_args  = (NewThreadArgs_T*) args;
    Calculation_Thread_T* content_info = (Calculation_Thread_T*)args;
    pthread_mutex_lock(&(content_info -> calculation_mutex));
    content_info->status = BACKENDRUNNING;
    pthread_mutex_unlock(&(content_info -> calculation_mutex));
    //printf("initializing backend thread with type %d\n", new_thread_args->thread_info->type);
    bool verbose = false;
    // if (content_info -> type == REALTIME_THREAD) {
    //     verbose = true;
    // }
    while (1) {
        //printf("start of a loop, backend status: %d\n", backend_status);
        pthread_mutex_lock(&(content_info -> calculation_mutex));
        BackEnd_Status_T current_status = content_info -> status;
        pthread_mutex_unlock(&(content_info -> calculation_mutex));

        if (verbose) {
            printf("thread type: %d, current status : %d\n", content_info ->type, content_info -> status);
        }
        
        if (current_status == BACKENDTERMINATING) {
            if (content_info -> type == REALTIME_THREAD) {
                printf("RealTime calculation Thread Terminating Signal Received\n");
            } else {
                printf("History calculation Thread Terminating Signal Received\n");
            }
            break;
        }

        bool need_calculation = false;
        cal_data_struct input_local;

        if (verbose) {
            printf("Checking Input Existance, current input status: %d\n", content_info -> input.status);
        }
        
        pthread_mutex_lock(&(content_info ->input_mutex));
        //printf("read from input\n");
        if (content_info -> input.status == CalcWaiting) {
            //printf("reading from input, current input status: %d\n", backend_input.status);
            content_info -> input.status = CalcInProgress;
            need_calculation = true;
            memcpy(&input_local, &(content_info -> input.input), sizeof(cal_data_struct));
        }
        pthread_mutex_unlock(&(content_info ->input_mutex));
        


        if (need_calculation) {
            if (verbose) {
                printf("placing input cnt: %d into the calculation thread content: [%d, %d, %d, %d, %d, %d]\n", input_local.cnt, input_local.data[0],input_local.data[1], input_local.data[2], input_local.data[3], input_local.data[4], input_local.data[5]);
                printf("Current Thread status: %d, Taking input for calculation, cnt: %d, status: %d\n", content_info->status, input_local.cnt, content_info -> input.status);
            }
            int res;
            
            pthread_mutex_lock(&(content_info ->calculation_mutex));
            if (content_info -> status == BACKENDUNINTIALIZED) {
                res = UNINITIALIZED;
            } else {
                if (content_info -> status == BACKENDRUNNING) {
                    res = perform_prediction(content_info ->service, input_local);
                    if (verbose) {
                        printf("Realtime current calculated result cnt: %d, result: %d from input  [%d, %d, %d, %d, %d, %d] \n", input_local.cnt, res, input_local.data[0],input_local.data[1], input_local.data[2], input_local.data[3], input_local.data[4], input_local.data[5]);
                    }
                }
            }

            pthread_mutex_unlock(&(content_info -> calculation_mutex));

            bool output_written = false;
            //printf("Writing output\n");
            while (output_written == false) {
                //printf("Waiting to write output status cnt: %d, current output status: %d\n", input_local.cnt, backend_output.status);
                //place result in the backend_output for takeout
                pthread_mutex_lock(&(content_info -> output_mutex));
                if (content_info -> result.status != ResultInUse) {
                    content_info -> result.label = res;
                    content_info -> result.cnt = input_local.cnt;
                    content_info -> result.status = ResultInUse;
                    output_written = true;
                }
                pthread_mutex_unlock(&(content_info -> output_mutex));

                if (output_written) {
                    pthread_mutex_lock(&(content_info ->input_mutex));
                    content_info ->input.status = CalcFinished;
                    pthread_mutex_unlock(&(content_info ->input_mutex));
                    
                    if (verbose) {
                        printf("Current Thread status: %d, Writing output for calculation, cnt: %d, current input status: %d\n", content_info->status, input_local.cnt, content_info -> input.status);
                    }
                }
				usleep(5000);
            }
        } 
		usleep(5000);
    }
    //printf("terminating, current status %d\n", backend_status);
    pthread_mutex_lock(&(content_info->calculation_mutex));
    content_info -> status = BACKENDTERMINATED;
    pthread_mutex_unlock(&(content_info->calculation_mutex));
    if (content_info -> type == REALTIME_THREAD) {
        printf("terminated realtime thread, current status %d\n", content_info ->status);
    } else {
        printf("terminated history thread, current status %d\n", content_info ->status);
    }
    return NULL;
}




//********************************* Communications between CMD Thread and Calculation Thread Helpers for Restarting and Updating Services *****************************************************
void terminate_backend_service(Calculation_Thread_T* calculation_thread) {
    //terminating backend for OTA
    pthread_mutex_lock(&(calculation_thread ->calculation_mutex));
    BackEnd_Status_T current_backend_status = calculation_thread -> status;
    pthread_mutex_unlock(&(calculation_thread -> calculation_mutex));

    if ((current_backend_status == BACKENDRUNNING)) {
        pthread_mutex_lock(&(calculation_thread -> calculation_mutex));
        calculation_thread -> status = BACKENDTERMINATING;
        pthread_mutex_unlock(&(calculation_thread -> calculation_mutex));
        //printf("backend running exists, thread id %ld\n", backend_tid);
        pthread_join(calculation_thread -> thread_id, NULL);
    }
    //printf("calculation thread terminated\n");
    pthread_mutex_lock(&(calculation_thread -> calculation_mutex));
    if (calculation_thread -> service != NULL) {
        free_service(calculation_thread -> service);
        calculation_thread -> service = NULL;
    }
    reset_backend_status(calculation_thread);
    pthread_mutex_unlock(&(calculation_thread -> calculation_mutex));
    //printf("service detached, termination of calculation thread %d successful\n", calculation_thread ->type);

}


static INIT_RETURN reinit_backend_service(Calculation_Thread_T* thread_info) {
    char latest_download_link[BUFFLEN];
    pthread_mutex_lock(&download_history_mutex);
    strcpy(latest_download_link, latest_download -> latest_link);
    pthread_mutex_unlock(&download_history_mutex);

    int res;
    int init_outcome;
    LSTM_Service_T* new_backend = NULL;
    char current_download_link [BUFFLEN];
    strcpy(current_download_link, latest_download_link);
    new_backend = init_service(current_download_link, &IO_mutex); 
    printf("service initialized, trying to check if replacement is valid\n");
    BackEnd_Status_T current_backend_status;

    if (new_backend != NULL) {
        terminate_backend_service(thread_info);
        pthread_mutex_lock(&(thread_info -> calculation_mutex));
        thread_info->service = new_backend;
        thread_info->status = BACKENDRUNNING;
        init_outcome = FILE_READING_VALID;
        res = INIT_SUCCESSFUL;
        pthread_mutex_unlock(&(thread_info -> calculation_mutex));
        printf("initialization complete, okay to replace the original model\n");
    } else {
        printf("initialization failure, use the original model\n");
        init_outcome = FILEREADER_FAILURE;
        res = NEW_DOWNLOAD_FAILURE;
    }
    pthread_mutex_lock(&download_history_mutex);
    latest_download->res = init_outcome;
    pthread_mutex_unlock(&download_history_mutex);
    if (res != INIT_SUCCESSFUL) {
        return res;
    } else {
        pthread_create(&(thread_info -> thread_id), NULL, backend_thread_calc, (void*)thread_info);
        printf("create new thread for running new backend service\n");
        return res;
    }
}



// static INIT_RETURN reload_backend_service(Calculation_Type thread_type) {
//     //check the previous download link
//     char latest_download_link[BUFFLEN];
    
//     pthread_mutex_lock(&download_history_mutex);
//     strcpy(latest_download_link, latest_download -> latest_link);
//     pthread_mutex_unlock(&download_history_mutex);

//     int res;
//     int init_outcome;
//     LSTM_Service_T* new_backend = NULL;
//     char current_download_link [BUFFLEN];
//     strcpy(current_download_link, latest_download_link);
//     new_backend = init_service(current_download_link, &IO_mutex); 
//     BackEnd_Status_T current_backend_status;
    
    
// 	// if (thread_type == REALTIME_THREAD) {
// 	// 	pthread_mutex_lock(&(realtime_calculation.calculation_mutex));
// 	// 	current_backend_status = realtime_calculation.status;
// 	// 	pthread_mutex_unlock(&(realtime_calculation.calculation_mutex));
// 	// } else if (thread_type == HISTORY_THREAD) {
// 	// 	pthread_mutex_lock(&(history_calculation.calculation_mutex));
// 	// 	current_backend_status = history_calculation.status;
// 	// 	pthread_mutex_unlock(&(history_calculation.calculation_mutex));
// 	// }

//     // //waiting for backend thread to finish
//     // if ((current_backend_status == BACKENDRUNNING) && new_backend!= NULL) {
//     //     if (thread_type == REALTIME_THREAD) {
//     //         //printf("Sending Terminating Signal to RealTime Thread\n");
//     //         pthread_mutex_lock(&realtime_calculation.calculation_mutex);
//     //         realtime_calculation.status = BACKENDTERMINATING;
//     //         pthread_mutex_unlock(&realtime_calculation.calculation_mutex);
//     //         pthread_join(realtime_calculation.thread_id, NULL);
//     //     } else if (thread_type == HISTORY_THREAD) {
//     //         //printf("Sending Terminating Signal to History Thread\n");
//     //         pthread_mutex_lock(&history_calculation.calculation_mutex);
//     //         history_calculation.status = BACKENDTERMINATING;
//     //         pthread_mutex_unlock(&history_calculation.calculation_mutex);
//     //         pthread_join(history_calculation.thread_id, NULL);
//     //     }
//     //     printf("calculation thread terminated\n");
//     // } 
//     if (thread_type == REALTIME_THREAD) {
//         if (new_backend != NULL) {
//             terminate_backend_service(&realtime_calculation);
//         }
//         pthread_mutex_lock(&realtime_calculation.calculation_mutex);
//         //realtime has been updated already
//         if (new_backend != NULL) {
//             reset_backend_status(&(realtime_calculation));
//             if (realtime_calculation.service != NULL) {
//                 free_service(realtime_calculation.service);
//                 realtime_calculation.service= NULL;
//             }
//             realtime_calculation.service = new_backend;
//             realtime_calculation.status = BACKENDRUNNING;
//             init_outcome = 0;
//             res = INIT_SUCCESSFUL;
//         } else {
//             init_outcome = 1;
//             res = NEW_DOWNLOAD_FAILURE;
//         }
//         pthread_mutex_unlock(&realtime_calculation.calculation_mutex);

//     } else if (thread_type == HISTORY_THREAD){
//         if (new_backend != NULL) {
//             terminate_backend_service(&history_calculation);
//         }
//         pthread_mutex_lock(&history_calculation.calculation_mutex);
//         //realtime has been updated already
//         if (new_backend != NULL) {
//             reset_backend_status(&(history_calculation));
//             if (history_calculation.service != NULL) {
//                 free_service(history_calculation.service);
//                 history_calculation.service= NULL;
//             }
//             history_calculation.service = new_backend;
//             history_calculation.status = BACKENDRUNNING;
//             init_outcome = 0;
//             res = INIT_SUCCESSFUL;
//         } else {
//             init_outcome = 1;
//             res = NEW_DOWNLOAD_FAILURE;
//         }
//         pthread_mutex_unlock(&history_calculation.calculation_mutex);
//     }
    
//     //update previous download link
//     pthread_mutex_lock(&download_history_mutex);
//     latest_download->res = init_outcome;
//     pthread_mutex_unlock(&download_history_mutex);
//     //printf("thread creating\n");

//     NewThreadArgs_T new_thread_args;
//     if (thread_type == REALTIME_THREAD) {
//         new_thread_args.thread_info = &realtime_calculation;
//         pthread_create(&(realtime_calculation.thread_id), NULL, backend_thread_calc, (void*)&new_thread_args);
//     } else if (thread_type == HISTORY_THREAD) {
//         new_thread_args.thread_info = &history_calculation;
//         pthread_create(&(history_calculation.thread_id), NULL, backend_thread_calc, (void*)&new_thread_args);
//     }
//     return res;
// }






//**************************************************************** CMD Thread Functions for handling Asynchronous cmd requests **********************************************

static void update_download_link(char* download_link) {
    pthread_mutex_lock(&download_history_mutex);
    strcpy(latest_download -> latest_link, download_link);
    pthread_mutex_unlock(&download_history_mutex);
}


static void finish_cmd_reply(int ret, char* msg, unsigned char cmdid, int cmd_index) {
    pthread_mutex_lock(&(cmd_processor.output_mutex));
    cmd_processor.cmd_output.cmdid = cmdid;
    cmd_processor.cmd_output.index = cmd_index;
    strcpy(cmd_processor.cmd_output.buf, msg);
    cmd_processor.cmd_output.ret = ret;
    pthread_mutex_unlock(&(cmd_processor.output_mutex));
}

static void* cmd_thread_service(void* args) {
    while (1) {
        bool new_input = false;
        unsigned char new_input_type = '\0';
        cmd_id_t new_input_index= -1;
        char new_content[BUFFLEN];
        new_content[0] = '\0';

        pthread_mutex_lock(&(cmd_processor.cmd_mutex));
        if (cmd_processor.cmd_status == CMD_PENDING) {
            new_input_type = cmd_processor.cmdid_in_progress;
            new_input_index = cmd_processor.cmd_index_in_progress;
            strcpy(new_content, cmd_processor.cmd_content_in_progress);
            new_input = true;
            //printf("reading from input source %s\n", new_content);
        } 
        pthread_mutex_unlock(&(cmd_processor.cmd_mutex));

        if (new_input) {
            CMD_REPLY_T ret;
            char msg[MSG_LEN];
            if (new_input_type == SHM_CMD_SHUTDOWN) {
                terminate_backend_service(&(realtime_calculation));
                terminate_backend_service(&(history_calculation));
                ret = CMD_EXEC_SUCCESS;
                msg[0] = '\0';
                finish_cmd_reply(ret, msg, new_input_type, new_input_index);
                break;
            } else {
                if (new_input_type == SHM_CMD_UPDATE_MODULE) {
                    bool same_as_last_download = false;

                    pthread_mutex_lock(&download_history_mutex);
                    if (strcmp(new_content, latest_download -> latest_link) == 0) {
                        same_as_last_download = true;
                        printf("loading the same file as the previous download instance\n");
                        if (latest_download -> res < 0) {
                            ret = CMD_EXEC_FAILURE;

                        } else {
                            ret = CMD_EXEC_SUCCESS;
                        }

                    } 
                    pthread_mutex_unlock(&download_history_mutex);

                    if (same_as_last_download == false) {
                        char prev_model_version [BUFFLEN];
                        pthread_mutex_lock(&download_history_mutex);
                        strcpy(prev_model_version, latest_download -> latest_link);
                        pthread_mutex_unlock(&download_history_mutex);

                        //update download history with the new link
                        update_download_link(new_content);
                        //printf("new link updated: %s\n", latest_download -> latest_link);
                        INIT_RETURN realtime_res = reinit_backend_service(&(realtime_calculation));
                        INIT_RETURN history_res =  reinit_backend_service(&(history_calculation));
                        //printf("finish realtime and history initialization\n");
                        if ((realtime_res >=0) && (history_res >=0)) {
                            char msg_output[MSG_LEN];
                            ret = CMD_EXEC_SUCCESS;
                            sprintf(msg, "%s;%d", new_content, ret);
                        } else {
                            char msg_output[MSG_LEN];
                            ret = CMD_EXEC_FAILURE;
                            sprintf(msg, "%s;%d", prev_model_version, ret);
                        }
                    }
                } else {
                    msg[0] = '\0';
                    //command not defined
                    ret = CMD_TYPE_UNDEFINED;
                }
                finish_cmd_reply(ret, msg, new_input_type, new_input_index);

                pthread_mutex_lock(&(cmd_processor.cmd_mutex));
                cmd_processor.cmd_status = CMD_FINISHED;
                cmd_processor.cmd_content_in_progress[0] = '\0';
                cmd_processor.cmd_index_in_progress = -1;
                cmd_processor.cmdid_in_progress = '\0';
                pthread_mutex_unlock(&(cmd_processor.cmd_mutex));
                //printf("output written with return type : %d, current processor status: %d\n", ret, cmd_processor.cmd_status);
            }
        } else {
            usleep(5000);
        }
    }
    pthread_mutex_lock(&(cmd_processor.cmd_mutex));
    cmd_processor.cmd_status = CMD_TERMINATED;
    pthread_mutex_unlock(&(cmd_processor.cmd_mutex));
    printf("CMD Thread Terminated\n");
    return NULL;
}



//**************************************************************** main thread functions  for handling cmd input requests and outputs **********************************************

static int clear_global() {
    //terminating backend for OTA
    pthread_mutex_lock(&IO_mutex);
    destroy_calculation_thread_info(&realtime_calculation);
    destroy_calculation_thread_info(&history_calculation);
    destroy_cmd_processor_info();
    pthread_mutex_unlock(&IO_mutex);
    
    pthread_mutex_lock(&download_history_mutex);
    free(latest_download);
    pthread_mutex_unlock(&download_history_mutex);

    pthread_mutex_destroy(&IO_mutex);
    pthread_mutex_destroy(&download_history_mutex);
    return 0;
}

static bool check_cmd_channel_ready() {
    bool res;
    if (cmd_processor.cmd_status == CMD_EMPTY || cmd_processor.cmd_status == CMD_RESULT_TAKEN) { 
        res = true;
    } else {
        res = false;
    }
    return res;
}


static void place_cmd_request(const i4s_cmd*cmd) {
    pthread_mutex_lock(&(cmd_processor.input_mutex));
    memcpy(&(cmd_processor.cmd_input), cmd, sizeof(i4s_cmd));
    pthread_mutex_unlock(&(cmd_processor.input_mutex));

    cmd_processor.cmd_status = CMD_PENDING;
    cmd_processor.cmd_index_in_progress = cmd->index;
    cmd_processor.cmdid_in_progress = cmd ->cmdid;
    strcpy(cmd_processor.cmd_content_in_progress, cmd->data);
    //printf("the new command link is %s\n", cmd_processor.cmd_content_in_progress);
}


static i4s_cmd_reply* check_and_collect_cmd_output() {
    i4s_cmd_reply* res = NULL;
    pthread_mutex_lock(&(cmd_processor.cmd_mutex));
    if (cmd_processor.cmd_status == CMD_FINISHED) {
        //printf("Taking output from cmd processor channel, result cmd: %d, cmd type: %d\n", cmd_processor.cmd_index_in_progress, cmd_processor.cmd_status);
        res = malloc(sizeof(i4s_cmd_reply));
        cmd_processor.cmd_status = CMD_RESULT_TAKEN;
        cmd_processor.cmd_index_in_progress = -1;
        cmd_processor.cmdid_in_progress = '\0';

        pthread_mutex_lock(&(cmd_processor.output_mutex));
        memcpy(res, &(cmd_processor.cmd_output), sizeof(i4s_cmd_reply));
        pthread_mutex_unlock(&(cmd_processor.output_mutex));

    } else if (cmd_processor.cmd_status == CMD_TERMINATED) {
        res = malloc(sizeof(i4s_cmd_reply));
        pthread_mutex_lock(&(cmd_processor.output_mutex));
        memcpy(res, &(cmd_processor.cmd_output), sizeof(i4s_cmd_reply));
        pthread_mutex_unlock(&(cmd_processor.output_mutex));
    }
    pthread_mutex_unlock(&(cmd_processor.cmd_mutex));
    return res;
}


static bool check_calculation_thread_running(Calculation_Thread_T* calculation_thread) {
    bool res;
    if (calculation_thread -> status == BACKENDRUNNING) {
        return true;
    }
    return false;
}

static bool check_calculation_input_channel_ready(Calculation_Thread_T* calculation_thread) {
    bool res = true;
    if ((calculation_thread -> input.status == CalcFinished) || (calculation_thread -> input.status == InputUnitialized)) {
        //printf("current calculation status on input: %d is %d\n", calculation_thread->input.input.cnt, calculation_thread->status);
        res = true;
    } else {
        res = false;
    }
    return res;
}


static bool place_new_calculation_data(const cal_data_struct input, Calculation_Thread_T* calculation_thread) {
    memcpy(&(calculation_thread->input.input), &input, sizeof(cal_data_struct));
    calculation_thread ->input.status = CalcWaiting;
}


static i4s_imu_data_reply* check_and_take_result_data(Calculation_Thread_T* calculation_thread, const int day) {
    i4s_imu_data_reply* res = NULL;
    if ((calculation_thread -> result.status == ResultInUse)) {
        res = malloc(sizeof(i4s_imu_data_reply));
        res -> count = calculation_thread -> result.cnt;
        res -> ret = calculation_thread -> result.label;
        res -> day = day;
		res -> latency = 0;
        // result already taken, change its status
        calculation_thread -> result.status = ResultTaken;
    }
    return res;
}


static i4s_imu_data_reply* produce_invalid_reading_reply(i4s_imu_data* input_data) {
    i4s_imu_data_reply* res = NULL;
    res = malloc(sizeof(i4s_imu_data_reply));
    res -> count = input_data ->count;
    res -> day = input_data -> day;
    res ->ret = UNINITIALIZED;
    res -> latency = 0;
    return res;
    
}

static void increment_request_cmd_index(int* request_cmd_index_addr) {
    int current_index = *request_cmd_index_addr;
    *request_cmd_index_addr = current_index - 1;
}

static i4s_cmd_reply* format_model_link_request(int command_index) {
    i4s_cmd_reply* res = NULL;
    res = malloc(sizeof(i4s_cmd_reply));
    res -> cmdid = REQUEST_MODEL_LINK;
    res -> index = command_index;
    res -> ret = 0; 
    return res;
}




void i4s_shm_api_updated(void) {
    i4s_shm_init(0);
    init_global();
    pthread_create(&(cmd_processor.thread_id), NULL, cmd_thread_service, NULL);
    int iter_counter = 0;
    int command_index = 0;
    
    char command_buf[1024];
    char command_reply[1024];

    char realtime_buf[1024];
    char realtime_reply[1024];
    
    char history_buf[1024];
    char history_reply[1024];

    int day_realtime = -1;
    int day_history = -1;
	int history_doing = 0;
	int live_doing = 0;
    i4s_cmd* cmd = (i4s_cmd*) command_buf;

	local_time_t live_start_time;
	local_time_t live_end_time;
	local_time_t history_start_time;
	local_time_t history_end_time;

    int request_cmd_index = -1;
    i4s_cmd_reply* new_model_request = format_model_link_request(request_cmd_index);
    memcpy((i4s_cmd_reply*)command_reply, new_model_request, sizeof(i4s_cmd_reply));
    i4s_shm_write_api(SHM_CMDDATA_REPLY, command_reply,sizeof(i4s_cmd_reply));
    free(new_model_request);

    while (1) {
        bool termination = false;
        pthread_mutex_lock(&(cmd_processor.cmd_mutex));
        bool cmd_ready = check_cmd_channel_ready();
        if (cmd_ready) {
            int read_ret = i4s_shm_read_api(SHM_CMDDATA,(uint8_t*)&command_buf[0],1024);
            //if cmd process is ready to handle inputs, read data from buffer
            if (read_ret > 0) {
                if (cmd -> cmdid == SHM_CMD_SHUTDOWN) {
                    //if cmd is OTA request, terminate the whole process for OTA 
                    termination = true;
                }
                place_cmd_request(cmd);
            }
        }
        pthread_mutex_unlock(&(cmd_processor.cmd_mutex));
        
        if (termination) {
            //if we're about to terminate the process, wait for the cmd thread to finish
            pthread_join(cmd_processor.thread_id, NULL);
            i4s_cmd_reply* cmd_ret = check_and_collect_cmd_output();
            if (cmd_ret != NULL) {
                memcpy((i4s_cmd_reply*)command_reply, cmd_ret, sizeof(i4s_cmd_reply));
                i4s_shm_write_api(SHM_CMDDATA_REPLY, command_reply,sizeof(i4s_cmd_reply));
                free(cmd_ret);
            }
            printf("Exiting main thread\n");
            break;
        } else {
            //if the cmd request is not terminate, we simply write it to the reply buffer
            i4s_cmd_reply* cmd_ret = check_and_collect_cmd_output();
            if (cmd_ret != NULL) {
                //printf("CMD OUTPUT: INDEX: %d, Command: %d, return value: %d\n", cmd_ret->index, cmd_ret ->cmdid, cmd_ret ->ret);
                memcpy((i4s_cmd_reply*)command_reply, cmd_ret, sizeof(i4s_cmd_reply));
                i4s_shm_write_api(SHM_CMDDATA_REPLY, command_reply,sizeof(i4s_cmd_reply));
                free(cmd_ret);
                continue;
            }
        }
        

		int realtime_data_ret = 0;
		bool unprepared_realtime_read = false;
        
        pthread_mutex_lock(&(realtime_calculation.calculation_mutex));
        pthread_mutex_lock(&(realtime_calculation.input_mutex));
        bool realtime_running = check_calculation_thread_running(&realtime_calculation);
        bool realtime_input_ready = check_calculation_input_channel_ready(&realtime_calculation);
        if (realtime_input_ready) {
            realtime_data_ret = i4s_shm_read_api(SHM_LIVE_DATA,(uint8_t*)&realtime_buf[0],1024);
            if (realtime_data_ret > 0) {
                if (realtime_running) {
                    unprepared_realtime_read = false;
                    //printf("feeding input to history thread: %d, the current history service status is %d, previous input status is %d, previous input cnt is: %d\n", input_data_list[history_iter % 300].cnt, history_calculation.status, history_calculation.input.status, history_calculation.input.input.cnt);
                    day_realtime = ((i4s_imu_data*)&realtime_buf[0]) -> day;
                    cal_data_struct input;
                    input.cnt = ((i4s_imu_data*)&realtime_buf[0]) -> count;
        			live_start_time = get_current_time(day_realtime);
                    memcpy(input.data, ((i4s_imu_data*)&realtime_buf[0]) -> data, sizeof(short) * 6);
                    place_new_calculation_data(input, &(realtime_calculation));
                    //printf("placing input cnt: %d into the realtime calculation thread content: [%d, %d, %d, %d, %d, %d]\n", input.cnt, input.data[0],input.data[1], input.data[2], input.data[3], input.data[4], input.data[5]);
                } else {
                    unprepared_realtime_read = true;
                }
            }
        }
        pthread_mutex_unlock(&(realtime_calculation.input_mutex));
        pthread_mutex_unlock(&(realtime_calculation.calculation_mutex));


        //printf("Checking realtime new output\n");

        if (unprepared_realtime_read) {
            //there're invalid input data in the cycle, output -1 as an unintialized response
            i4s_imu_data_reply* reply_realtime = produce_invalid_reading_reply((i4s_imu_data*)&realtime_buf[0]);
			live_doing = 0;
            i4s_shm_write_api(SHM_LIVE_DATA_REPLY,(i4s_imu_data_reply*)reply_realtime,sizeof(i4s_imu_data_reply));
            free(reply_realtime);
        } else {
            //Check if there're valid realtime output data, if there're place it inside the reply buffer and return it
            pthread_mutex_lock(&(realtime_calculation.output_mutex));
            i4s_imu_data_reply* reply_realtime = check_and_take_result_data(&realtime_calculation, day_realtime);

            if (reply_realtime != NULL) {
                printf("REALTIME OUTPUT: CNT:%d, RESULT: %d\n", reply_realtime->count, reply_realtime ->ret);
                live_end_time = get_current_time(day_realtime);
                int duration_in_millisec = local_time_to_milisec(time_difference(live_end_time, live_start_time));
                reply_realtime ->latency = duration_in_millisec;
				live_doing = 0;
                i4s_shm_write_api(SHM_LIVE_DATA_REPLY,(i4s_imu_data_reply*)reply_realtime,sizeof(i4s_imu_data_reply));
                free(reply_realtime);
            }
            pthread_mutex_unlock(&(realtime_calculation.output_mutex));
        }


        //printf("Finish Checking realtime new output\n");
        bool unprepared_history_read = false;
		int history_data_ret = 0;
        pthread_mutex_lock(&(history_calculation.calculation_mutex));
        pthread_mutex_lock(&(history_calculation.input_mutex));
        bool history_running = check_calculation_thread_running(&history_calculation);
        bool history_input_ready = check_calculation_input_channel_ready(&history_calculation);
        if (history_input_ready) {
            history_data_ret = i4s_shm_read_api(SHM_HISTORY_DATA,(uint8_t*)&history_buf[0],1024);
            if (history_data_ret > 0) {
                if (history_running) {
                    unprepared_history_read = false;
                    //printf("feeding input to history thread: %d, the current history service status is %d, previous input status is %d, previous input cnt is: %d\n", input_data_list[history_iter % 300].cnt, history_calculation.status, history_calculation.input.status, history_calculation.input.input.cnt);
                    day_history = ((i4s_imu_data*)&history_buf[0]) -> day;
                    cal_data_struct input;
                    input.cnt = ((i4s_imu_data*)&history_buf[0]) -> count;
                    history_start_time = get_current_time(day_history);
                    memcpy(input.data, ((i4s_imu_data*)&history_buf[0]) -> data, sizeof(short) * 6);
                    //printf("placing input cnt: %d into the history calculation thread content: [%d, %d, %d, %d, %d, %d]\n", input.cnt, input.data[0],input.data[1], input.data[2], input.data[3], input.data[4], input.data[5]);
                    place_new_calculation_data(input, &(history_calculation));
                } else {
                    //the history input channle is ready to take inputs, but the history calculation thread has not been initialized
                    unprepared_history_read = true;
                }
            }
        } 
        pthread_mutex_unlock(&(history_calculation.input_mutex));
        pthread_mutex_unlock(&(history_calculation.calculation_mutex));
        //printf("Finish Checking history new input\n");


        //printf("Checking history new output\n");
        //Check if there're realtime output data, if there're place it inside the reply buffer and return it
        if (unprepared_history_read) {
            //there're invalid input data in the cycle, output -1 as an unintialized response
            i4s_imu_data_reply* reply_history = produce_invalid_reading_reply((i4s_imu_data*)&history_buf[0]);
			history_doing = 0;
            i4s_shm_write_api(SHM_HISTORY_DATA_REPLY,(i4s_imu_data_reply*)reply_history,sizeof(i4s_imu_data_reply));
            free(reply_history);
        } else {
            pthread_mutex_lock(&(history_calculation.output_mutex));
            i4s_imu_data_reply* reply_history = check_and_take_result_data(&history_calculation, day_history);
            if (reply_history != NULL) {
                history_end_time = get_current_time(day_history);
                int duration_in_millisec = local_time_to_milisec(time_difference(history_end_time, history_start_time));
                reply_history ->latency = duration_in_millisec;
                printf("HISTORY OUTPUT: CNT:%d, RESULT: %d\n", reply_history->count, reply_history ->ret);
				history_doing = 0;
                i4s_shm_write_api(SHM_HISTORY_DATA_REPLY,(i4s_imu_data_reply*)reply_history,sizeof(i4s_imu_data_reply));
                free(reply_history);
            }
            pthread_mutex_unlock(&(history_calculation.output_mutex));
        }
		usleep(1000);
    }
    clear_global();
}




//*************************************************** In file test functions *******************************************************

void construct_new_cmd_input(int index, char* command_buf) {
    i4s_cmd* cmd = (i4s_cmd*) command_buf;
    cmd -> cmdid = SHM_CMD_UPDATE_MODULE;
    cmd -> index = index;
    char* download_link = "http://md.ccyz.cc/CAT_DIFF_5.bin";
    int datalen = strlen(download_link);
    cmd -> datalen = datalen;
    strcpy(cmd->data, download_link);
}

void construct_new_cmd_input_fake(int index, char* command_buf) {
    i4s_cmd* cmd = (i4s_cmd*) command_buf;
    cmd -> cmdid = SHM_CMD_UPDATE_MODULE;
    cmd -> index = index;
    char* download_link = "http://md.ccyz.cc/CAT_DIFF_xxx?version=1.0.0.bin";
    int datalen = strlen(download_link);
    cmd -> datalen = datalen;
    strcpy(cmd->data, download_link);
}


void construct_OTA_cmd_input(int index, char* command_buf) {
    i4s_cmd* cmd = (i4s_cmd*) command_buf;
    cmd -> cmdid = SHM_CMD_SHUTDOWN;
    cmd -> index = index;
    int datalen = 0;
    cmd -> datalen = datalen;
}


void construct_random_input(cal_data_struct input, int day, char* input_buf) {
    i4s_imu_data* data = (i4s_imu_data*) input_buf;
    data -> day = day;
    data -> count = input.cnt;
    memcpy(data->data, input.data, sizeof(short) * 6);
}




void test_lost_package() {
    init_global();
    srandom(10);
    pthread_create(&(cmd_processor.thread_id), NULL, cmd_thread_service, NULL);
    int iter_counter = 0;
    int command_index = 0;
    
    char command_buf[1024];
    char command_reply[1024];

    char realtime_buf[1024];
    char realtime_reply[1024];
    
    char history_buf[1024];
    char history_reply[1024];
    int realtime_iter = 0;
    int history_iter = 0;
    int cmd_ret = 0;
    int iters_till_termination = 300;
    int day = 19260817;
    int datalen = 150;
    cal_data_struct* input_data_list = load_input_seq_from_float_bin("test_data/short_input.bin", 300, 6);
    cal_data_struct* lost_packets = calloc(150, sizeof(cal_data_struct));

    int cnt_iterations = 0;
    int gap_idx = 100;
    for (int i = 0; i < 150; i++) {
        cal_data_struct tmp = input_data_list[cnt_iterations];
        lost_packets[i] = tmp;
        if (i == gap_idx) {
            cnt_iterations += 10;
        } else if ( i == gap_idx + 2 || i == gap_idx - 1) {
            cnt_iterations += 2;
        } else {
            cnt_iterations += 1;
        }
    }
    free(input_data_list);

    while(1){
        //printf("Looping in main thread\n");
        bool termination = false;
        if (realtime_iter % iters_till_termination == 0 && realtime_iter > 0) {
            pthread_mutex_lock(&(cmd_processor.cmd_mutex));
            bool cmd_ready = check_cmd_channel_ready();
            construct_OTA_cmd_input(command_index, command_buf);
            i4s_cmd* cmd = (i4s_cmd*) command_buf;
            if (cmd_ready) {
                //if cmd process is ready to handle inputs, read data from buffer
                if (cmd -> cmdid == SHM_CMD_SHUTDOWN) {
                    //if cmd is OTA request, terminate the whole process for OTA 
                    termination = true;
                }
                place_cmd_request(cmd);
            }
            pthread_mutex_unlock(&(cmd_processor.cmd_mutex));
            command_index += 1;
        }
        if (realtime_iter % 200 == 0) {
            pthread_mutex_lock(&(cmd_processor.cmd_mutex));
            //check if cmd channel is ready to handle inputs
            bool cmd_ready = check_cmd_channel_ready();
            construct_new_cmd_input(command_index, command_buf);
            i4s_cmd* cmd = (i4s_cmd*) command_buf;
            if (cmd_ready) {
                //if cmd process is ready to handle inputs, read data from buffer
                if (cmd -> cmdid == SHM_CMD_SHUTDOWN) {
                    //if cmd is OTA request, terminate the whole process for OTA 
                    termination = true;
                }
                place_cmd_request(cmd);
            }
            pthread_mutex_unlock(&(cmd_processor.cmd_mutex));
            command_index += 1;
        }
        //reading output status from cmd channel
        if (termination) {
            //if we're about to terminate the process, wait for the cmd thread to finish
            pthread_join(cmd_processor.thread_id, NULL);
            i4s_cmd_reply* cmd_ret = check_and_collect_cmd_output();
            if (cmd_ret != NULL) {
                memcpy((i4s_cmd_reply*)command_reply, cmd_ret, sizeof(i4s_cmd_reply));
                free(cmd_ret);
            }
            printf("Exiting main thread\n");
            iter_counter += 1;
            break;
        } else {
            //if the cmd request is not terminate, we simply write it to the reply buffer
            i4s_cmd_reply* cmd_ret = check_and_collect_cmd_output();
            if (cmd_ret != NULL) {
                //printf("CMD OUTPUT: INDEX: %d, Command: %d, return value: %d\n", cmd_ret->index, cmd_ret ->cmdid, cmd_ret ->ret);
                memcpy((i4s_cmd_reply*)command_reply, cmd_ret, sizeof(i4s_cmd_reply));
                free(cmd_ret);
                iter_counter += 1;
                //printf("Finish parsing inputs\n");
                continue;
            }
        }

        //printf("Checking realtime new input\n");
        pthread_mutex_lock(&(realtime_calculation.calculation_mutex));
        pthread_mutex_lock(&(realtime_calculation.input_mutex));
        bool realtime_running = check_calculation_thread_running(&realtime_calculation);
        bool realtime_ready = check_calculation_input_channel_ready(&realtime_calculation);
        if (realtime_running && realtime_ready) {
            //printf("Feeding input to realtime thread: %d, the current realtime service status is %d, previous input status is %d, previous input cnt is: %d\n", input_data_list[realtime_iter % 300].cnt, realtime_calculation.status, (&realtime_calculation)->input.status, realtime_calculation.input.input.cnt);
            cal_data_struct input = lost_packets[realtime_iter % 150];
            place_new_calculation_data(input, &(realtime_calculation));
            //printf("placing input cnt: %d into the realtime calculation thread content: [%d, %d, %d, %d, %d, %d]\n", input.cnt, input.data[0],input.data[1], input.data[2], input.data[3], input.data[4], input.data[5]);
            realtime_iter += 1;
        }
        pthread_mutex_unlock(&(realtime_calculation.input_mutex));
        pthread_mutex_unlock(&(realtime_calculation.calculation_mutex));
        //printf("Finish realtime checking new input\n");


        //printf("Checking realtime new output\n");
        //Check if there're realtime output data, if there're place it inside the reply buffer and return it
        pthread_mutex_lock(&(realtime_calculation.output_mutex));
        i4s_imu_data_reply* reply_realtime = check_and_take_result_data(&realtime_calculation, day);
        if (reply_realtime != NULL) {
            printf("REALTIME OUTPUT: CNT:%d, RESULT: %d\n", reply_realtime->count, reply_realtime ->ret);
            free(reply_realtime);
        }
        pthread_mutex_unlock(&(realtime_calculation.output_mutex));
        iter_counter += 1;
    }
    clear_global();
    free(lost_packets);

}

void test_single_thread() {
    FILE *output_file;
    output_file = fopen("stdout.txt", "w");

    init_global();
    pthread_create(&(cmd_processor.thread_id), NULL, cmd_thread_service, NULL);
    int iter_counter = 0;
    int command_index = 0;
    
    char command_buf[1024];
    char command_reply[1024];

    char realtime_buf[1024];
    char realtime_reply[1024];
    
    char history_buf[1024];
    char history_reply[1024];

    int day = 19260817;
    
    cal_data_struct_test* validation_data_list = load_test_input_seq_from_float_bin("/home/yudong/projects/EdgeComputing/MLServer/LSTM_raw_implementation/versions/package_fix_lost_cnt/test_data/dataset_data.bin", 10000, 6);
    printf("cnt: %d, index0: %d, index1: %d, index2: %d, index3: %d, index4: %d, index5: %d, label: %d\n",  validation_data_list[0].cnt, validation_data_list[0].data[0], validation_data_list[0].data[1], validation_data_list[0].data[2], validation_data_list[0].data[3], validation_data_list[0].data[4], validation_data_list[0].data[5], validation_data_list[0].label);
    int realtime_iter = 0;
    int history_iter = 0;
    int cmd_ret = 0;
    int iters_till_termination = 10000;
    int model_renew_gap = 10000;
    while(1){
        //printf("Looping in main thread\n");
        bool termination = false;
        if (realtime_iter % iters_till_termination == 0 && realtime_iter > 0) {
            pthread_mutex_lock(&(cmd_processor.cmd_mutex));
            bool cmd_ready = check_cmd_channel_ready();
            construct_OTA_cmd_input(command_index, command_buf);
            i4s_cmd* cmd = (i4s_cmd*) command_buf;
            if (cmd_ready) {
                //if cmd process is ready to handle inputs, read data from buffer
                if (cmd -> cmdid == SHM_CMD_SHUTDOWN) {
                    //if cmd is OTA request, terminate the whole process for OTA 
                    termination = true;
                }
                place_cmd_request(cmd);
            }
            pthread_mutex_unlock(&(cmd_processor.cmd_mutex));
            command_index += 1;
        }
        if (realtime_iter % model_renew_gap == 0) {
            pthread_mutex_lock(&(cmd_processor.cmd_mutex));
            //check if cmd channel is ready to handle inputs
            bool cmd_ready = check_cmd_channel_ready();
            construct_new_cmd_input(command_index, command_buf);
            i4s_cmd* cmd = (i4s_cmd*) command_buf;
            if (cmd_ready) {
                //if cmd process is ready to handle inputs, read data from buffer
                if (cmd -> cmdid == SHM_CMD_SHUTDOWN) {
                    //if cmd is OTA request, terminate the whole process for OTA 
                    termination = true;
                }
                place_cmd_request(cmd);
            }
            pthread_mutex_unlock(&(cmd_processor.cmd_mutex));
            command_index += 1;
        }
        //reading output status from cmd channel
        if (termination) {
            //if we're about to terminate the process, wait for the cmd thread to finish
            pthread_join(cmd_processor.thread_id, NULL);
            i4s_cmd_reply* cmd_ret = check_and_collect_cmd_output();
            if (cmd_ret != NULL) {
                memcpy((i4s_cmd_reply*)command_reply, cmd_ret, sizeof(i4s_cmd_reply));
                free(cmd_ret);
            }
            printf("Exiting main thread\n");
            iter_counter += 1;
            break;
        } else {
            //if the cmd request is not terminate, we simply write it to the reply buffer
            i4s_cmd_reply* cmd_ret = check_and_collect_cmd_output();
            if (cmd_ret != NULL) {
                //printf("CMD OUTPUT: INDEX: %d, Command: %d, return value: %d\n", cmd_ret->index, cmd_ret ->cmdid, cmd_ret ->ret);
                memcpy((i4s_cmd_reply*)command_reply, cmd_ret, sizeof(i4s_cmd_reply));
                free(cmd_ret);
                iter_counter += 1;
                //printf("Finish parsing inputs\n");
                continue;
            }
        }

        //printf("Checking realtime new input\n");
        pthread_mutex_lock(&(realtime_calculation.calculation_mutex));
        pthread_mutex_lock(&(realtime_calculation.input_mutex));
        bool realtime_running = check_calculation_thread_running(&realtime_calculation);
        bool realtime_ready = check_calculation_input_channel_ready(&realtime_calculation);
        if (realtime_running && realtime_ready) {
            //printf("Feeding input to realtime thread: %d, the current realtime service status is %d, previous input status is %d, previous input cnt is: %d\n", input_data_list[realtime_iter % 300].cnt, realtime_calculation.status, (&realtime_calculation)->input.status, realtime_calculation.input.input.cnt);
            cal_data_struct input = extract_input_seq_from_labeled_data(validation_data_list[realtime_iter]);
            int cnt = extract_cnt_from_labeled_data(validation_data_list[realtime_iter]);
            fprintf(output_file, "%d ", cnt);
            fprintf(output_file, "%d %d %d %d %d %d ", input.data[0], input.data[1], input.data[2], input.data[3], input.data[4], input.data[5]);
            place_new_calculation_data(input, &(realtime_calculation));
            //printf("placing input cnt: %d into the realtime calculation thread content: [%d, %d, %d, %d, %d, %d]\n", input.cnt, input.data[0],input.data[1], input.data[2], input.data[3], input.data[4], input.data[5]);
            realtime_iter += 1;
        }
        pthread_mutex_unlock(&(realtime_calculation.input_mutex));
        pthread_mutex_unlock(&(realtime_calculation.calculation_mutex));
        //printf("Finish realtime checking new input\n");


        //printf("Checking realtime new output\n");
        //Check if there're realtime output data, if there're place it inside the reply buffer and return it
        pthread_mutex_lock(&(realtime_calculation.output_mutex));
        i4s_imu_data_reply* reply_realtime = check_and_take_result_data(&realtime_calculation, day);
        if (reply_realtime != NULL) {
            fprintf(output_file, "%d\n", reply_realtime->ret);
            printf("REALTIME OUTPUT: CNT:%d, RESULT: %d\n", reply_realtime->count, reply_realtime ->ret);
            free(reply_realtime);
        }
        pthread_mutex_unlock(&(realtime_calculation.output_mutex));
        iter_counter += 1;
    }
    clear_global();
    free(validation_data_list);
    fclose(output_file);
}



void test_async() {
    init_global();
    pthread_create(&(cmd_processor.thread_id), NULL, cmd_thread_service, NULL);
    int iter_counter = 0;
    int command_index = 0;
    
    char command_buf[1024];
    char command_reply[1024];

    char realtime_buf[1024];
    char realtime_reply[1024];
    
    char history_buf[1024];
    char history_reply[1024];

    int day = 19260817;
    cal_data_struct* input_data_list = load_input_seq_from_float_bin("/home/yudong/projects/EdgeComputing/MLServer/LSTM_raw_implementation/package_dir/src/tmp/short_input.bin", 300, 6);
    int realtime_iter = 0;
    int history_iter = 0;
    int cmd_ret = 0;
    int iters_till_termination = 1000;
    int model_renew_gap = 300;
    bool test_fake = false;
    while(1){
        //printf("Looping in main thread\n");
        bool termination = false;
        if (realtime_iter % iters_till_termination == 0 && realtime_iter > 0) {
            pthread_mutex_lock(&(cmd_processor.cmd_mutex));
            bool cmd_ready = check_cmd_channel_ready();
            construct_OTA_cmd_input(command_index, command_buf);
            i4s_cmd* cmd = (i4s_cmd*) command_buf;
            if (cmd_ready) {
                //if cmd process is ready to handle inputs, read data from buffer
                if (cmd -> cmdid == SHM_CMD_SHUTDOWN) {
                    //if cmd is OTA request, terminate the whole process for OTA 
                    termination = true;
                }
                place_cmd_request(cmd);
            }
            pthread_mutex_unlock(&(cmd_processor.cmd_mutex));
            command_index += 1;
        }
        if (realtime_iter % model_renew_gap == 0) {
            //printf("renewing model\n");
            pthread_mutex_lock(&(cmd_processor.cmd_mutex));
            //check if cmd channel is ready to handle inputs
            bool cmd_ready = check_cmd_channel_ready();
            
            if (test_fake && (realtime_iter > 0)) {
                construct_new_cmd_input_fake(command_index, command_buf);
            } else {
                construct_new_cmd_input(command_index, command_buf);
            }
            
            i4s_cmd* cmd = (i4s_cmd*) command_buf;
            if (cmd_ready) {
                //if cmd process is ready to handle inputs, read data from buffer
                if (cmd -> cmdid == SHM_CMD_SHUTDOWN) {
                    //if cmd is OTA request, terminate the whole process for OTA 
                    termination = true;
                }
                place_cmd_request(cmd);
            }
            pthread_mutex_unlock(&(cmd_processor.cmd_mutex));
            command_index += 1;
        }
        //reading output status from cmd channel
        if (termination) {
            //if we're about to terminate the process, wait for the cmd thread to finish
            pthread_join(cmd_processor.thread_id, NULL);
            i4s_cmd_reply* cmd_ret = check_and_collect_cmd_output();
            if (cmd_ret != NULL) {
                memcpy((i4s_cmd_reply*)command_reply, cmd_ret, sizeof(i4s_cmd_reply));
                free(cmd_ret);
            }
            printf("Exiting main thread\n");
            iter_counter += 1;
            break;
        } else {
            //if the cmd request is not terminate, we simply write it to the reply buffer
            i4s_cmd_reply* cmd_ret = check_and_collect_cmd_output();
            if (cmd_ret != NULL) {
                //printf("CMD OUTPUT: INDEX: %d, Command: %d, return value: %d\n", cmd_ret->index, cmd_ret ->cmdid, cmd_ret ->ret);
                memcpy((i4s_cmd_reply*)command_reply, cmd_ret, sizeof(i4s_cmd_reply));
                printf("Command output: %s\n", ((i4s_cmd_reply*)command_reply) -> buf);
                free(cmd_ret);
                iter_counter += 1;
                //printf("Finish parsing inputs\n");
                continue;
            }
        }

        //printf("Checking realtime new input\n");
        pthread_mutex_lock(&(realtime_calculation.calculation_mutex));
        pthread_mutex_lock(&(realtime_calculation.input_mutex));
        bool realtime_running = check_calculation_thread_running(&realtime_calculation);
        bool realtime_ready = check_calculation_input_channel_ready(&realtime_calculation);
        if (realtime_running && realtime_ready) {
            //printf("Feeding input to realtime thread: %d, the current realtime service status is %d, previous input status is %d, previous input cnt is: %d\n", input_data_list[realtime_iter % 300].cnt, realtime_calculation.status, (&realtime_calculation)->input.status, realtime_calculation.input.input.cnt);
            cal_data_struct input = input_data_list[realtime_iter % 300];
            place_new_calculation_data(input, &(realtime_calculation));
            //printf("placing input cnt: %d into the realtime calculation thread content: [%d, %d, %d, %d, %d, %d]\n", input.cnt, input.data[0],input.data[1], input.data[2], input.data[3], input.data[4], input.data[5]);
            realtime_iter += 1;
        }
        pthread_mutex_unlock(&(realtime_calculation.input_mutex));
        pthread_mutex_unlock(&(realtime_calculation.calculation_mutex));
        //printf("Finish realtime checking new input\n");


        //printf("Checking realtime new output\n");
        //Check if there're realtime output data, if there're place it inside the reply buffer and return it
        pthread_mutex_lock(&(realtime_calculation.output_mutex));
        i4s_imu_data_reply* reply_realtime = check_and_take_result_data(&realtime_calculation, day);
        if (reply_realtime != NULL) {
            printf("REALTIME OUTPUT: CNT:%d, RESULT: %d\n", reply_realtime->count, reply_realtime ->ret);
            free(reply_realtime);
        }
        pthread_mutex_unlock(&(realtime_calculation.output_mutex));

        //printf("Finish Checking realtime new output\n");
        
        pthread_mutex_lock(&(history_calculation.calculation_mutex));
        pthread_mutex_lock(&(history_calculation.input_mutex));
        bool history_running = check_calculation_thread_running(&history_calculation);
        bool history_ready = check_calculation_input_channel_ready(&history_calculation);
        if (history_running && history_ready) {
            cal_data_struct input = input_data_list[history_iter % 300];
            place_new_calculation_data(input, &(history_calculation));
            history_iter += 1;
        }
        pthread_mutex_unlock(&(history_calculation.input_mutex));
        pthread_mutex_unlock(&(history_calculation.calculation_mutex));
        pthread_mutex_lock(&(history_calculation.output_mutex));
        i4s_imu_data_reply* reply_history = check_and_take_result_data(&history_calculation, day);
        if (reply_history != NULL) {
            printf("HISTORY OUTPUT: CNT:%d, RESULT: %d\n", reply_history->count, reply_history ->ret);
            free(reply_history);
        }
        pthread_mutex_unlock(&(history_calculation.output_mutex));
        iter_counter += 1;
    }
    clear_global();
    free(input_data_list);
}



//*************************************************** Original Test functions *******************************************************


void handle_cmd_reply(const i4s_cmd*cmd,i4s_cmd_reply*reply){
	//updata cmd
	switch(cmd->cmdid){
		case SHM_CMD_SHUTDOWN:
			break;
		case SHM_CMD_UPDATE_MODULE:
			break;
		default:
			break;
	}
	return;
}
void handle_live_data(const i4s_imu_data*data,i4s_imu_data_reply*reply){
	reply->day = data->day;
	reply->count = data->count;

	return;
}
void handle_history_data(const i4s_imu_data*data,i4s_imu_data_reply*reply){
	reply->day = data->day;
	reply->count = data->count;
	return;
}


void i4s_shm_api(void){
	i4s_shm_init(0);
	int ret;
	char tmpbuf[1024];
	uint8_t replybuf[1024];
	i4s_cmd *cmd = (i4s_cmd *)tmpbuf;
	i4s_imu_data *data = (i4s_imu_data *)tmpbuf;
	while(1){
		ret = i4s_shm_read_api(SHM_CMDDATA,(uint8_t*)&tmpbuf[0],1024);
		if(ret > 0){
			handle_cmd_reply(cmd,(i4s_cmd_reply*)replybuf);
			i4s_shm_write_api(SHM_CMDDATA_REPLY,replybuf,sizeof(i4s_cmd_reply));
			continue;
		}
		ret = i4s_shm_read_api(SHM_LIVE_DATA,(uint8_t*)&tmpbuf[0],1024);
		
		if(ret > 0){
			//printf("read len=%d\n",ret);
			handle_live_data(data,(i4s_imu_data_reply*)replybuf);
			//printf("write len=%d\n",sizeof(i4s_imu_data_reply));
			i4s_shm_write_api(SHM_LIVE_DATA_REPLY,replybuf,sizeof(i4s_imu_data_reply));
			continue;
		}
		ret = i4s_shm_read_api(SHM_HISTORY_DATA,(uint8_t*)&tmpbuf[0],1024);
		if(ret > 0){

			handle_history_data(data,(i4s_imu_data_reply*)replybuf);
			i4s_shm_write_api(SHM_LIVE_DATA_REPLY,replybuf,sizeof(i4s_imu_data_reply));

			continue;
		}
		usleep(1000);
		
	}
}


void test_timer() {
    int day = 0;
    local_time_t current_time = get_current_time(day);
    usleep(10000);
    local_time_t finish_time = get_current_time(day);
    local_time_t diff = time_difference(finish_time, current_time);
    int milisec_counter = local_time_to_milisec(diff);
    printf("current time gap: %d\n", milisec_counter);
}

int main(int argc, char**argv){
	//i4s_shm_api_updated(); 
	test_lost_package();
    //test_single_thread();
    //test_async();
	//test_calculation_memory_leak();
    //test_timer();
    return 0;
}

