#include "../include/utils.h"
#include "../include/communication.h"
#include <assert.h>

// Function to generate a random short integer between min and max (inclusive)
static short generateRandomShort(short min, short max) {
    return (short)(min + rand() % (max - min + 1));
}

int move_file(char* src, char* dest) {
    char move_command[BUFFLEN];
    strcpy(move_command, "mv ");
    strcat(move_command, src);
    strcat(move_command, " ");
    strcat(move_command, dest);
    int status = system(move_command);
    return status;
}

int download_file(char* link, char* dest) {
    char download_command[BUFFLEN];
    strcpy(download_command, "wget -P ");
    strcat(download_command, dest);
    strcat(download_command, " ");
    strcat(download_command, link);
    int status = system(download_command);
    return status;
}

int unzip_to_dest(char* source, char* dest) {
    char unzip_command[BUFFLEN];
    sprintf(unzip_command, "unzip %s -d %s", source, dest);
    int status = system(unzip_command);
    return status;
}

int remove_file(char* file_path) {
    char delete_download_file_command[BUFFLEN];
    strcpy(delete_download_file_command, "rm ");
    strcat(delete_download_file_command, file_path);
    int status = system(delete_download_file_command);
    return status;
}

int remove_dir(char* src_dir) {
    char delete_download_dir_command[BUFFLEN];
    strcpy(delete_download_dir_command, "rm -rf ");
    strcat(delete_download_dir_command, src_dir);
    int status = system(delete_download_dir_command);
    return status;
}

void saveToDATFile(char* filename, Hyperparameter_t* hyperparameters) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fwrite(hyperparameters, sizeof(Hyperparameter_t), 1, file);

    fclose(file);
}



// Function to read struct from file
void readFromDATFile(char* filename, Hyperparameter_t* product) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fread(product, sizeof(Hyperparameter_t), 1, file);

    fclose(file);
}


int** readMatrixFromFile (char* filename, int ROWS, int COLS) {
    int Matrix[ROWS][COLS];
    int** result = calloc(ROWS, sizeof(int*));
    for (int row = 0; row < ROWS; row++) {
        result[row] = calloc(COLS, sizeof(int));
    }
    FILE *file = fopen(filename, "rb"); // Open the file in binary read mode
    if (file != NULL) {
        // Read the matrix data from the file
        fread(Matrix, sizeof(int), ROWS * COLS, file);
        
        fclose(file); // Close the file
        for (int row = 0; row < ROWS; row ++) {
            for (int col = 0; col < COLS; col++) {
                result[row][col] = Matrix[row][col];
            }
        }
        return result;
    } else {
        printf("Error opening file.\n");
        return NULL;
    }
}





