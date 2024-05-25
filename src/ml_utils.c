#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/communication.h"
#include <unistd.h>
#include "../include/utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>



//************************************ IO helper functions ***************************************************

static int is_folder(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        // Error occurred, unable to get file information
        return -1;
    }
    return S_ISDIR(path_stat.st_mode);
}

//check if a path has a folder, if not, create one 
static int create_folder(char* dest_folder) {
    int folder_exists = access(dest_folder, F_OK);
    if (folder_exists == -1) {
        //create a folder if it does not exist
        if (mkdir(dest_folder, 0777) == 0) {
            return 0;
        } else {
            printf("failed to create download folder at %s\n", dest_folder);
            return -1;
        }
    } else {
        int status = is_folder(dest_folder);
        if (status == 1) {
            printf("download folder %s already exists\n", dest_folder);
            return 0;
        } else {
            if (remove(dest_folder) == 0) {
                printf("File '%s' removed successfully for folder creation in the same directory.\n", dest_folder);
            } else {
                printf("Failed to remove file '%s' when creating folder in the same location.\n", dest_folder);
                perror("Error");
                return -1;
            }
            //create a folder if it does not exist
            if (mkdir(dest_folder, 0777) == 0) {
                return 0;
            } else {
                printf("failed to create download folder at %s\n", dest_folder);
                return -1;
            }
        }
    }
}


static char* getFilenameFromURL(char* url) {
    char* filename = strrchr(url, '/'); // Find the last occurrence of '/'
    if (filename == NULL) // If '/' not found, return the entire URL
        return url;
    else
        return filename + 1; // Return the substring after '/'
}


static float short_to_float(short value) {
    return (float)value;
}

static int store_link_to_file(char* filename, char* link){
    // Open a file in write mode ("w")
    FILE *file = fopen(filename, "w");

    // Check if the file was opened successfully
    if (file == NULL) {
        printf("Error opening the file.\n");
        return 1;
    }
    // Write the string to the file
    fputs(link, file);

    // Close the file
    fclose(file);

    return 0;
}

static int read_link_from_file (char* filename, char* line){
    FILE *file;
    // Open the file
    file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }
    // Read lines from the file
    if (fgets(line, BUFFLEN - 1, file) != NULL) {
        if(line[strlen(line) -1] == '\n') {
            line[strlen(line) - 1] = '\0';
        }
        fclose(file);
        return 0;
    } else {
        fclose(file);
        return -1;
    }
}

void retrieve_model_version (char* output_line){
    char* filename = "version.txt";
    read_link_from_file(filename, output_line);
    printf("getting current version link path: %s\n", output_line);
}


void retrieve_model_local_path (char* output_line){
    char* filename = "param_path.txt";
    read_link_from_file(filename, output_line);
    printf("getting model local path: %s\n", output_line);
}



static void remove_all_downloaded_files() {
    char cwd [BUFFLEN];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return;
    }
    char download_dest [BUFFLEN];
    strcpy(download_dest, cwd);
    strcat(download_dest, "/");
    strcat(download_dest, "download");

    char* model_link_file = "version.txt";
    char* model_path_file = "param_path.txt";
    char* tmp_folder = "tmp";
    
    rmdir(tmp_folder);
    char current_model_path[BUFFLEN];
    read_link_from_file(model_path_file, current_model_path);
    if (access(current_model_path, F_OK) != -1) {
        remove(current_model_path);
    }
    remove(model_link_file);
    remove(model_path_file);
}





