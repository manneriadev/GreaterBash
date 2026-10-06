#pragma once
#include "tokens.h"

typedef struct a{
    Token data;
    struct a* next;
} node_t;

typedef struct {
    node_t* head;
    node_t* tail;
} TokenList;

node_t* get_new_item(Token data);
TokenList* get_list();
void append(Token data , TokenList* list);
void free_tokens(TokenList* tokens);