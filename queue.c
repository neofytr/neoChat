#include <stdlib.h>

#include "queue.h"

queue_t *create_queue()
{
    queue_t *new_queue = (queue_t *)malloc(sizeof(queue_t));
    if (!new_queue)
    {
        return NULL;
    }

    new_queue->front = new_queue->rear = NULL;
    return new_queue;
}

int enqueue(int client_fd, char *service, queue_t *queue)
{
    if (!queue)
    {
        return -1;
    }

    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (!new_node)
    {
        return -1;
    }

    new_node->client_fd = client_fd;
    new_node->service = service;
    new_node->next_node = NULL;

    if (!queue->front)
    {
        queue->front = new_node;
        queue->rear = new_node;
    }
    else
    {
        queue->rear->next_node = new_node;
        queue->rear = new_node;
    }

    return 0;
}

int dequeue(queue_t *queue)
{
    if (!queue || !queue->front)
    {
        return -1;
    }

    node_t *temp = queue->front;
    queue->front = queue->front->next_node;

    if (!queue->front)
    {
        queue->rear = NULL;
    }

    free(temp);
    return 0;
}

node_t *peek(queue_t *queue)
{
    if (!queue || !queue->front)
    {
        return NULL;
    }

    return queue->front;
}

int is_empty(queue_t *queue)
{
    return !queue || !queue->front;
}

void clear_queue(queue_t *queue)
{
    while (!is_empty(queue))
    {
        dequeue(queue);
    }
}

void destroy_queue(queue_t *queue)
{
    clear_queue(queue);
    free(queue);
}
