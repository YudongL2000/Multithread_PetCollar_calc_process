#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/download.h"
#include "stdbool.h"


int read_link_from_file (char* filename, char* line){
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



void remove_all_downloaded_files() {
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



bool compare_download_link(char* new_download, LATEST_INIT* latest_download, pthread_mutex_t* download_history_mutex_addr) {
    char past_download[BUFFLEN];
    pthread_mutex_lock(download_history_mutex_addr);
    strcpy(past_download, latest_download -> latest_link);
    pthread_mutex_unlock(download_history_mutex_addr);
    if (strcmp(past_download, new_download)==0) {
        return true;
    } else {
        return false;
    }
}