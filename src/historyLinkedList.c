#include "../include/historyLinkedList.h"
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 


HistoryList* init_history_list(int label_num, int max_history_len) {
    HistoryList* history_list = malloc(sizeof(HistoryList));
    history_list -> head = NULL ;
    history_list -> tail = NULL;
    history_list -> max_history_len = max_history_len;
    history_list -> total_labels = label_num;
    history_list -> label_count = calloc(label_num, sizeof(int));
    history_list -> history_len = 0;
    for (int j = 0; j < label_num; j++) {
        history_list -> label_count[j] = 0;
    }
    return history_list;
}

int get_history_len(HistoryList* list) {
    return list -> history_len;
}



void add_pred(HistoryList* list, int elem) {
    HistoryNode* new_node = malloc(sizeof(HistoryNode));
    new_node -> val = elem;
    HistoryNode* tail_node = list->tail;
    list -> label_count[elem] += 1;
    //printf("mein fuhrer\n");
    if (tail_node != NULL) {
        new_node -> next = NULL;
        new_node -> prev = tail_node;

        tail_node -> next = new_node;
        list -> tail = new_node;
    } else {
        new_node -> prev = NULL;
        new_node -> next = NULL;
        list -> head = new_node;
        list -> tail = new_node;
    }
    list -> history_len += 1;
}

int pop_pred(HistoryList* list) {
    HistoryNode* headnode = list -> head;
    int pred_val = headnode -> val;
    if (headnode -> next == NULL) {
        list -> head = NULL;
        list -> tail = NULL;
        list -> history_len = 0;
    } else {
        HistoryNode* new_headnode = headnode -> next;
        new_headnode -> prev = NULL;
        list -> head = new_headnode;
        list -> history_len -= 1;
    }
    list -> label_count[pred_val] -= 1; 
    free(headnode);
    return pred_val;
}


void clear_history(HistoryList* list) {
    HistoryNode* iter = list -> head;
    while (iter != NULL) {
        HistoryNode* current_node = iter;
        iter = iter -> next;
        free(current_node);
    }
    list -> head = NULL;
    list -> tail = NULL;
    for (int k = 0; k < list->total_labels; k++) {
        list -> label_count[k] = 0;
    }
    list -> history_len = 0;
}



void free_history_list(HistoryList* list) {
    HistoryNode* iter = list -> head;
    while (iter != NULL) {
        HistoryNode* current_node = iter;
        int content = iter -> val;
        iter = iter -> next;
        free(current_node);
        list -> label_count[content] -= 1;
        list -> history_len -= 1;
    }
    free(list -> label_count);
    free(list);
}





void print_history(HistoryList* list) {
    HistoryNode* head = list -> head;
    HistoryNode* tracer = head;
    int counter = 0;
    printf("size: %d\n", list->history_len);
    if (head != NULL && head -> prev == NULL) {
        printf("NULL");
    }
    while (tracer != NULL) {
        counter += 1;
        int cell_value = tracer -> val;
        if (counter == 1 && tracer->next != NULL) {
            printf("->CELL: %d(HEAD)", cell_value);
        } else if (counter >1 && tracer -> next == NULL) {
            printf("->CELL: %d(Tail)->NULL", cell_value);
        } else if (counter == 1 && tracer->next == NULL) {
            printf("->CELL: %d (HEAD/TAIL)->NULL", cell_value);
        } else {
            printf("->CELL: %d ", cell_value);
        }
        tracer = tracer -> next;
        if (counter > list -> history_len) {
            break;
        }
    }
    if (counter == 0) {
        printf("(HEAD)NULL(TAIL)");
    }
    printf("\n");
}



bool check_history_correctness(HistoryList* list) {
    int counter = 0;
    HistoryNode* iter = list -> head;
    if(iter == NULL) {
        if (list -> tail != NULL) {
            printf("list tail should have null pointer when list head have null pointer\n");
            return false;
        } else if (list -> history_len != 0) {
            printf ("list size should be 0 when it's empty\n");
            return false;
        }
        return true;
    }
    if (iter -> prev != NULL) {
        printf("list head should have null prev pointer\n");
        return false;
    }
    int* label_counter = calloc(list-> total_labels, sizeof(int));
    for (int k = 0; k < list -> total_labels; k++) {
        label_counter[k] = 0;
    }
    while (iter != NULL) {
        counter += 1;
        if (iter -> val < 0 || iter -> val >= list->total_labels) {
            printf("list element is invalid\n");
            break;
        }
        label_counter[iter -> val] += 1;
        if (iter -> next == NULL) {
            if (list -> tail != iter) {
                printf("list tail has invalid pointer\n");
                return false;
            }
        } else if (iter != iter -> next -> prev) {
            printf("list connection invalid\n");
            return false;
        }
        if (list -> history_len < counter) {
            printf("list size does not match with number of elements\n");
            return false;
        }
        iter = iter -> next;
    }
    if (list-> history_len != counter) {
        printf("invalid size");
        return false;
    }
    for (int w = 0; w < list -> total_labels; w++) {
        if (list -> label_count[w] != label_counter[w]) {
            printf("label counter does not match");

        }
    }
    free(label_counter);
    return true;
}

static void print_list(int* list, int list_size) {
    printf("LIST START-");
    for (int k = 0; k < list_size; k++) {
        printf("%d-", list[k]);
    }
    printf("END\n");
}

int search_majority(HistoryList* list) {
    //print_history(list);
    int label_argmax = -1;
    int label_maxcount = -1;
    for (int idx = 0; idx < list -> total_labels; idx ++) {
        if (list -> label_count[idx] > label_maxcount) {
            label_argmax = idx;
            label_maxcount = list -> label_count[idx];
        }
    }
    return label_argmax;
}


// int main() {
//     printf("start\n");
//     int max_history_len = 30;
//     int total_labels = 10;
//     //printf("initing list\n");
//     HistoryList* datalist = init_history_list(total_labels, max_history_len);
//     int total_elem = 500;

//     int* poped_element = calloc(total_elem, sizeof(int));
//     int* inserted_element = calloc(total_elem, sizeof(int));
//     int pop_counter = 0;
//     int inserted_counter = 0;
//     for (int i = 0; i < total_elem; i++) {
//         //printf("allocated payload： %d", i);
//         int new_elem = rand() % 10;
//         //print_history(datalist);
//         inserted_element[inserted_counter] = new_elem;
//         inserted_counter += 1;
//         add_pred(datalist, new_elem);
//         if (datalist -> history_len > datalist -> max_history_len) {
//             //printf("start poping\n");
//             int removed_element = pop_pred(datalist);
//             poped_element[pop_counter] = removed_element;
//             pop_counter += 1;
//         }
//         if (check_correctness(datalist) == false) {
//             print_history(datalist);
//             printf("\n");
//             printf("invalid datalist after %d iterations of element insertions", i);
//             return 1;
//         }
//     }
//     //print_list(poped_element, pop_counter);
//     //print_list(inserted_element, inserted_counter);
//     for (int n=0; n < pop_counter; n++) {
//         if (poped_element[n] != inserted_element[n]) {
//             printf("%d th poped element doesn't match with inserted elements in order", n);
//             break;
//         }
//     }
//     free_history_list(datalist);
//     free(poped_element);
//     free(inserted_element);
//     return 0;
// }