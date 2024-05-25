#ifndef LINEAR_ALG_H
#define LINEAR_ALG_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "globals.h"
#include <unistd.h>
#include "../include/communication.h"



int read_link_from_file (char* filename, char* line);
void remove_all_downloaded_files();
bool compare_download_link(char* new_download, LATEST_INIT* latest_download, pthread_mutex_t* download_history_mutex_addr);


#endif