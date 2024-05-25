#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 

typedef struct HistoryNode {
    int val;
    struct HistoryNode* prev;
    struct HistoryNode* next;
} HistoryNode;

typedef struct HistoryList {
    int max_history_len;
    int history_len;
    int total_labels;
    int* label_count;
    HistoryNode* head;
    HistoryNode* tail;
} HistoryList;

HistoryList* init_history_list(int label_num, int max_history_len);
int get_history_len(HistoryList* list);
void add_pred(HistoryList* list, int elem);
int pop_pred(HistoryList* list);
void clear_history(HistoryList* list);
void free_history_list(HistoryList* list);
void print_history(HistoryList* list);
bool check_history_correctness(HistoryList* list);


//functional methods for label selection
int search_majority(HistoryList* list);


