#include <stdio.h>
#include <stdlib.h>

const int MAX_QUEUE_LEN = 30;

typedef struct {
    int data;
    int priority;
} QueueElement;

// Define a priority queue structure
typedef struct {
    QueueElement* elements;
    int size;
} PriorityQueue;


PriorityQueue* initPriorityQueue();
void enqueue(PriorityQueue *queue, int data, int priority);
int dequeue(PriorityQueue *queue);
void free_priority_queue(PriorityQueue* queue);





