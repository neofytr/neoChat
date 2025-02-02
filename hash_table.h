#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_BUCKETS 128 // should always be a power of two
#define MAX_USERNAME_LEN 256
#define MAX_PASS_LEN 256

typedef struct hash_node
{
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASS_LEN];
    struct hash_node *next_node;
} hash_node_t;

typedef struct
{
    hash_node_t *buckets[NUM_BUCKETS];
    size_t total_entries;
} hash_table_t;

hash_table_t *create_hash_table();
void destroy_hash_table(hash_table_t *table);
bool hash_table_insert(hash_table_t *table, const char *username, const char *password);
hash_node_t *hash_table_search(hash_table_t *table, const char *username);
bool hash_table_delete(hash_table_t *table, const char *username);
void hash_table_clear(hash_table_t *table);
size_t hash_table_size(hash_table_t *table);
void hash_table_print_stats(hash_table_t *table);

size_t get_hash(const char *username)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *username++))
    {
        hash = ((hash << 5) + hash) + c;
    }

    return hash % NUM_BUCKETS;
}

hash_table_t *create_hash_table()
{
    hash_table_t *table = (hash_table_t *)malloc(sizeof(hash_table_t));
    if (!table)
    {
        return NULL;
    }

    memset(table->buckets, 0, sizeof(table->buckets));
    table->total_entries = 0;

    return table;
}

void destroy_hash_table(hash_table_t *table)
{
    if (!table)
    {
        return;
    }

    hash_table_clear(table);

    free(table);
}

bool hash_table_insert(hash_table_t *table, const char *username, const char *password)
{
    if (!table || !username || strlen(username) >= MAX_USERNAME_LEN || strlen(password) >= MAX_PASS_LEN)
    {
        return false;
    }

    if (hash_table_search(table, username))
    {
        return false;
    }

    size_t hash = get_hash(username);

    hash_node_t *new_node = (hash_node_t *)malloc(sizeof(hash_node_t));
    if (!new_node)
    {
        return false;
    }

    strncpy(new_node->username, username, MAX_USERNAME_LEN - 1);
    new_node->username[strlen(username) - 1] = '\0';

    strncpy(new_node->password, password, MAX_PASS_LEN - 1);
    new_node->username[strlen(password) - 1] = '\0';

    new_node->next_node = table->buckets[hash];
    table->buckets[hash] = new_node;

    table->total_entries++;

    return true;
}

hash_node_t *hash_table_search(hash_table_t *table, const char *username)
{
    if (!table || !username)
    {
        return NULL;
    }

    size_t hash = get_hash(username);
    hash_node_t *current = table->buckets[hash];

    while (current)
    {
        if (!strcmp(current->username, username))
        {
            return current;
        }
        current = current->next_node;
    }

    return NULL;
}

bool hash_table_delete(hash_table_t *table, const char *username)
{
    if (!table || !username)
    {
        return false;
    }

    size_t hash = get_hash(username);
    hash_node_t *current = table->buckets[hash];
    hash_node_t *prev = NULL;

    while (current)
    {
        if (strcmp(current->username, username) == 0)
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

void hash_table_clear(hash_table_t *table)
{
    if (!table)
    {
        return;
    }

    for (size_t i = 0; i < NUM_BUCKETS; i++)
    {
        hash_node_t *current = table->buckets[i];
        while (current)
        {
            hash_node_t *temp = current;
            current = current->next_node;
            free(temp);
        }
        table->buckets[i] = NULL;
    }

    table->total_entries = 0;
}

size_t hash_table_size(hash_table_t *table)
{
    return table ? table->total_entries : 0;
}

void hash_table_print_stats(hash_table_t *table)
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
        hash_node_t *current = table->buckets[i];

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

    printf("Hash Table Statistics:\n");
    printf("Total Entries: %zu\n", table->total_entries);
    printf("Number of Buckets: %d\n", NUM_BUCKETS);
    printf("Non-empty Buckets: %zu\n", non_empty_buckets);
    printf("Max Chain Length: %zu\n", max_chain_length);
}

#undef NUM_BUCKETS // should always be a power of two
#undef MAX_USERNAME_LEN
#undef MAX_PASS_LEN

#endif /* HASH_TABLE_H */