static LSTM_Service_T* load_model_from_single_file(char* src_filename, int max_iters, pthread_mutex_t* download_mutex_addr) {
    if (access(src_filename, F_OK) != -1) {
        int iter_counter = 0;
        bool file_valid = false;
        printf("Looping for %d iterations for checking file status\n", max_iters);
        while (true) {

            pthread_mutex_lock(download_mutex_addr);
            BIN_FILE_RESULTS file_status = bin_file_complete(src_filename);
            pthread_mutex_unlock(download_mutex_addr);

            if (file_status == FILE_READING_VALID) {
                file_valid = true;
                break;
            } 
            printf("current status %d\n", file_status);
            iter_counter += 1;
            if (iter_counter > max_iters) {
                return NULL;
            }
            sleep(1);
        }
        LSTM_Service_T* lstm_service = malloc(sizeof(LSTM_Service_T));
        lstm_service -> hyperparameter = NULL;

        pthread_mutex_lock(download_mutex_addr);
        BIN_FILE_RESULTS params_reading_status = read_param_single_file(src_filename, lstm_service);
        pthread_mutex_unlock(download_mutex_addr);

        if (params_reading_status != FILE_READING_VALID) {
            free(lstm_service);
            return NULL;
        } else {
            return lstm_service;
        }
    } else {
        printf("File %s does not exist.\n", src_filename);
        return NULL;
    }
}



//************************************ Hyperparameters functions ***************************************************

static void free_hyperparameters(Hyperparameter_t* hyperparameters) {
    free(hyperparameters);
}

//************************************ calculation helper functions ***************************************************

static gsl_matrix_float* create_input(DataList* input_list, Hyperparameter_t* hyperparameters) {
    gsl_matrix_float* model_input;
    if (hyperparameters -> USE_DIFF == false) {
        //printf("create input using None DIF\n");
        gsl_matrix_float* model_input = gsl_matrix_calloc(hyperparameters -> LSTM_INPUT_LEN, hyperparameters -> LSTM_INPUT_DIM);
        ShortNode* lstm_node = input_list->head;
        int counter = 0;
        while (lstm_node != NULL) {
            short* data_payload = lstm_node -> LSTM_input;
            for (int k = 0; k < hyperparameters -> LSTM_INPUT_DIM; k++) {
                gsl_matrix_set(model_input, counter, k, data_payload[k]);
            }
            counter += 1;
            lstm_node = lstm_node -> next;
        }
        assert(counter == hyperparameters -> LSTM_INPUT_LEN);
        return model_input;
    } else {
        //printf("create input using DIFF\n");
        gsl_matrix_float* model_input = gsl_matrix_calloc(hyperparameters -> LSTM_INPUT_LEN, hyperparameters -> LSTM_INPUT_DIM);
        ShortNode* lstm_node = input_list->head;
        int counter = 0;
        short* prev_payload = NULL;
        while (lstm_node != NULL) {
            short* data_payload = lstm_node -> LSTM_input;
            if (prev_payload != NULL) {
                for (int k = 0; k < hyperparameters -> LSTM_INPUT_DIM; k++) {
                    gsl_matrix_set(model_input, counter, k, data_payload[k] - prev_payload[k]);
                }
                counter += 1;
            }
            prev_payload = data_payload;
            lstm_node = lstm_node -> next;
        }
        assert(counter == hyperparameters -> LSTM_INPUT_LEN);
        return model_input;
    }
}




static int process_new_prediction(LSTM_Service_T* ML_Frame, int current_pred_label) {
    if (ML_Frame == NULL || ML_Frame -> pred_history == NULL || current_pred_label < 0) {
        return -1;
    }
    //there was no moves displayed before, start accumulating
    if (ML_Frame -> continous_mode == -1) {
        ML_Frame -> continous_mode = current_pred_label; 
        ML_Frame -> persistent_action_counter = 1;
        ML_Frame -> new_action_counter = 0;
        ML_Frame -> new_action = -1;
        return ML_Frame -> continous_mode;
    }

    if (current_pred_label == ML_Frame -> continous_mode) {
        ML_Frame -> persistent_action_counter += 1;
        ML_Frame -> new_action_counter = 0;
        ML_Frame -> new_action = -1;
    } else {
        //we have a new action that's been predicted
        if (ML_Frame -> new_action == current_pred_label) {
            //the action has already been predicted for a while, add to accumulator for transition
            ML_Frame -> new_action_counter += 1;
            // if (ML_Frame -> new_action == -1) {
            //     printf("case 2 backtrace meeting invalid 1\n");
            // }
        } else {
            //the action is totally new, we need to reset the transitional accumulator
            ML_Frame -> new_action_counter = 1;
            ML_Frame -> new_action = current_pred_label; 
            // if (ML_Frame -> new_action == -1) {
            //     printf("case 2 backtrace meeting invalid 3\n");
            // }
        }
        //matrix [i, j]: the length of persistant moves needed to transit from current displaying move i to the new move j
        int transitional_move_needed = ML_Frame -> inertial_matrix[ML_Frame -> continous_mode][current_pred_label];
        
        //printf("continous mode: %d; new action: %d; current_pred: %d\n", ML_Frame -> continous_mode, ML_Frame->new_action, current_pred_label);
        if (ML_Frame -> new_action_counter >= transitional_move_needed) {
            if (ML_Frame -> new_action == -1) {
                //printf("case 2 backtrace meeting invalid 2, with new action counter: %d\n", ML_Frame -> new_action_counter);
            }
            //we have hit enough moves for a transition
            ML_Frame -> continous_mode = ML_Frame -> new_action;
            ML_Frame -> persistent_action_counter = ML_Frame -> new_action_counter;
            ML_Frame -> new_action_counter = 0;
            ML_Frame -> new_action = -1;
        }
        // if (ML_Frame -> continous_mode == -1) {
        //     printf("case 2\n");
        // }
    }
    return ML_Frame -> continous_mode;
}


