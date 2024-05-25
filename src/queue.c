#include <stdio.h>
#include <stdlib.h>
#include "../include/queue.h"


PriorityQueue* initPriorityQueue() {
    PriorityQueue* queue = malloc(sizeof(PriorityQueue));
    queue->elements = calloc(MAX_QUEUE_LEN, sizeof(int));
    queue->size = 0;
    return queue;
}

// Function to enqueue an element into the priority queue
void enqueue(PriorityQueue *queue, int data, int priority) {
    if (queue->size >= MAX_QUEUE_LEN) {
        fprintf(stderr, "Priority queue is full\n");
        return;
    }

    int index = queue->size;
    queue->elements[index].data = data;
    queue->elements[index].priority = priority;

    // Percolate up to maintain heap property
    while (index > 0 && queue->elements[(index - 1) / 2].priority > priority) {
        // Swap with parent
        QueueElement temp = queue->elements[index];
        queue->elements[index] = queue->elements[(index - 1) / 2];
        queue->elements[(index - 1) / 2] = temp;
        index = (index - 1) / 2;
    }

    queue->size++;
}

int dequeue(PriorityQueue *queue) {
    if (queue->size == 0) {
        fprintf(stderr, "Priority queue is empty\n");
        return -1;
    }

    int data = queue->elements[0].data;
    queue->size--;

    // Move the last element to the root
    queue->elements[0] = queue->elements[queue->size];

    // Percolate down to maintain heap property
    int index = 0;
    while (2 * index + 1 < queue->size) {
        int left_child = 2 * index + 1;
        int right_child = 2 * index + 2;
        int min_child = left_child;

        if (right_child < queue->size && queue->elements[right_child].priority < queue->elements[left_child].priority) {
            min_child = right_child;
        }

        if (queue->elements[index].priority <= queue->elements[min_child].priority) {
            break;
        }

        // Swap with the minimum child
        QueueElement temp = queue->elements[index];
        queue->elements[index] = queue->elements[min_child];
        queue->elements[min_child] = temp;
        index = min_child;
    }

    return data;
}

void free_priority_queue(PriorityQueue* queue) {
    free(queue -> elements);
    free(queue);
    return;
}

// int main() {
//     PriorityQueue* queue = initPriorityQueue();

//     // Enqueue elements with priorities
//     enqueue(queue, 10, 2);
//     enqueue(queue, 20, 1);
//     enqueue(queue, 30, 3);

//     // Dequeue elements
//     printf("Dequeued element: %d\n", dequeue(queue));
//     printf("Dequeued element: %d\n", dequeue(queue));
//     printf("Dequeued element: %d\n", dequeue(queue));
//     printf("elements remaining in the queue: %d\n", queue->size);

//     return 0;
// }