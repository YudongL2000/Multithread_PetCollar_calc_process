#include "../include/lstmInputList.h"
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 


DataList* init_list(int dim) {
    DataList* input_list = malloc(sizeof(DataList));
    input_list -> head = NULL ;
    input_list -> tail = NULL;
    input_list -> size = 0;
    input_list -> dim = dim;
    return input_list;
}

int get_list_size(DataList* list) {
    return list -> size;
}

void add_to_head(DataList* list, short* elem) {
    ShortNode* new_node = malloc(sizeof(ShortNode));
    short* new_node_payload = calloc(list->dim, sizeof(short));
    new_node -> LSTM_input = new_node_payload;
    memcpy(new_node->LSTM_input, elem, sizeof(short) * (list->dim));

    ShortNode* head_node = list->head;
    if (head_node != NULL) {
        new_node -> prev = NULL;
        new_node -> next = head_node;
        
        head_node -> prev = new_node;
        list -> head = new_node;
    } else {
        new_node -> prev = NULL;
        new_node -> next = NULL;
        list -> head = new_node;
        list -> tail = new_node;
    }
    list -> size += 1;
}

void add_to_tail(DataList* list, short* elem) {
    ShortNode* new_node = malloc(sizeof(ShortNode));
    short* new_node_payload = calloc(list->dim, sizeof(short));
    new_node -> LSTM_input = new_node_payload;
    memcpy(new_node->LSTM_input, elem, sizeof(short) * (list->dim));

    ShortNode* tail_node = list->tail;
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
    list -> size += 1;
}

void pop_from_head(DataList* list) {
    ShortNode* headnode = list -> head;
    if (headnode -> next == NULL) {
        list -> head = NULL;
        list -> tail = NULL;
        list -> size = 0;
    } else {
        ShortNode* new_headnode = headnode -> next;
        new_headnode -> prev = NULL;
        list -> head = new_headnode;
        list -> size -= 1;
    }
    free(headnode -> LSTM_input);
    free(headnode);
}

void pop_from_tail(DataList* list) {
    ShortNode* tailnode = list -> tail;
    if (tailnode -> next == NULL) {
        list -> head = NULL;
        list -> tail = NULL;
        list -> size = 0;
    } else {
        ShortNode* new_tailnode = tailnode -> prev;
        new_tailnode -> next = NULL;
        list -> tail = new_tailnode;
        list -> size -= 1;
    }
    free(tailnode -> LSTM_input);
    free(tailnode);
}


void clear_list(DataList* list) {
    ShortNode* iter = list -> head;
    while (iter != NULL) {
        ShortNode* current_node = iter;
        iter = iter -> next;
        free(current_node -> LSTM_input);
        free(current_node);
    }
    list -> head = NULL;
    list -> tail = NULL;
    list -> size = 0;
}

void free_list(DataList* list) {
    ShortNode* iter = list -> head;
    while (iter != NULL) {
        free(iter -> LSTM_input);
        ShortNode* current_node = iter;
        iter = iter -> next;
        free(current_node);
    }
    free(list);
}





void print_list(DataList* list) {
    ShortNode* head = list -> head;
    ShortNode* tracer = head;
    int counter = 0;
    printf("size: %d\n", list->size);

    if (head != NULL && head -> prev == NULL) {
        printf("NULL");
    }
    while (tracer != NULL) {
        counter += 1;
        if (counter == 1 && tracer->next != NULL) {
            printf("->CELL(HEAD)");
        } else if (counter >1 && tracer -> next == NULL) {
            printf("->CELL(Tail)->NULL");
        } else if (counter == 1 && tracer->next == NULL) {
            printf("->CELL(HEAD/TAIL)->NULL");
        } else {
            printf("->CELL");
        }
        tracer = tracer -> next;
        if (counter > list -> size) {
            break;
        }
    }
    if (counter == 0) {
        printf("(HEAD)NULL(TAIL)");
    }
    printf("\n");
}



bool check_correctness(DataList* list) {
    int counter = 0;
    ShortNode* iter = list -> head;
    if(iter == NULL) {
        if (list -> tail != NULL) {
            printf("list tail should have null pointer when list head have null pointer\n");
            return false;
        } else if (list -> size != 0) {
            printf ("list size should be 0 when it's empty\n");
            return false;
        }
        return true;
    }
    if (iter -> prev != NULL) {
        printf("list head should have null prev pointer\n");
        return false;
    }
    while (iter != NULL) {
        counter += 1;
        if (iter -> LSTM_input == NULL) {
            printf("list element is invalid\n");
        }
        if (iter -> next == NULL) {
            if (list -> tail != iter) {
                printf("list tail has invalid pointer\n");
                return false;
            }
        } else if (iter != iter -> next -> prev) {
            printf("list connection invalid\n");
            return false;
        }
        if (list -> size < counter) {
            printf("list size does not match with number of elements\n");
            return false;
        }
        iter = iter -> next;
    }
    if (list-> size != counter) {
        printf("invalid size");
        return false;
    }
    return true;
}

// int main() {
//     printf("nothing is worth doing\n");
//     int dim = 6;
//     int len = 10;
//     int lim = 10;
//     printf("initing list\n");
//     DataList* datalist = init_list(dim);
//     int total_elem = 10000;
//     for (int i = 0; i < total_elem; i++) {
//         //printf("executing %d th iterations of element insertions", i);
//         short* payload = calloc(10, sizeof(short));
//         //printf("allocated payload");
//         add_to_tail(datalist, payload);
//         //printf("added to tail\n");
//         free(payload);
//         if (datalist -> size > lim) {
//             pop_from_head(datalist);
//         }
//         //print_list(datalist);
//         if (i % 1000 == 0) {
//             //print_list(datalist);
//             //print_list(datalist);
//             clear_list(datalist);
//             //print_list(datalist);
//         }
//         if (check_correctness(datalist) == false) {
//             print_list(datalist);
//             printf("\n");
//             printf("invalid datalist after %d iterations of element insertions", i);
//             return 1;
//         }
//     }
//     free_list(datalist);
//     return 0;
// }