static int sleep_actions(LSTM_Service_T* ML_Frame) {
    if (ML_Frame -> model_type == CAT) {
        return CATSLEEP;
    } else if (ML_Frame -> model_type == DOG_LARGE || ML_Frame -> model_type == DOG_SMALL) {
        return DOGSLEEP;
    }
}

static void update_sleep_mode(LSTM_Service_T* ML_Frame) {
    //enter sleep mode if the animal has been sleeping or resting for a certain period of time
    if (ML_Frame -> model_type == CAT && (ML_Frame -> continous_mode == CATSLEEP || ML_Frame -> continous_mode == CATREST)){
        if (ML_Frame -> persistent_action_counter >= ML_Frame -> hyperparameter -> SLEEP_MODE_THRESH) {
            ML_Frame -> sleep_mode = true;
        }
    } else if ((ML_Frame->model_type == DOG_LARGE && ML_Frame-> model_type == DOG_SMALL) && (ML_Frame -> continous_mode == DOGSLEEP || ML_Frame -> continous_mode == DOGREST)){
        if (ML_Frame -> persistent_action_counter >= ML_Frame -> hyperparameter -> SLEEP_MODE_THRESH) {
            ML_Frame -> sleep_mode = true;
        }
    } else {
        // if the cat or dog is awoke (making othe moves rather than rest or sleep), terminate sleep mode directly
        ML_Frame -> sleep_mode = false;
    }
}



//************************************ API helper functions ***************************************************

