#ifndef LSTM_INPUT_H
#define LSTM_INPUT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 


typedef struct ShortNode {
    short* LSTM_input;
    struct ShortNode* prev;
    struct ShortNode* next;
} ShortNode;

typedef struct DataList {
    int dim;
    int size;
    ShortNode* head;
    ShortNode* tail;
} DataList;



DataList* init_list(int dim);
int get_list_size(DataList* list);
void add_to_head(DataList* list, short* elem);
void add_to_tail(DataList* list, short* elem);
void pop_from_head(DataList* list);
void pop_from_tail(DataList* list);
void clear_list(DataList* list);
void free_list(DataList* list);
void print_list(DataList* list);
bool check_correctness(DataList* list);

#endif