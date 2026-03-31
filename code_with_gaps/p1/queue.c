#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

void init_queue(Queue *queue, int queue_size)
{
    register int i;

    if (queue_size < 1)
    {
        perror("Error: The queue size must be greater than or equal to 1");
        exit(1);
    }
    if (queue == NULL)
    {
        perror("Error: The pointer to the queue is NULL in init_queue");
        exit(2);
    }
    // TODO: TO BE COMPLETED BY STUDENT
    ...
    ...
}

void free_queue(Queue *queue)
{
    // Memory pointed to by each pointer stored in the queue must be freed,
    // as well as the memory where those pointers are stored,
    // and the semaphores and mutexes must be destroyed.

    // TODO: TO BE COMPLETED BY STUDENT
    ...
    ...
}

void enqueue(Queue *queue, queue_item *item)
{
    // copy the pointer directly into the appropriate position in the queue
    // TODO: TO BE COMPLETED BY STUDENT
    ...
    ...
}

queue_item *dequeue(Queue *queue)
{
    queue_item *p;

    // TODO: TO BE COMPLETED BY STUDENT
    ...
    ...
}