LSTM_Service_T* init_service(char* download_link, pthread_mutex_t* download_mutex_addr) {
    //get currrent working directory for setting downloading path
    char cwd [BUFFLEN];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return NULL;
    }
    char download_dest [BUFFLEN];
    strcpy(download_dest, cwd);
    strcat(download_dest, "/");
    strcat(download_dest, "download");
    
    char* tmp_download_dest = "tmp";
    char* model_link_file = "version.txt";
    char* model_path_file = "param_path.txt";

    if (create_folder(download_dest) == -1) {
        perror("failed to create dest folder for storing model parameters\n");
        return NULL;
    }

    if (create_folder(tmp_download_dest) == -1) {
        perror("failed to create folder for downloading model parameters\n");
        return NULL;
    }

    LSTM_Service_T* res;
    bool new_model = true;
    //char* loading_src = "download/parameters.bin";

    //check download files
    pthread_mutex_lock(download_mutex_addr);
    if (access(model_link_file, F_OK) == -1 || access(model_path_file, F_OK) ==-1) {
        //first time init
        new_model = true;
    } else {
        char prev_link[BUFFLEN];
        read_link_from_file(model_link_file, prev_link);
        if (strcmp(prev_link, download_link)==0) {
            printf("model already downloaded, loading from local\n");
            char prev_model_dir[BUFFLEN];
            read_link_from_file(model_path_file, prev_model_dir);
            if (access(prev_model_dir, F_OK) == 0) {
                BIN_FILE_RESULTS loading_status = bin_file_complete(prev_model_dir);
                //printf("loading from %s, returned status: %d\n *****************\n", prev_model_dir, loading_status);
                if (loading_status == FILE_READING_VALID) {
                    new_model = false;
                } else {
                    new_model = true;
                }
            } else {
                new_model = true;
            }
        } else {
            new_model = true;
        }
    }

    int status = 0;
    char src_bin_filepath [BUFFLEN];
    bool need_replacement = false;
    //set the source bin path to be zero length
    src_bin_filepath[0] = '\0';
    if (new_model) {
        //extracting filename from URL
        char* filename = getFilenameFromURL(download_link);

        // tmp folder for storing files that has just been downloaded, including params, model files
        char tmp_download_filename[BUFFLEN];
        strcpy(tmp_download_filename, tmp_download_dest);
        strcat(tmp_download_filename, "/");
        strcat(tmp_download_filename, filename);
        printf("downloading file to %s\n", tmp_download_filename);
        download_file(download_link,tmp_download_dest);
        BIN_FILE_RESULTS bin_file_res = bin_file_complete(tmp_download_filename);
        if (bin_file_res == FILE_READING_VALID) {
            printf("model file is valid and ready to be loaded\n");
            //if the model is new and the previous model indeed exists, then remove the previous model to save space in disk
            if (access(model_path_file, F_OK) == 0) {
                char prev_model_path[BUFFLEN];
                read_link_from_file(model_path_file, prev_model_path);
                if (access(prev_model_path, F_OK) == 0) {
                    remove(prev_model_path);
                } 
            }
            
            need_replacement = true;
            //final file dest for adding new model
            char download_filename[BUFFLEN];
            strcpy(download_filename, download_dest);
            strcat(download_filename, "/");
            strcat(download_filename, filename);
            printf("moving file to %s\n", download_filename);
            rename(tmp_download_filename, download_filename);

            
            //if we need to update the model, then we remove the model link and model path files to replace them with the new model link and path
            if (access(model_link_file, F_OK) != -1) {
                //printf("removing current link file\n");
                remove(model_link_file);
            }
            if (access(model_path_file, F_OK) != -1) {
                //printf("removing current path file\n");
                remove(model_path_file);
            }
            store_link_to_file(model_link_file, download_link);
            store_link_to_file(model_path_file, download_filename);
            strcpy(src_bin_filepath, download_filename);
            printf("downloaded files moved from tmp to local folder\n");
        } else {
            need_replacement = false;
        }
        remove(tmp_download_filename);
        printf("tmp file removed from tmp folder\n");
    } else {
        need_replacement = false;
        char prev_model_path[BUFFLEN];
        read_link_from_file(model_path_file, prev_model_path);
        strcpy(src_bin_filepath, prev_model_path);
    }
    pthread_mutex_unlock(download_mutex_addr);
    
    if (strlen(src_bin_filepath) > 0) {
        printf("start loading model from downloaded folder\n");
        res = load_model_from_single_file(src_bin_filepath, 30, download_mutex_addr);
        printf("finish loading model from downloaded folder\n");
    } else {
        printf("******************File invalid ****************\n");
        res = NULL;
    }
    return res;
}




cal_data_struct weighted_avg(cal_data_struct data_head, cal_data_struct data_tail, int current_cnt) {
    cal_data_struct output;
    output.cnt = current_cnt;
    for (int i = 0; i < DEFAULT_LSTM_INPUT_DIM; i++) {
        float weighted_tmp = (data_head.data[i] * (data_tail.cnt - current_cnt) + data_tail.data[i] * (current_cnt - data_head.cnt));
        float res = weighted_tmp / (float) (data_tail.cnt - data_head.cnt);
        output.data[i] = (short) res;
    }
    return output;
}


