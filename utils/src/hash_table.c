#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

size_t hash_func(char* key)
{
    size_t value = 0;
    for(size_t i = 0 ; key[i] != '\0' ; ++i) value += (value<<5) + key[i];
    return value%256;
}

hash_table* create_table()
{
    hash_table* table = malloc(sizeof(hash_table));
    if(!table)
    {
        fprintf(stderr, "mallco is not init mem for hash table");
        return NULL;
    }
    memset(table , 0 , sizeof(hash_table));
    return table;    
}

node_tt* create_base(char* key , char* value)
{
    node_tt* base = malloc(sizeof(node_tt));
    if(!base)
    {
        fprintf(stderr , "malloc not init mem for base of hash table");
        return NULL;
    }
    base->key = strdup(key);
    base->value = strdup(value);
    base->next = NULL;
    return base;
}

void table_insert(hash_table* table , const char* key , const char* value)
{
    if(!table || !key) return;

    size_t index = hash_func(key);
    node_tt* cur = table->buckets[index];

    while(cur) {
        if(strcmp(cur->key , key) == 0)
        {
            free(cur->value);
            cur->value = strdup(value);
            return;
        }
        cur = cur->next;
    }
    node_tt* node = create_base((char*)key , (char*)(value?value:""));
    if(!node) return;

    node->next = table->buckets[index];
    table->buckets[index] = node;
    return;
}

char* table_get(hash_table* table , const char* key)
{
    if(!table || !key) return NULL;

    size_t index = hash_func(key);
    node_tt* cur = table->buckets[index];
    while(cur)
    {
        if(strcmp(cur->key, key) == 0) return cur->value;
        cur = cur -> next;
    }
    return NULL;
}

void table_del(hash_table* table , const char* key)
{
    if(!table || !key) return;
    
    size_t hash = hash_func(key);
    node_tt* cur = table->buckets[hash];
    node_tt* prev = NULL;
    while(cur)
    {
        if(strcmp(cur->key , key) == 0)
        {
            if(prev)prev->next = cur->next;
            else table->buckets[hash] = cur->next;
            free(cur->key);
            free(cur->value);
            free(cur);
            break;
        }
        prev = cur;
        cur = cur->next;
    }

}

void print_table(hash_table* table)
{
    if(!table) return;

    for(size_t i = 0 ; i < HASH_TABLE_SIZE ; ++i)
    {
        node_tt* cur = table->buckets[i];
        while(cur)
        {
            fprintf(stdout , "%s=%s\n" , cur->key , cur->value);
            cur = cur->next;
        }
    }
}

hash_table* copy_table(hash_table* table)
{
    if(!table) return NULL;

    hash_table* new_table = create_table();

    for(size_t i = 0 ; i < HASH_TABLE_SIZE ; ++i)
    {
        if(table->buckets[i] != NULL)
        {
            node_tt* tmp = table->buckets[i];
            while(tmp)
            {
                table_insert(new_table , tmp->key , tmp->value);
                tmp = tmp->next;
            }
        }
    }

    return new_table;
}

void table_free(hash_table* table)
{
    for(size_t i = 0; i < HASH_TABLE_SIZE ; ++i)
    {
        node_tt* cur = table->buckets[i];
        while(cur)
        {
            node_tt* tmp = cur->next;
            free(cur->value);
            free(cur->key);
            free(cur);
            cur = tmp;
        }
    }
    free(table);
}