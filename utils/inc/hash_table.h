#pragma once

#define HASH_TABLE_SIZE 256

typedef struct node_t{
    char* key;
    char* value;
    struct node_t* next;
} node_tt;

typedef struct hash_node{
    node_tt* buckets[HASH_TABLE_SIZE];
} hash_table;

size_t hash_func(char* key);
hash_table* create_table();
node_tt* create_base(char* key , char* value);
void table_insert(hash_table* table , const char* key , const char* value);
char* table_get(hash_table* table , const char* key);
void table_del(hash_table* table , const char* key);
void print_table(hash_table* table);
hash_table* copy_table(hash_table* table);
void table_free(hash_table* table);