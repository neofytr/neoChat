#ifndef mutex_table_H
#define mutex_table_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

#define NUM_BUCKETS 128 // should always be a power of two

typedef struct mutex_node_
{
    int32_t user_fd;
    pthread_mutex_t user_mutex;
    struct mutex_node_ *next_node;
} mutex_node_t;

typedef struct
{
    mutex_node_t *buckets[NUM_BUCKETS];
    size_t total_entries;
} mutex_table_t;

mutex_table_t *create_mutex_table();
void destroy_mutex_table(mutex_table_t *table);
bool mutex_table_insert(mutex_table_t *table, int32_t user_fd);
mutex_node_t *mutex_table_search(mutex_table_t *table, int32_t user_fd);
bool mutex_table_delete(mutex_table_t *table, int32_t user_fd);
void mutex_table_clear(mutex_table_t *table);
size_t mutex_table_size(mutex_table_t *table);
void mutex_table_print_stats(mutex_table_t *table);

size_t get_int_hash(int32_t user_fd)
{
    // using a variation of Jenkins hash function for integers
    uint32_t hash = (uint32_t)user_fd;
    hash = (hash + 0x7ed55d16) + (hash << 12);
    hash = (hash ^ 0xc761c23c) ^ (hash >> 19);
    hash = (hash + 0x165667b1) + (hash << 5);
    hash = (hash + 0xd3a2646c) ^ (hash << 9);
    hash = (hash + 0xfd7046c5) + (hash << 3);
    hash = (hash ^ 0xb55a4f09) ^ (hash >> 16);

    return hash % NUM_BUCKETS;
}

mutex_table_t *create_mutex_table()
{
    mutex_table_t *table = (mutex_table_t *)malloc(sizeof(mutex_table_t));
    if (!table)
    {
        return NULL;
    }

    memset(table->buckets, 0, sizeof(table->buckets));
    table->total_entries = 0;

    return table;
}

void destroy_mutex_table(mutex_table_t *table)
{
    if (!table)
    {
        return;
    }

    mutex_table_clear(table);
    free(table);
}

bool mutex_table_insert(mutex_table_t *table, int32_t user_fd)
{
    if (!table)
    {
        return false;
    }

    if (mutex_table_search(table, user_fd))
    {
        return false; // user_fd already exists
    }

    size_t hash = get_int_hash(user_fd);

    mutex_node_t *new_node = (mutex_node_t *)malloc(sizeof(mutex_node_t));
    if (!new_node)
    {
        return false;
    }

    new_node->user_fd = user_fd;
    new_node->user_mutex = PTHREAD_MUTEX_INITIALIZER;
    new_node->next_node = table->buckets[hash];
    table->buckets[hash] = new_node;

    table->total_entries++;

    return true;
}

mutex_node_t *mutex_table_search(mutex_table_t *table, int32_t user_fd)
{
    if (!table)
    {
        return NULL;
    }

    size_t hash = get_int_hash(user_fd);
    mutex_node_t *current = table->buckets[hash];

    while (current)
    {
        if (current->user_fd == user_fd)
        {
            return current;
        }
        current = current->next_node;
    }

    return NULL;
}

bool mutex_table_delete(mutex_table_t *table, int32_t user_fd)
{
    if (!table)
    {
        return false;
    }

    size_t hash = get_int_hash(user_fd);
    mutex_node_t *current = table->buckets[hash];
    mutex_node_t *prev = NULL;

    while (current)
    {
        if (current->user_fd == user_fd)
        {
            if (prev)
            {
                prev->next_node = current->next_node;
            }
            else
            {
                table->buckets[hash] = current->next_node;
            }

            free(current);
            table->total_entries--;

            return true;
        }

        prev = current;
        current = current->next_node;
    }

    return false;
}

void mutex_table_clear(mutex_table_t *table)
{
    if (!table)
    {
        return;
    }

    for (size_t i = 0; i < NUM_BUCKETS; i++)
    {
        mutex_node_t *current = table->buckets[i];
        while (current)
        {
            mutex_node_t *temp = current;
            current = current->next_node;
            free(temp);
        }
        table->buckets[i] = NULL;
    }

    table->total_entries = 0;
}

size_t mutex_table_size(mutex_table_t *table)
{
    return table ? table->total_entries : 0;
}

void mutex_table_print_stats(mutex_table_t *table)
{
    if (!table)
    {
        return;
    }

    size_t max_chain_length = 0;
    size_t non_empty_buckets = 0;

    for (size_t i = 0; i < NUM_BUCKETS; i++)
    {
        size_t current_chain_length = 0;
        mutex_node_t *current = table->buckets[i];

        while (current)
        {
            current_chain_length++;
            current = current->next_node;
        }

        if (current_chain_length > 0)
        {
            non_empty_buckets++;
        }

        if (current_chain_length > max_chain_length)
        {
            max_chain_length = current_chain_length;
        }
    }

    printf("Integer Hash Table Statistics:\n");
    printf("Total Entries: %zu\n", table->total_entries);
    printf("Number of Buckets: %d\n", NUM_BUCKETS);
    printf("Non-empty Buckets: %zu\n", non_empty_buckets);
    printf("Max Chain Length: %zu\n", max_chain_length);
}

#endif /* mutex_table_H */