void load_Model_From_Folder(char* src_dir, LSTM_Service_T* lstm_model) {
    char hyperparam_src [BUFFLEN];
    char model_src [BUFFLEN];
    char transitional_mat_src [BUFFLEN];

    strncpy(hyperparam_src, src_dir, strlen(src_dir)+1);
    strncpy(model_src, src_dir, strlen(src_dir)+1);
    strncpy(transitional_mat_src, src_dir, strlen(src_dir)+1);
    


    strcat(hyperparam_src, "/Hyperparameters.DAT");
    Hyperparameter_t* hyperparameters = malloc(sizeof(Hyperparameter_t));
    readFromDATFile(hyperparam_src, hyperparameters);


    strcat(model_src, "/");
    strcat(model_src, hyperparameters ->REL_MODEL_SAVE_PATH);
    strcat(transitional_mat_src, "/");
    strcat(transitional_mat_src, hyperparameters -> REL_TRANSITIONAL_SAVE_PATH);

    // printf("hyperparam_src: %s\n", hyperparam_src);
    // printf("model_src: %s\n", model_src);
    // printf("transitional_mat_src: %s\n", transitional_mat_src);

    if (lstm_model -> hyperparameter != NULL) {
        free(lstm_model -> hyperparameter);
    }
    lstm_model -> model_type = hyperparameters ->model_type;
    lstm_model -> LSTM_float = get_lstm_float(model_src, hyperparameters -> LSTM_LAYERS, hyperparameters ->LSTM_INPUT_DIM, hyperparameters -> LSTM_HIDDEN_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    lstm_model -> inertial_matrix = readMatrixFromFile(transitional_mat_src, hyperparameters->LSTM_OUTPUT_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    lstm_model -> pred_history = init_history_list(hyperparameters -> LSTM_OUTPUT_DIM, hyperparameters->HISTORY_LEN);
    lstm_model -> lstm_input = init_list(hyperparameters -> LSTM_INPUT_DIM);
    lstm_model -> hyperparameter = hyperparameters;
    lstm_model -> sleep_mode = false;
    lstm_model -> prev_cnt = -1;
    lstm_model -> continous_mode = -1;
    lstm_model -> persistent_action_counter = 0;
    lstm_model -> new_action = -1;
    lstm_model -> new_action_counter = 0;
    
}



static int list_filename_to_dest(char* src, char* dest_file) {
    char ls_command[BUFFLEN];
    strcpy(ls_command, "ls ");
    strcat(ls_command, src);
    strcat(ls_command, " ");
    strcat(ls_command, ">");
    strcat(ls_command, " ");
    strcat(ls_command, dest_file);
    int status = system(ls_command);
    return status;
}



static int readLine (char* file_path, char* line){
    FILE *file;
    // Open the file
    file = fopen(file_path, "r");
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

static void remove_extension(char *filename) {
    // Find the last occurrence of '.' in the filename
    char *dot = strrchr(filename, '.');

    // Check if dot is found and if it is followed by "zip"
    if (dot != NULL && strcmp(dot, ".zip") == 0) {
        // Truncate the string at the position of the dot
        *dot = '\0';
    }
}


int load_Model_Service(char* download_link, LSTM_Service_T* lstm_service) {
    download_file(download_link, "download");
    char* zip_file_txt = "model_zip_path.txt";
    char zipped_file_path[BUFFLEN];
    strcpy(zipped_file_path, "download/");
    char zip_filename[BUFFLEN];

    list_filename_to_dest("download", zip_file_txt);
    int line_exists = readLine(zip_file_txt, zip_filename);
    if (line_exists < 0) {
        perror("download failure");
        return -1;
    } 

    //unzip the file to a folder
    char* unzip_path = "model";
    strcat(zipped_file_path, zip_filename);
    unzip_to_dest(zipped_file_path, unzip_path);

    char unzipped_folder[BUFFLEN];
    strcpy(unzipped_folder, unzip_path);
    strcat(unzipped_folder, "/");
    strcat(unzipped_folder, zip_filename);
    remove_extension(unzipped_folder);
    printf("unzipped folder is in %s\n",unzipped_folder);
    load_Model_From_Folder(unzipped_folder, lstm_service);

    remove_file(zip_file_txt);
    remove_dir("download");
    remove_dir(unzipped_folder);
    remove_dir("model");
    return 0;
}


static int** load_Matrix_from_SingleFile(FILE* src_file, int ROWS, int COLS) {
    int Matrix[ROWS][COLS];
    int** result = calloc(ROWS, sizeof(int*));
    for (int row = 0; row < ROWS; row++) {
        result[row] = calloc(COLS, sizeof(int));
    }
    
    fread(Matrix, sizeof(int), ROWS * COLS, src_file);
    for (int row = 0; row < ROWS; row ++) {
        for (int col = 0; col < COLS; col++) {
            result[row][col] = Matrix[row][col];
        }
    }
    return result;
}

static void save_Trans_Matrix_to_SingleFile(FILE* dest_file, int ROWS, int COLS, int** Matrix) {
    int* tmp = (int*) malloc(ROWS * COLS * sizeof(int));
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            tmp[i * COLS +j] = Matrix[i][j];
        }
    }
    fwrite(tmp, sizeof(int), ROWS * COLS, dest_file);
}



BIN_FILE_RESULTS bin_file_complete(const char* filename) {
    if (access(filename, F_OK) == -1) {
        //perror(sprintf("FILE %s Not Found\n", filename));
        return FILE_NOT_FOUND;
    }
    FILE* src_file = fopen(filename, "rb");
    if (fseek(src_file, 0, SEEK_SET) != 0) {
        perror("Error resetting file pointer to the start of header");
        return FILEREADER_FAILURE;
    } else {
        BIN_FILE_HEADER* bin_file_header = malloc(sizeof(BIN_FILE_HEADER));
        BIN_FILE_FOOTER* bin_file_footer = malloc(sizeof(BIN_FILE_FOOTER));
        fread(bin_file_header, sizeof(BIN_FILE_HEADER), 1, src_file);
        if (bin_file_header -> STARTIO != FILESTARTSYMBOL) {
            printf("start of the file is incorrect\n");
            fclose(src_file);
            free(bin_file_header);
            free(bin_file_footer);
            return FILE_INCOMPLETE;
        }

        if (bin_file_header -> hyperparameter_offset != sizeof(BIN_FILE_HEADER)) {
            printf("binary file header hyperparam parameter offset does not match the bin header size\n");
            fclose(src_file);
            free(bin_file_header);
            free(bin_file_footer);
            return FILE_OFFSET_ERROR;
        }

        if (bin_file_header -> transitional_matrix_offset != sizeof(BIN_FILE_HEADER) + sizeof(Hyperparameter_t)) {
            printf("binary file matrix parameter offset does not match the bin header size plus the hyperparam offset\n");
            fclose(src_file);
            free(bin_file_header);
            free(bin_file_footer);
            return FILE_OFFSET_ERROR;
        }

        long hyperparameter_offset = bin_file_header -> hyperparameter_offset;
        long model_parameter_offset = bin_file_header -> model_parameter_offset;
        long transitional_matrix_offset = bin_file_header -> transitional_matrix_offset;
        long footer_offset = bin_file_header -> filesize;
        //printf("current bin file footer location: %ld\n", footer_offset);
    
        if (fseek(src_file, footer_offset, SEEK_SET) != 0) {
            perror("Error resetting file pointer to the start of footer");
            fclose(src_file);
            free(bin_file_header);
            free(bin_file_footer);
            return FILEREADER_FAILURE;
        }
        fread(bin_file_footer, sizeof(BIN_FILE_FOOTER), 1, src_file);
        if (bin_file_footer -> filesize != bin_file_header -> filesize) {
            printf("start and end of the file is not consistant\n");
            fclose(src_file);
            free(bin_file_header);
            free(bin_file_footer);
            return FILE_INCOMPLETE;
        }
        if (bin_file_footer -> ENDIO != FILEENDSYMBOL) {
            printf("end of the file is incorrect\n");
            fclose(src_file);
            free(bin_file_header);
            free(bin_file_footer);
            return FILE_INCOMPLETE;
        }
        free(bin_file_header);
        free(bin_file_footer);
    }
    fclose(src_file);
    return FILE_READING_VALID;

}


BIN_FILE_RESULTS read_param_single_file(char* bin_filename, LSTM_Service_T* lstm_service) {
    if (lstm_service -> hyperparameter != NULL) {
        free(lstm_service -> hyperparameter);
    }
    FILE* src_file = fopen(bin_filename, "rb");
    BIN_FILE_RESULTS initial_check = bin_file_complete(bin_filename);
    if (initial_check != FILE_READING_VALID) {
        return initial_check;
    } 

    BIN_FILE_HEADER* bin_file_header = malloc(sizeof(BIN_FILE_HEADER));
    BIN_FILE_FOOTER* bin_file_footer = malloc(sizeof(BIN_FILE_FOOTER));
    Hyperparameter_t* hyperparameters = malloc(sizeof(Hyperparameter_t));
    
    fread(bin_file_header, sizeof(BIN_FILE_HEADER), 1, src_file);
    long hyperparameter_offset = ftell(src_file);
    fread(hyperparameters, sizeof(Hyperparameter_t), 1, src_file);
    //printf("reducing everything 2\n");
    
    long trans_mat_offset = ftell(src_file);
    //printf("matrix size: %d x %d", hyperparameters->LSTM_OUTPUT_DIM, hyperparameters->LSTM_OUTPUT_DIM);
    int** TransMat = load_Matrix_from_SingleFile(src_file, hyperparameters->LSTM_OUTPUT_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    
    //printf("reducing everything 3\n");
    long lstm_offset = ftell(src_file);
    LSTM_float* lstm = load_from_single_file(src_file, hyperparameters -> LSTM_LAYERS, hyperparameters ->LSTM_INPUT_DIM, hyperparameters -> LSTM_HIDDEN_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    
    long footer_offset = ftell(src_file);
    fread(bin_file_footer, sizeof(BIN_FILE_FOOTER), 1, src_file);

    if (footer_offset != bin_file_header ->filesize) {
        fclose(src_file);
        free(bin_file_header);
        free(bin_file_footer);
        free(hyperparameters);
        return FILE_OFFSET_ERROR;
    }

    lstm_service -> model_type = hyperparameters ->model_type;
    lstm_service -> inertial_matrix = TransMat;
    lstm_service -> LSTM_float = lstm;
    lstm_service -> pred_history = init_history_list(hyperparameters -> LSTM_OUTPUT_DIM, hyperparameters->HISTORY_LEN);
    lstm_service -> lstm_input = init_list(hyperparameters -> LSTM_INPUT_DIM);
    lstm_service -> hyperparameter = hyperparameters;
    lstm_service -> sleep_mode = false;
    lstm_service -> prev_cnt = -1;
    lstm_service -> continous_mode = -1;
    lstm_service -> persistent_action_counter = 0;
    lstm_service -> new_action = -1;
    lstm_service -> new_action_counter = 0;
    fclose(src_file);
    //free(src_file);
    free(bin_file_header);
    free(bin_file_footer);
    return FILE_READING_VALID;
}


static LSTM_Service_T* load_Model_from_SingleFile(char* src_filename, int max_iters) {
    if (access(src_filename, F_OK) != -1) {
        int iter_counter = 0;
        while (bin_file_complete(src_filename) != FILE_READING_VALID) {
            printf("current status %d\n", bin_file_complete(src_filename));
            sleep(1);
            iter_counter += 1;
            if (iter_counter > max_iters) {
                return NULL;
            }
        }
        LSTM_Service_T* lstm_service = malloc(sizeof(LSTM_Service_T));
        lstm_service -> hyperparameter = NULL;
        if (read_param_single_file(src_filename, lstm_service) != FILE_READING_VALID) {
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


void save_lstm_service_to_SingleFile(char* dest_file, LSTM_Service_T* lstm_service) {
    FILE* file = fopen(dest_file, "wb");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    long hyperparam_offset = sizeof(BIN_FILE_HEADER);
    long trans_mat_offset = sizeof(BIN_FILE_HEADER) + sizeof(Hyperparameter_t);
    long lstm_offset = trans_mat_offset + (sizeof(float) * (lstm_service -> hyperparameter -> LSTM_OUTPUT_DIM * lstm_service -> hyperparameter -> LSTM_OUTPUT_DIM));

    BIN_FILE_HEADER* header = malloc(sizeof(BIN_FILE_HEADER));
    header -> STARTIO = FILESTARTSYMBOL;
    header -> hyperparameter_offset = hyperparam_offset;
    header -> transitional_matrix_offset = trans_mat_offset;
    header -> model_parameter_offset = lstm_offset;

    BIN_FILE_FOOTER* footer = malloc(sizeof(BIN_FILE_FOOTER));
    footer -> ENDIO = FILEENDSYMBOL;

    fwrite(header, sizeof(BIN_FILE_HEADER), 1, file);
    fwrite(lstm_service -> hyperparameter, sizeof(Hyperparameter_t), 1, file);
    save_Trans_Matrix_to_SingleFile(file, lstm_service -> hyperparameter -> LSTM_OUTPUT_DIM, lstm_service -> hyperparameter -> LSTM_OUTPUT_DIM, lstm_service -> inertial_matrix);
    store_lstm_to_single_file(lstm_service -> LSTM_float, file);

    header -> filesize = ftell(file);
    footer -> filesize = ftell(file);

    //assert(footer_offset == ftell(file));
    fwrite(footer, sizeof(BIN_FILE_FOOTER), 1, file);
    
    if (fseek(file, 0, SEEK_SET) != 0) {
        perror("Error resetting file pointer to the start of header");
    }
    fwrite(header, sizeof(BIN_FILE_HEADER), 1, file);

    fclose(file);
    free(header);
    free(footer);
}

bool compare_matrices(int** m1, int** m2, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (m1[i][j] != m2[i][j]) {
                return false;
            }
        }
    } 
    return true;
}

void print_matrix(int** M, int rows, int cols) {
    printf("start printing Matrix\n");
    for (int i = 0; i < rows; i ++) {
        printf("ROW %d : ", i);
        for (int j = 0; j < cols; j++) {
            printf("- %d -", M[i][j]);
        }
        printf("\n");
    }
}




static void check_cat_no_diff_hyperparameters(char* param_folder, char* single_bin_file, char* cat_transitional_mat_path) {
    printf("Checking no diff hyperparameters");
    Hyperparameter_t* hyperparameters;
    Hyperparameter_t ref;
    hyperparameters = &ref;
    hyperparameters -> model_type = CAT;
    hyperparameters -> USE_DIFF = false;
    hyperparameters -> HISTORY_LEN = 10;
    hyperparameters -> LSTM_INPUT_LEN = 10;
    hyperparameters -> LSTM_LAYERS = 5;
    hyperparameters  -> LSTM_INPUT_DIM = 6;
    hyperparameters -> LSTM_HIDDEN_DIM = 100;
    hyperparameters -> LSTM_OUTPUT_DIM = 10;
    hyperparameters -> SLEEP_MODE_THRESH = 100;

    char*  REL_MODEL_SAVE_PATH = "no path exists";
    char*  REL_TRANSITIONAL_SAVE_PATH = "no path exists";
    char src_folder [BUFFLEN];
    strcpy(src_folder, param_folder);
    char saved_bin [BUFFLEN];
    strcpy(saved_bin, single_bin_file);
    char transitional_src [BUFFLEN];
    strcpy(transitional_src, cat_transitional_mat_path);

    strncpy(hyperparameters -> REL_MODEL_SAVE_PATH, REL_MODEL_SAVE_PATH, strlen(REL_MODEL_SAVE_PATH)+1);
    strncpy(hyperparameters -> REL_TRANSITIONAL_SAVE_PATH, REL_TRANSITIONAL_SAVE_PATH, strlen(REL_TRANSITIONAL_SAVE_PATH)+1);

    LSTM_Service_T* lstm_service = malloc(sizeof(LSTM_Service_T));
    lstm_service -> LSTM_float = get_lstm_float(src_folder, hyperparameters -> LSTM_LAYERS , hyperparameters -> LSTM_INPUT_DIM, hyperparameters -> LSTM_HIDDEN_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    printf("model loaded\n");
    lstm_service -> inertial_matrix = readMatrixFromFile(transitional_src, hyperparameters -> LSTM_OUTPUT_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    lstm_service -> hyperparameter = hyperparameters;


    save_lstm_service_to_SingleFile(saved_bin, lstm_service);
    LSTM_Service_T* lstm_reloaded_service = load_Model_from_SingleFile(saved_bin, 100);
    bool lstm_similarity = compare_lstm_models(lstm_reloaded_service -> LSTM_float, lstm_service -> LSTM_float);
    if (lstm_similarity) {
        printf("lstm is consistent before and after loading\n");
    } else {
        printf("lstm is NOT consistent before and after loading\n");
    }
    
    printf("Cat lstm loading complete\n");
    bool transitional_Matrix_similarity = compare_matrices(lstm_reloaded_service -> inertial_matrix, lstm_service ->inertial_matrix, 10, 10);
    printf("reloaded lstm service transitional matrix\n");
    //print_matrix(lstm_reloaded_service -> inertial_matrix, 10, 10);
    printf("original lstm service transitional matrix\n");
    //print_matrix(lstm_service -> inertial_matrix, 10, 10);
    if (transitional_Matrix_similarity) {
        printf("Transitional Matrix is consistent before and after loading\n");
    } else {
        printf("Transitional Matrix is NOT consistent before and after loading\n");
    }
    Hyperparameter_t* hyperparameters_read = lstm_reloaded_service -> hyperparameter;
    assert(hyperparameters_read -> USE_DIFF == hyperparameters -> USE_DIFF);
    assert(hyperparameters_read -> model_type == hyperparameters -> model_type);
    assert(hyperparameters_read -> HISTORY_LEN == hyperparameters -> HISTORY_LEN);
    assert(hyperparameters_read -> LSTM_INPUT_LEN == hyperparameters -> LSTM_INPUT_LEN);
    assert(hyperparameters_read -> LSTM_INPUT_DIM == hyperparameters -> LSTM_INPUT_DIM);
    assert(hyperparameters_read -> LSTM_LAYERS == hyperparameters -> LSTM_LAYERS);
    assert(hyperparameters_read -> LSTM_HIDDEN_DIM  == hyperparameters -> LSTM_HIDDEN_DIM);
    assert(hyperparameters_read -> SLEEP_MODE_THRESH  == hyperparameters -> SLEEP_MODE_THRESH);
    printf("hyperparameters matched\n");
    
}

static void check_cat_diff_hyperparameters(char* param_folder, char* single_bin_file, char* cat_transitional_mat_path) {
    printf("Checking diff hyperparameters Cat\n");
    Hyperparameter_t* hyperparameters;
    Hyperparameter_t ref;
    hyperparameters = &ref;
    hyperparameters -> model_type = CAT;
    hyperparameters -> USE_DIFF = true;
    hyperparameters -> HISTORY_LEN = 5;
    hyperparameters -> LSTM_INPUT_LEN = 9;
    hyperparameters -> LSTM_LAYERS = 5;
    hyperparameters  -> LSTM_INPUT_DIM = 6;
    hyperparameters -> LSTM_HIDDEN_DIM = 100;
    hyperparameters -> LSTM_OUTPUT_DIM = 10;
    hyperparameters -> SLEEP_MODE_THRESH = 100;

    char*  REL_MODEL_SAVE_PATH = "no path exists";
    char*  REL_TRANSITIONAL_SAVE_PATH = "no path exists";
    
    char src_folder [BUFFLEN];
    strcpy(src_folder, param_folder);
    char transitional_src [BUFFLEN];
    strcpy(transitional_src, cat_transitional_mat_path);

    strncpy(hyperparameters -> REL_MODEL_SAVE_PATH, REL_MODEL_SAVE_PATH, strlen(REL_MODEL_SAVE_PATH)+1);
    strncpy(hyperparameters -> REL_TRANSITIONAL_SAVE_PATH, REL_TRANSITIONAL_SAVE_PATH, strlen(REL_TRANSITIONAL_SAVE_PATH)+1);
    LSTM_Service_T* lstm_service = malloc(sizeof(LSTM_Service_T));
    lstm_service -> LSTM_float = get_lstm_float(src_folder, hyperparameters -> LSTM_LAYERS , hyperparameters -> LSTM_INPUT_DIM, hyperparameters -> LSTM_HIDDEN_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    printf("model loaded\n");
    lstm_service -> inertial_matrix = readMatrixFromFile(transitional_src, hyperparameters -> LSTM_OUTPUT_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    lstm_service -> hyperparameter = hyperparameters;

    char saved_bin [BUFFLEN];
    strcpy(saved_bin, single_bin_file);
    save_lstm_service_to_SingleFile(saved_bin, lstm_service);
    LSTM_Service_T* lstm_reloaded_service = load_Model_from_SingleFile(saved_bin, 100);
    bool lstm_similarity = compare_lstm_models(lstm_reloaded_service -> LSTM_float, lstm_service -> LSTM_float);
    if (lstm_similarity) {
        printf("lstm is consistent before and after loading\n");
    } else {
        printf("lstm is NOT consistent before and after loading\n");
    }
    printf("Cat lstm loading complete\n");
    bool transitional_Matrix_similarity = compare_matrices(lstm_reloaded_service -> inertial_matrix, lstm_service ->inertial_matrix, 10, 10);
    printf("reloaded lstm service transitional matrix\n");
    //print_matrix(lstm_reloaded_service -> inertial_matrix, 10, 10);
    printf("original lstm service transitional matrix\n");
    //print_matrix(lstm_service -> inertial_matrix, 10, 10);
    if (transitional_Matrix_similarity) {
        printf("Transitional Matrix is consistent before and after loading\n");
    } else {
        printf("Transitional Matrix is NOT consistent before and after loading\n");
    }
    Hyperparameter_t* hyperparameters_read = lstm_reloaded_service -> hyperparameter;
    assert(hyperparameters_read -> USE_DIFF == hyperparameters -> USE_DIFF);
    assert(hyperparameters_read -> model_type == hyperparameters -> model_type);
    assert(hyperparameters_read -> HISTORY_LEN == hyperparameters -> HISTORY_LEN);
    assert(hyperparameters_read -> LSTM_INPUT_LEN == hyperparameters -> LSTM_INPUT_LEN);
    assert(hyperparameters_read -> LSTM_INPUT_DIM == hyperparameters -> LSTM_INPUT_DIM);
    assert(hyperparameters_read -> LSTM_LAYERS == hyperparameters -> LSTM_LAYERS);
    assert(hyperparameters_read -> LSTM_HIDDEN_DIM  == hyperparameters -> LSTM_HIDDEN_DIM);
    assert(hyperparameters_read -> SLEEP_MODE_THRESH  == hyperparameters -> SLEEP_MODE_THRESH);
    printf("hyperparameters matched\n");
}

static void check_dog_diff_hyperparameters(char* param_folder, char* single_bin_file, char* dog_transitional_mat_path, Model_Type_T dog_type) {
    printf("Checking diff hyperparameters Dog\n");
    Hyperparameter_t* hyperparameters;
    Hyperparameter_t ref;
    hyperparameters = &ref;
    hyperparameters -> model_type = dog_type;
    hyperparameters -> USE_DIFF = true;
    hyperparameters -> HISTORY_LEN = 10;
    hyperparameters -> LSTM_INPUT_LEN = 9;
    hyperparameters -> LSTM_LAYERS = 5;
    hyperparameters  -> LSTM_INPUT_DIM = 6;
    hyperparameters -> LSTM_HIDDEN_DIM = 100;
    hyperparameters -> LSTM_OUTPUT_DIM = 9;
    hyperparameters -> SLEEP_MODE_THRESH = 1000;

    char*  REL_MODEL_SAVE_PATH = "no path exists";
    char*  REL_TRANSITIONAL_SAVE_PATH = "no path exists";
    
    char src_folder [BUFFLEN];
    strcpy(src_folder, param_folder);
    char transitional_src [BUFFLEN];
    strcpy(transitional_src, dog_transitional_mat_path);

    strncpy(hyperparameters -> REL_MODEL_SAVE_PATH, REL_MODEL_SAVE_PATH, strlen(REL_MODEL_SAVE_PATH)+1);
    strncpy(hyperparameters -> REL_TRANSITIONAL_SAVE_PATH, REL_TRANSITIONAL_SAVE_PATH, strlen(REL_TRANSITIONAL_SAVE_PATH)+1);
    LSTM_Service_T* lstm_service = malloc(sizeof(LSTM_Service_T));
    lstm_service -> LSTM_float = get_lstm_float(src_folder, hyperparameters -> LSTM_LAYERS , hyperparameters -> LSTM_INPUT_DIM, hyperparameters -> LSTM_HIDDEN_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    printf("model loaded\n");
    lstm_service -> inertial_matrix = readMatrixFromFile(transitional_src, hyperparameters -> LSTM_OUTPUT_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    lstm_service -> hyperparameter = hyperparameters;

    char saved_bin [BUFFLEN];
    strcpy(saved_bin, single_bin_file);
    save_lstm_service_to_SingleFile(saved_bin, lstm_service);
    LSTM_Service_T* lstm_reloaded_service = load_Model_from_SingleFile(saved_bin, 100);
    bool lstm_similarity = compare_lstm_models(lstm_reloaded_service -> LSTM_float, lstm_service -> LSTM_float);
    if (lstm_similarity) {
        printf("lstm is consistent before and after loading\n");
    } else {
        printf("lstm is NOT consistent before and after loading\n");
    }
    printf("Dog lstm loading complete\n");

    bool transitional_Matrix_similarity = compare_matrices(lstm_reloaded_service -> inertial_matrix, lstm_service ->inertial_matrix, hyperparameters -> LSTM_OUTPUT_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    printf("reloaded lstm service transitional matrix\n");
    //print_matrix(lstm_reloaded_service -> inertial_matrix, hyperparameters -> LSTM_OUTPUT_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    //printf("original lstm service transitional matrix\n");
    //print_matrix(lstm_service -> inertial_matrix, hyperparameters -> LSTM_OUTPUT_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    if (transitional_Matrix_similarity) {
        printf("Transitional Matrix is consistent before and after loading\n");
    } else {
        printf("Transitional Matrix is NOT consistent before and after loading\n");
    }
    Hyperparameter_t* hyperparameters_read = lstm_reloaded_service -> hyperparameter;
    assert(hyperparameters_read -> USE_DIFF == hyperparameters -> USE_DIFF);
    assert(hyperparameters_read -> model_type == hyperparameters -> model_type);
    assert(hyperparameters_read -> HISTORY_LEN == hyperparameters -> HISTORY_LEN);
    assert(hyperparameters_read -> LSTM_INPUT_LEN == hyperparameters -> LSTM_INPUT_LEN);
    assert(hyperparameters_read -> LSTM_INPUT_DIM == hyperparameters -> LSTM_INPUT_DIM);
    assert(hyperparameters_read -> LSTM_LAYERS == hyperparameters -> LSTM_LAYERS);
    assert(hyperparameters_read -> LSTM_HIDDEN_DIM  == hyperparameters -> LSTM_HIDDEN_DIM);
    assert(hyperparameters_read -> SLEEP_MODE_THRESH  == hyperparameters -> SLEEP_MODE_THRESH);
    printf("hyperparameters matched\n");
}


static void check_dog_no_diff_hyperparameters(char* param_folder, char* single_bin_file, char* dog_transitional_mat_path, Model_Type_T dog_type) {
    printf("Checking No diff hyperparameters Dog\n");
    Hyperparameter_t* hyperparameters;
    Hyperparameter_t ref;
    hyperparameters = &ref;
    hyperparameters -> model_type = dog_type;
    hyperparameters -> USE_DIFF = false;
    hyperparameters -> HISTORY_LEN = 10;
    hyperparameters -> LSTM_INPUT_LEN = 10;
    hyperparameters -> LSTM_LAYERS = 5;
    hyperparameters  -> LSTM_INPUT_DIM = 6;
    hyperparameters -> LSTM_HIDDEN_DIM = 100;
    hyperparameters -> LSTM_OUTPUT_DIM = 9;
    hyperparameters -> SLEEP_MODE_THRESH = 1000;

    char*  REL_MODEL_SAVE_PATH = "no path exists";
    char*  REL_TRANSITIONAL_SAVE_PATH = "no path exists";
    
    char src_folder [BUFFLEN];
    strcpy(src_folder, param_folder);
    char transitional_src [BUFFLEN];
    strcpy(transitional_src, dog_transitional_mat_path);

    strncpy(hyperparameters -> REL_MODEL_SAVE_PATH, REL_MODEL_SAVE_PATH, strlen(REL_MODEL_SAVE_PATH)+1);
    strncpy(hyperparameters -> REL_TRANSITIONAL_SAVE_PATH, REL_TRANSITIONAL_SAVE_PATH, strlen(REL_TRANSITIONAL_SAVE_PATH)+1);
    LSTM_Service_T* lstm_service = malloc(sizeof(LSTM_Service_T));
    lstm_service -> LSTM_float = get_lstm_float(src_folder, hyperparameters -> LSTM_LAYERS , hyperparameters -> LSTM_INPUT_DIM, hyperparameters -> LSTM_HIDDEN_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    printf("model loaded\n");
    lstm_service -> inertial_matrix = readMatrixFromFile(transitional_src, hyperparameters -> LSTM_OUTPUT_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    lstm_service -> hyperparameter = hyperparameters;

    char saved_bin [BUFFLEN];
    strcpy(saved_bin, single_bin_file);
    save_lstm_service_to_SingleFile(saved_bin, lstm_service);
    LSTM_Service_T* lstm_reloaded_service = load_Model_from_SingleFile(saved_bin, 100);
    bool lstm_similarity = compare_lstm_models(lstm_reloaded_service -> LSTM_float, lstm_service -> LSTM_float);
    if (lstm_similarity) {
        printf("lstm is consistent before and after loading\n");
    } else {
        printf("lstm is NOT consistent before and after loading\n");
    }
    printf("Dog lstm loading complete\n");

    bool transitional_Matrix_similarity = compare_matrices(lstm_reloaded_service -> inertial_matrix, lstm_service ->inertial_matrix, hyperparameters -> LSTM_OUTPUT_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    printf("reloaded lstm service transitional matrix\n");
    // print_matrix(lstm_reloaded_service -> inertial_matrix, hyperparameters -> LSTM_OUTPUT_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    // printf("original lstm service transitional matrix\n");
    // print_matrix(lstm_service -> inertial_matrix, hyperparameters -> LSTM_OUTPUT_DIM, hyperparameters -> LSTM_OUTPUT_DIM);
    if (transitional_Matrix_similarity) {
        printf("Transitional Matrix is consistent before and after loading\n");
    } else {
        printf("Transitional Matrix is NOT consistent before and after loading\n");
    }
    Hyperparameter_t* hyperparameters_read = lstm_reloaded_service -> hyperparameter;
    assert(hyperparameters_read -> USE_DIFF == hyperparameters -> USE_DIFF);
    assert(hyperparameters_read -> model_type == hyperparameters -> model_type);
    assert(hyperparameters_read -> HISTORY_LEN == hyperparameters -> HISTORY_LEN);
    assert(hyperparameters_read -> LSTM_INPUT_LEN == hyperparameters -> LSTM_INPUT_LEN);
    assert(hyperparameters_read -> LSTM_INPUT_DIM == hyperparameters -> LSTM_INPUT_DIM);
    assert(hyperparameters_read -> LSTM_LAYERS == hyperparameters -> LSTM_LAYERS);
    assert(hyperparameters_read -> LSTM_HIDDEN_DIM  == hyperparameters -> LSTM_HIDDEN_DIM);
    assert(hyperparameters_read -> SLEEP_MODE_THRESH  == hyperparameters -> SLEEP_MODE_THRESH);
    printf("hyperparameters matched\n");
}




cal_data_struct* load_input_seq_from_float_bin(char* src_file, int len, int dim) {
    cal_data_struct* res = calloc(len, sizeof(cal_data_struct));
    float Matrix[len][dim];
    FILE *file = fopen(src_file, "rb");
    fread(Matrix, sizeof(float), len * dim, file);
    for (int i = 0; i < len; i ++) {
        for (int j = 0; j < dim; j++) {
            res[i].cnt = i + 30;
            res[i].data[j] = (short)Matrix[i][j];
        }
    }
    fclose(file);
    return res;
}



//testing util functions using labeled data


cal_data_struct extract_input_seq_from_labeled_data(cal_data_struct_test input_with_label) {
    cal_data_struct output; 
    for (int i = 0; i < 6; i++) {
        output.data[i] = input_with_label.data[i];
    }
    return output;
}

int extract_cnt_from_labeled_data(cal_data_struct_test input_with_label) {
    return input_with_label.cnt;
}


int extract_label_from_labeled_data(cal_data_struct_test input_with_label) {
    return input_with_label.label;
}


cal_data_struct_test* load_test_input_seq_from_float_bin(char* src_file, int len, int dim) {
    cal_data_struct_test* res = calloc(len, sizeof(cal_data_struct_test));
    int matrix_width = dim + 2;
    float Matrix[len][matrix_width];
    FILE *file = fopen(src_file, "rb");
    fread(Matrix, sizeof(float), len * matrix_width, file);
    for (int i = 0; i < len; i ++) {
        res[i].cnt = Matrix[i][0];
        for (int j = 0; j < dim; j++) {
            res[i].data[j] = (short)Matrix[i][j + 1];
        }
        res[i].label = (int)(Matrix[i][dim + 1]);
    }
    fclose(file);
    return res;
}





void save_lstm_input_to_float_bin(char* dest_file, cal_data_struct* input, int len, int dim) {
    float tmp_Matrix[len][dim];

    for (int i = 0; i < len; i++) {
        for (int j = 0; j < dim; j++) {
            tmp_Matrix[i][j] = (float)input[i].data[j];
        }
    }
    FILE *file = fopen(dest_file, "wb"); // Open the file in binary write mode
    if (file != NULL) {
        // Write the matrix to the file
        fwrite(tmp_Matrix, sizeof(float), len * dim, file);
        
        fclose(file); // Close the file
    } else {
        printf("Error opening file.\n");
    }
}

static void print_input(cal_data_struct* input, int len, int dim) {
    for (int i = 0; i < len; i ++) {
        printf("ROW %d: ", i);
        for (int j = 0; j < dim; j ++) {
            printf(" %d, ", input[i].data[j]);
        }
        printf("\n");
    } 
}


static void check_save_and_load_input(char* dest, int len) {
    char saved_file [BUFFLEN];
    strcpy(saved_file, dest);
    int dim = 6;
    cal_data_struct* input = calloc(len, sizeof(cal_data_struct));
    short short_max = 0x7fff;
    short short_min = 0xffff;
    for (int i = 0; i < len; i ++) {
        for (int j = 0; j < dim; j++) {
            input[i].data[j] = (short)rand();
            input[i].cnt = i;
        }
    }
    //printf("initialized input is:\n ");
    //print_input(input, len, dim);
    
    save_lstm_input_to_float_bin(saved_file, input, len, dim);
    cal_data_struct* loaded_input = load_input_seq_from_float_bin(saved_file, len, dim);
    
    //printf("loaded input is:\n ");
    //print_input(loaded_input, len, dim);
    
    for (int i = 0; i < len; i ++) {
        for (int j = 0; j < dim; j ++) {
            assert(input[i].data[j] == loaded_input[i].data[j]);
        }
    }
}





static void free_hyperparameters(Hyperparameter_t* hyperparameters) {
    free(hyperparameters);
}

// static void free_service(LSTM_Service_T* ML_Frame) {
//     if (ML_Frame == NULL) {
//         return;
//     }
//     for (int row = 0; row < ML_Frame -> hyperparameter -> LSTM_OUTPUT_DIM; row++) {
//         free(ML_Frame -> inertial_matrix[row]);
//     }
//     free(ML_Frame -> inertial_matrix);
//     if (ML_Frame -> pred_history != NULL) {
//         free_history_list(ML_Frame->pred_history);
//     }
//     if (ML_Frame -> LSTM_float != NULL) {
//         free_lstm_float(ML_Frame -> LSTM_float);
//     }
//     if (ML_Frame -> lstm_input != NULL) {
//         free_list(ML_Frame -> lstm_input);
//     }
//     if (ML_Frame -> hyperparameter != NULL) {
//         free_hyperparameters(ML_Frame -> hyperparameter);
//     }
//     free(ML_Frame);
// }


static void test_loading_model_from_local() {
    char* single_file_src = "download/CAT_DIFF_5.bin";
    LSTM_Service_T* ML_RealTime = malloc(sizeof(LSTM_Service_T));
    int Res_real = read_param_single_file(single_file_src, ML_RealTime);
    free_service(ML_RealTime);
}






// int main() {
//     check_cat_diff_hyperparameters("params/cat_lstm_diff_float", "params/SINGLEBINS/CAT_DIFF.bin", "transitional_mat/cat_diff_transitional_mat_new.bin");
//     // check_cat_no_diff_hyperparameters("params/cat_lstm_no_diff_float", "params/SINGLEBINS/CAT_NO_DIFF.bin", "transitional_mat/cat_diff_transitional_mat.bin");
//     // check_dog_diff_hyperparameters("params/dog_midlarge_lstm_diff_float", "params/SINGLEBINS/DOG_MIDLARGE_DIFF.bin", "transitional_mat/dog_no_diff_transitional_mat.bin", DOG_LARGE);
//     check_dog_no_diff_hyperparameters("params/dog_midlarge_lstm_no_diff_float", "params/SINGLEBINS/DOG_MIDLARGE_NO_DIFF.bin", "transitional_mat/dog_no_diff_transitional_mat.bin", DOG_LARGE);
//     // check_dog_diff_hyperparameters("params/dog_small_lstm_diff_float", "params/SINGLEBINS/DOG_SMALL_DIFF.bin", "transitional_mat/dog_no_diff_transitional_mat.bin", DOG_SMALL);
//     check_dog_no_diff_hyperparameters("params/dog_small_lstm_no_diff_float", "params/SINGLEBINS/DOG_SMALL_NO_DIFF.bin", "transitional_mat/dog_no_diff_transitional_mat.bin", DOG_SMALL);
//     //check_save_and_load_input("test_data/short_example_2000.bin", 2000);
//     //return 0;
// }

// int main() {
//     Hyperparameter_t* hyperparameters;
//     Hyperparameter_t ref;
//     hyperparameters = &ref;
//     hyperparameters -> model_type = CAT;
//     hyperparameters -> HISTORY_LEN = 10;
//     hyperparameters -> LSTM_INPUT_LEN = 10;
//     hyperparameters -> LSTM_LAYERS = 5;
//     hyperparameters  -> LSTM_INPUT_DIM = 6;
//     hyperparameters -> LSTM_HIDDEN_DIM = 100;
//     hyperparameters -> LSTM_OUTPUT_DIM = 10;
//     hyperparameters -> SLEEP_MODE_THRESH = 100;
//     char*  REL_MODEL_SAVE_PATH = "model_bin_float";
//     char* REL_TRANSITIONAL_SAVE_PATH = "transitional_mat.bin";
//     strncpy(hyperparameters -> REL_MODEL_SAVE_PATH, REL_MODEL_SAVE_PATH, strlen(REL_MODEL_SAVE_PATH)+1);
//     strncpy(hyperparameters -> REL_TRANSITIONAL_SAVE_PATH, REL_TRANSITIONAL_SAVE_PATH, strlen(REL_TRANSITIONAL_SAVE_PATH)+1);

//     char* save_hyper_file = "Hyperparameters.DAT";
//     saveToDATFile(save_hyper_file, hyperparameters);

//     Hyperparameter_t* hyperparameters_read = malloc(sizeof(Hyperparameter_t));
//     readFromDATFile(save_hyper_file, hyperparameters_read);
//     assert(hyperparameters_read -> model_type == hyperparameters -> model_type);
//     assert(hyperparameters_read -> HISTORY_LEN == hyperparameters -> HISTORY_LEN);
//     assert(hyperparameters_read -> LSTM_INPUT_LEN == hyperparameters -> LSTM_INPUT_LEN);
//     assert(hyperparameters_read -> LSTM_INPUT_DIM == hyperparameters -> LSTM_INPUT_DIM);
//     assert(hyperparameters_read -> LSTM_LAYERS == hyperparameters -> LSTM_LAYERS);
//     assert(hyperparameters_read -> LSTM_HIDDEN_DIM  == hyperparameters -> LSTM_HIDDEN_DIM);
//     assert(hyperparameters_read -> SLEEP_MODE_THRESH  == hyperparameters -> SLEEP_MODE_THRESH);
//     //printf("load path: %s, real_path: %s ", hyperparameters_read -> REL_MODEL_SAVE_PATH, hyperparameters->REL_MODEL_SAVE_PATH);
//     //printf("load path: %s, real_path: %s ", hyperparameters_read -> REL_TRANSITIONAL_SAVE_PATH, hyperparameters->REL_TRANSITIONAL_SAVE_PATH);
//     assert(strcmp(hyperparameters_read -> REL_MODEL_SAVE_PATH, hyperparameters->REL_MODEL_SAVE_PATH) == 0);
//     assert(strcmp(hyperparameters_read -> REL_TRANSITIONAL_SAVE_PATH, hyperparameters -> REL_TRANSITIONAL_SAVE_PATH) == 0);

//     free(hyperparameters_read);
//     remove_file(save_hyper_file);

//     LSTM_Service_T* lstm_service = malloc(sizeof(LSTM_Service_T));
//     lstm_service -> hyperparameter = NULL;


//     char* example_download_link = "https://md.ccyz.cc/cat_package_example.zip";

//     // download_file(example_download_link, "download");
//     // char* zip_file_txt = "model_zip_path.txt";
//     // char zipped_file_path[BUFFLEN];
//     // strcpy(zipped_file_path, "download/");
//     // char zip_filename[BUFFLEN];

//     // list_filename_to_dest("download", zip_file_txt);
//     // int line_exists = readLine(zip_file_txt, zip_filename);
//     // if (line_exists < 0) {
//     //     perror("download failure");
//     //     return -1;
//     // } 

    
//     // //unzip the file to a folder
//     // char* unzip_path = "model";
//     // strcat(zipped_file_path, zip_filename);
//     // unzip_to_dest(zipped_file_path, unzip_path);

//     // char unzipped_folder[BUFFLEN];
//     // strcpy(unzipped_folder, unzip_path);
//     // strcat(unzipped_folder, "/");
//     // strcat(unzipped_folder, zip_filename);
//     // remove_extension(unzipped_folder);
//     // printf("unzipped folder is in %s\n",unzipped_folder);
//     // load_Model_From_Folder(unzipped_folder, lstm_service);

//     // remove_file(zip_file_txt);
//     // remove_dir("download");
//     // remove_dir(unzipped_folder);
//     load_Model_Service(example_download_link, lstm_service);

//     //load_Model_From_Folder("cat_package_example", lstm_service);
//     printf("sanity check passed\n");
//     return 0;
// }
