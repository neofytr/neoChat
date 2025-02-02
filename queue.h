#ifndef QUEUE_H
#define QUEUE_H

typedef struct node_ node_t;
typedef struct queue_ queue_t;

struct queue_
{
    node_t *front;
    node_t *rear;
};

struct node_
{
    int client_fd;
    node_t *next_node;
};

queue_t *create_queue();
int enqueue(int client_fd, queue_t *queue);
int dequeue(queue_t *queue);
int is_empty(queue_t *queue);
void clear_queue(queue_t *queue);
void destroy_queue(queue_t *queue);
node_t *peek(queue_t *queue);

#endif