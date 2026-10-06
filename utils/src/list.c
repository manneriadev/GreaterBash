#include <stdlib.h>
#include <stdio.h>
#include "list.h"

node_t* get_new_item(Token data) // malloc make and return new item
{
    node_t* tmp = (node_t*)malloc(sizeof(node_t));
    if(!tmp){fprintf(stderr , "sorry. malloc is not init memory"); exit(EXIT_FAILURE);}
    tmp -> data = data;
    tmp -> next = NULL;
    return tmp;
}

TokenList* get_list() // malloc make and return struct of list
{
    TokenList* tmp = (TokenList*)malloc(sizeof(TokenList));
    if(!tmp){perror("sorry. malloc is not init memory"); exit(EXIT_FAILURE);}
    tmp -> head = NULL;
    tmp -> tail = NULL;
    return tmp;
}

void append(Token data , TokenList* list) // append new elem
{
    node_t* item = get_new_item(data);
    if(list -> head == NULL){
        list -> head = item;
        list -> tail = item;
    } else {
        list -> tail -> next = item;
        list -> tail = item;
    }
}

void free_tokens(TokenList* tokens)
{
    if(!tokens) return;
    node_t* tmp = tokens -> head;
    while(tmp != NULL)
    {
        tmp = tokens->head->next;
        free(tokens->head->data.value);
        free(tokens->head);
        tokens->head = tmp;
    }
    free(tokens);
}