int perform_prediction(LSTM_Service_T* ML_Frame, cal_data_struct input) {
    //print new input data 
    //printf("New Input: %d, %d, %d, %d, %d, %d\n", input.data[0], input.data[1], input.data[2], input.data[3], input.data[4], input.data[5]);
    if (ML_Frame == NULL) {
        return UNINITIALIZED;
    } 

    if (ML_Frame -> prev_cnt >= 0 && (input.cnt - ML_Frame ->prev_cnt > DEFAULT_CNT_GAP_TOLERANCE) ) {
        clear_list(ML_Frame ->lstm_input);
        add_to_tail(ML_Frame -> lstm_input, input.data);
        ML_Frame -> prev_cnt = input.cnt;
        ML_Frame -> continous_mode = -1;
        ML_Frame -> persistent_action_counter = 0;
        ML_Frame -> new_action = -1;
        ML_Frame -> new_action_counter = 0;
        return DISCONTINUOUSCNT;
    } 

    // discontinous cnt, but within tolerance, therefore we take weighted average
    if ((ML_Frame -> prev_cnt >= 0) && (input.cnt - ML_Frame -> prev_cnt > 1)) {  
        int tail_cnt = ML_Frame -> prev_cnt;
        cal_data_struct tail_node;
        tail_node.cnt = tail_cnt;
        memcpy(tail_node.data, ML_Frame->lstm_input->tail->LSTM_input, sizeof(short) * 6);
        int cnt_tracer = ML_Frame -> prev_cnt + 1;
        while (cnt_tracer < input.cnt) {
            cal_data_struct new_input = weighted_avg(tail_node, input, cnt_tracer);
            add_to_tail(ML_Frame -> lstm_input, new_input.data);
            ML_Frame -> prev_cnt = cnt_tracer;
            cnt_tracer += 1;
        }
    }
    
    add_to_tail(ML_Frame -> lstm_input, input.data);
    ML_Frame -> prev_cnt = input.cnt;



    int list_len_upperbound;
    if (ML_Frame -> hyperparameter -> USE_DIFF) {
        list_len_upperbound = ML_Frame -> hyperparameter -> LSTM_INPUT_LEN + 1;
    } else {
        list_len_upperbound = ML_Frame -> hyperparameter -> LSTM_INPUT_LEN;
    }

    if (get_list_size(ML_Frame -> lstm_input) >= list_len_upperbound) {
        while (get_list_size(ML_Frame -> lstm_input)  > list_len_upperbound) {
            pop_from_head(ML_Frame -> lstm_input);
        }
        gsl_matrix_float* input_vec = create_input(ML_Frame -> lstm_input, ML_Frame -> hyperparameter);
        //print_matrix_float(input_vec);
        gsl_matrix_float* pred_res = lstm_forward_float(ML_Frame->LSTM_float, input_vec);
        int label = generate_label(pred_res);
        //printf("raw label: %d; ", label);
        add_pred(ML_Frame ->pred_history, label);
        
        //printf(" the history length we care about is %d ", ML_Frame -> hyperparameter -> HISTORY_LEN);
        while(get_history_len(ML_Frame -> pred_history) > ML_Frame -> hyperparameter -> HISTORY_LEN) {
            pop_pred(ML_Frame -> pred_history); 
        }
        int majority_label = search_majority(ML_Frame -> pred_history);
        //printf("majority label: %d ", majority_label);
        int display_label = process_new_prediction(ML_Frame, majority_label);
        //printf("inertial label: %d ", display_label);
        int res = display_label;
        gsl_matrix_free(input_vec);
        gsl_matrix_free(pred_res);
        update_sleep_mode(ML_Frame);
        if (ML_Frame -> sleep_mode) {
            int res = sleep_actions(ML_Frame);
        }
        //printf("display label: %d\n", display_label);
        return res;
    } else {
        printf("insufficient input length\n");
        return INSUFFICIENTDATA;
    }
}


void free_service(LSTM_Service_T* ML_Frame) {
    for (int row = 0; row < ML_Frame -> hyperparameter -> LSTM_OUTPUT_DIM; row ++) {
        free(ML_Frame -> inertial_matrix[row]);
    }
    free(ML_Frame -> inertial_matrix);
    free_history_list(ML_Frame->pred_history);
    free_lstm_float(ML_Frame -> LSTM_float);
    free_list(ML_Frame -> lstm_input);
    free_hyperparameters(ML_Frame -> hyperparameter);
    free(ML_Frame);
}