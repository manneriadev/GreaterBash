#pragma once
#include "tokens.h"
#include "list.h"
#include <stdio.h>

// binary opers
// all tokens i divide in simple command , that will be united with -> | || && |& ; \n
// word = word created and did separately
// 
//  unary opers that -> > >> &> &>> < & 
// 
//  string is arg of all this NodeType
//
//  cmd is a first word in simple command and all another words is args
//
// !! if i meet lparen i must find rparen else it is error 
// !! or wait next input data and wait then )

typedef enum {
    NODE_CMD,   // all simple command 
    NODE_ARG, // argument words
    
    NODE_GROUP, // ()

    NODE_GT,             // >
    NODE_GT_GT,            // >>
    NODE_AND_GT,           // &>
    NODE_AND_GT_GT,            // &>>
    NODE_LT,           // <
    
    NODE_BINARY, // all binary opers
} NodeType;

typedef enum{
    BNODE_OR,           // |
    BNODE_OR_AND,           // |&
    BNODE_SEMI,         // ;
    BNODE_AND_AND,          // &&
    BNODE_OR_OR,            // ||
    BNODE_AND,          // &
    BNODE_NL,   // newline
} BinaryNode;

typedef struct ASTNode{

    NodeType type;
    union {
        struct{
            BinaryNode btype;
            struct ASTNode* left;
            struct ASTNode* right;
        } binary;

        struct{
            struct ASTNode* base;
        } unary;

        struct{
            bool bg;
            struct ASTNode** redirs;
            struct ASTNode** args;
        }cmd;

        struct{
            char* name;
            bool flag; // 1 - has assign
        } string;
    };
} ASTNode;

ASTNode* create_binary(NodeType type, BinaryNode btype, ASTNode* left, ASTNode* right);
ASTNode* create_unary(NodeType type, ASTNode* base);
ASTNode* create_string(NodeType type , bool flag , char* name);
ASTNode* create_cmd(Token*);

ASTNode* append_redir(ASTNode* cmd , ASTNode** redir);
ASTNode* append_arg(ASTNode* cmd , ASTNode** arg);
void ast_print_tree(ASTNode* node , size_t space , size_t value);
void ast_clean(ASTNode* node);