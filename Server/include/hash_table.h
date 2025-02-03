#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define NUM_BUCKETS 128 // should always be a power of two
#define MAX_USERNAME_LEN 256
#define MAX_PASS_LEN 256

typedef struct hash_node
{
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASS_LEN];
    int user_fd;
    struct hash_node *next_node;
} hash_node_t;

typedef struct
{
    hash_node_t *buckets[NUM_BUCKETS];
    size_t total_entries;
} hash_table_t;

hash_table_t *create_hash_table();
char *hash_table_search_via_id(hash_table_t *table, int user_fd);
void destroy_hash_table(hash_table_t *table);
bool hash_table_insert(hash_table_t *table, const char *username, const char *password, int user_fd);
hash_node_t *hash_table_search(hash_table_t *table, const char *username);
bool hash_table_delete(hash_table_t *table, const char *username);
bool hash_table_delete_via_id(hash_table_t *table, int user_fd);
void hash_table_clear(hash_table_t *table);
size_t hash_table_size(hash_table_t *table);
void hash_table_print_stats(hash_table_t *table);

#endif /* HASH_TABLE_H */