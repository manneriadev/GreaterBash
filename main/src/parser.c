#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

ASTNode* parse_binary(int priotity);
ASTNode* parse_primary();
ASTNode* parse_cmd(); // all names + str

static void __append__cmd(ASTNode*** array , ASTNode* smth);
static bool is_redir(TokenType type);
static bool is_bin(TokenType type);
static void next_token();
static bool match(TokenType type);

static node_t* cur = NULL;

ASTNode* parse(TokenList* list)
{
    cur = list->head;
    return parse_binary(1);
}

static int get_priority(TokenType type)
{
    if(type == TOKEN_SEMI || type == TOKEN_NL) return 1;
    if(type == TOKEN_OR || type == TOKEN_OR_AND) return 2;
    if(type == TOKEN_OR_OR || type == TOKEN_AND_AND) return 3;
    if(type == TOKEN_AND) return 4;
    return 0;
}

ASTNode* parse_binary(int priority)
{
    ASTNode* left = parse_primary();
    if(!left) return NULL;

    while(cur && !match(TOKEN_EOF) && !match(TOKEN_RPAREN))
    {
        TokenType type = cur->data.type;
        int pr = get_priority(type);

        if(pr < priority) break;

        next_token();

        if(type == TOKEN_AND && left->type == NODE_CMD) left->cmd.bg = true;
    
        ASTNode* right = parse_binary(pr + 1);
        if(!right) return left;
        
        left = create_binary(NODE_BINARY , type , left , right);
    }
    return left;
}

ASTNode* parse_primary()
{

    ASTNode* base = NULL;
    if(match(TOKEN_LPAREN))
    {
        next_token();
        base = parse_binary(1);
        if(!match(TOKEN_RPAREN))
        {
            fprintf(stderr , "not expected ')' after '('\n");
            return NULL;
        }
        next_token();
        return create_unary(NODE_GROUP , base);
    }

    return parse_cmd();
}

ASTNode* parse_cmd()
{
    if(!cur || match(TOKEN_EOF)) return NULL;

    ASTNode* node = NULL;
    ASTNode** redirs = NULL;
    ASTNode** var_args = NULL;

    while(cur && !match(TOKEN_EOF) && !is_bin(cur->data.type) && !match(TOKEN_RPAREN))
    {
        TokenType t = cur->data.type;
        if(t == TOKEN_WORD)
        {
            __append__cmd(&var_args , create_string(NODE_ARG , cur->data.flag , cur->data.value));
            next_token();
            continue;
        }

        if(is_redir(t))
        {
            bool flag = cur->data.flag;
            next_token();
            if(!cur || cur->data.type != TOKEN_WORD)
            {
                fprintf(stderr , "unexpected token near redir");
                if(redirs)
                {
                    for(size_t i = 0 ; redirs[i] != NULL ; ++i) ast_clean(redirs[i]);
                    free(redirs);
                }
                if(var_args)
                {
                    for(size_t i = 0 ; var_args[i] != NULL ; ++i) ast_clean(var_args[i]);
                    free(var_args);
                }
                return NULL;
            }
            __append__cmd(&redirs , create_unary(
                (t == TOKEN_GT) ? NODE_GT :
                (t == TOKEN_GT_GT) ? NODE_GT_GT : 
                (t == TOKEN_AND_GT) ? NODE_AND_GT :
                (t == TOKEN_AND_GT_GT) ? NODE_AND_GT_GT : 
                NODE_LT , create_string(NODE_ARG , flag , cur->data.value))
            );

            next_token();
            continue;
        }
        break;
    }
    if(!var_args)
    {
        fprintf(stderr , "no cmd name. exited");
        if(redirs)
        {
            for(size_t i = 0 ; redirs[i] ; ++i) ast_clean(redirs[i]);
            free(redirs);
        }
        return NULL;
    }

    node = create_cmd(&(Token){TOKEN_WORD , NULL , 0});
    append_arg(node , var_args);
    append_redir(node , redirs);

    return node;
}

static void __append__cmd(ASTNode*** array , ASTNode* smth)
{

    if(!(*array))
    {
        *array = malloc(sizeof(ASTNode*) * 2);
        (*array)[0] = smth;
        (*array)[1] = NULL;
        return;
    }

    size_t count = 0;
    while((*array)[count]) ++count;
    *array = realloc(*array , sizeof(ASTNode*) * (count + 2));
    (*array)[count] = smth;
    (*array)[count + 1] = NULL;
}

static bool is_redir(TokenType type)
{
    return type == TOKEN_AND_GT || type == TOKEN_AND_GT_GT || type == TOKEN_GT || 
    type == TOKEN_GT_GT || type == TOKEN_LT;
}

static bool is_bin(TokenType type)
{
    return type == TOKEN_AND || type == TOKEN_AND_AND || type == TOKEN_SEMI || 
    type == TOKEN_OR || type == TOKEN_OR_OR || type == TOKEN_OR_AND || type == TOKEN_NL;
}

static void next_token()
{
    if(cur) cur = cur->next;
}

static bool match(TokenType type)
{
    return cur && cur->data.type == type;
}