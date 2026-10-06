#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASTNode* create_binary(NodeType type, BinaryNode btype, ASTNode* left , ASTNode* right)
{
    ASTNode* node  = (ASTNode*)malloc(sizeof(ASTNode));
    node->binary.btype = btype;
    node->type = type;
    node->binary.left = left;
    node->binary.right = right;
    return node;
}

ASTNode* create_unary(NodeType type , ASTNode* base)
{
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = type;
    node->unary.base = base;
    return node;
}

ASTNode* create_cmd(Token* token)
{
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = NODE_CMD;
    node->cmd.bg = false;
    node->cmd.redirs = NULL;
    node->cmd.args = NULL;
    return node;
}

ASTNode* create_string(NodeType type, bool flag , char* name)
{
    ASTNode* node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = type;
    node->string.flag = flag;
    node->string.name = strdup(name);
    return node;
}

ASTNode* append_redir(ASTNode* cmd , ASTNode** redir)
{
    if(!cmd || !redir || cmd->type != NODE_CMD ) return cmd;
    cmd->cmd.redirs = redir;
    return cmd;
}

ASTNode* append_arg(ASTNode* cmd , ASTNode** arg)
{
    if(!cmd || !arg || cmd->type != NODE_CMD ) return cmd;
    cmd->cmd.args = arg;
    return cmd;
}

static const char* node_type_str(NodeType t)
{
    switch (t)
    {
        case NODE_GROUP: return "()";
        case NODE_CMD: return "CMD";
        case NODE_ARG: return "ARG";
        case NODE_GT: return ">";
        case NODE_GT_GT: return ">>";
        case NODE_AND_GT: return "&>";
        case NODE_AND_GT_GT: return "&>>";
        case NODE_LT: return "<";
        case NODE_BINARY: return "BINARY";
        default: return "?";
    }
}

static const char* token_type_str(TokenType t)
{
    switch (t)
    {
        case TOKEN_AND: return "&";
        case TOKEN_AND_AND: return "&&";
        case TOKEN_OR: return "|";
        case TOKEN_OR_OR: return "||";
        case TOKEN_OR_AND: return "|&";
        case TOKEN_SEMI: return ";";
        case TOKEN_NL: return "\\n";
        default: return "?";
    }
}


void ast_print_tree(ASTNode* node , size_t space , size_t value)
{
    if(!node) return;

    switch (node->type)
    {
        case NODE_BINARY:
            if(node->binary.left) ast_print_tree(node->binary.left , space + 5 , 2);
            for(size_t i = 0 ; i < space ; ++i) putchar(' ');
            if(value == 2) printf("/---%s[%s]\n" , node_type_str(node->type) , token_type_str(node->binary.btype));
            else if(value == 1) printf("\\---%s[%s]\n" , node_type_str(node->type) , token_type_str(node->binary.btype));
            else printf("%s[%s]\n" , node_type_str(node -> type) , token_type_str(node->binary.btype));
            if(node->binary.right) ast_print_tree(node->binary.right , space + 5 , 1);
            break;
        case NODE_CMD:
            for(size_t i = 0 ; i < space ; ++i) putchar(' ');
            if(value == 2) printf("/---(null)");
            else if(value == 1) printf("\\---(null)");
            else printf("(null)->");
            if(node->cmd.redirs) for(size_t i = 0 ; node->cmd.redirs[i] ; ++i) printf("->(%s in {%s}%s)" , node_type_str(node->cmd.redirs[i]->type) , node_type_str(node->cmd.redirs[i]->unary.base->type), node->cmd.redirs[i]->unary.base->string.name);
            if(node->cmd.args) for(size_t i = 0 ; node->cmd.args[i] ; ++i) printf("[%s: %d]" , node->cmd.args[i]->string.name , node->cmd.args[i]->string.flag);
            printf("\n");
            break;
        case NODE_ARG:
            for(size_t i = 0 ; i < space ; ++i) putchar(' ');
            if(value == 2) printf("/---{%s}%s\n" , node_type_str(node->type) ,  node->string.name);
            else if(value == 1) printf("\\---{%s}%s\n" , node_type_str(node->type) , node->string.name);
            else printf("{%s}%s\n" , node_type_str(node->type) , node->string.name);
            break;
        case NODE_GROUP:
            for(size_t i = 0 ; i < space ; ++i) putchar(' ');
            if(value == 2) printf("/---%s" , node_type_str(node->type));
            else if(value == 1) printf("\\---%s" , node_type_str(node->type));
            else printf("%s" , node_type_str(node->type));
            printf("\n");
            if(node->unary.base) ast_print_tree(node->unary.base , space + 5 , 1);
            break;
        default: break;
    }
}

void ast_clean(ASTNode* node)
{
    if(!node) return;

    switch(node->type)
    {
        case NODE_BINARY:
            ast_clean(node->binary.left);
            ast_clean(node->binary.right);
            break;
        case NODE_AND_GT:
        case NODE_AND_GT_GT:
        case NODE_GT:
        case NODE_GT_GT: 
        case NODE_LT:
        case NODE_GROUP:
            ast_clean(node->unary.base);
            break;
        case NODE_CMD:
            if(node->cmd.args)
            {
                for(size_t i = 0 ; node->cmd.args[i] ; ++i) ast_clean(node->cmd.args[i]);
                free(node->cmd.args);
            }
            if (node->cmd.redirs)
            {
                for(size_t i = 0 ; node->cmd.redirs[i] ; ++i) ast_clean(node->cmd.redirs[i]);
                free(node->cmd.redirs);
            }
            break;
        case NODE_ARG:
            free(node->string.name);
            break;
        default: break;
    }
    free(node);
}