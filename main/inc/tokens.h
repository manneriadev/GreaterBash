#pragma once
#include <stdbool.h>

typedef enum{
    TOKEN_OR,           // |
    TOKEN_OR_AND,           // |&
    TOKEN_SEMI,         // ;
    TOKEN_AND_AND,          // &&
    TOKEN_OR_OR,            // ||
    TOKEN_AND,          // &
    TOKEN_NL,   // newline

    TOKEN_WORD, // ls echo , file.txt , pattern , -flags , ./text , /bin/usr and more

    TOKEN_LPAREN, // (
    TOKEN_RPAREN, // )

    TOKEN_GT,             // >
    TOKEN_GT_GT,            // >>
    TOKEN_AND_GT,           // &>
    TOKEN_AND_GT_GT,            // &>>
    TOKEN_LT,           // <

    TOKEN_EOF ,  //    eof - space\tabs is ignored
    TOKEN_ERR   // all things that my shell not known (will be used)
} TokenType;

typedef struct Token{
    TokenType type;
    char* value;
    bool flag; // 1 - has assign , 2 - has variable , 4 - has string solo , 8 - has string duo , 
    //16 - has math expr , 32 - has command expr
} Token;