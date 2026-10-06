#include "lexer.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    const char* input;
    size_t index;
} Lexer;

TokenList* Tokenize(const char*);
static Token extract(Lexer*);

Token extract_word(Lexer*);
Token extrart_groups(Lexer*);

static void skip(Lexer* lexer);
static void push_index(Lexer* lexer , size_t n);
static char peek (Lexer* lexer);
static bool match(Lexer* lexer , char c);
static bool match_nl(Lexer* lexer);
static bool match_oper_char(char c);
static bool match_space(Lexer* lexer);
static bool match_end(Lexer* lexer);

TokenList* list;

TokenList* Tokenize(const char* input){
    Lexer lexer = {input , 0};
    list = get_list();
    if(!list)
    {
        fprintf(stderr , "malloc is not init memory for tokens in list\n");
        return NULL;
    }

    while(1)
    {
        while(match_space(&lexer)) push_index(&lexer , 1);

        if(match_end(&lexer)) break;
        Token token = extract(&lexer);
        if(token.type == TOKEN_ERR) return NULL;
        append(token , list);
    }
    append((Token){TOKEN_EOF , NULL , 0} , list);
    return list;
}

static Token extract(Lexer* lexer)
{
    while(match_space(lexer)) push_index(lexer , 1);
    if(match_end(lexer)) return (Token){TOKEN_EOF , NULL , 0};
    if(match_nl(lexer))
    {
        push_index(lexer , 1);
        return (Token){TOKEN_NL , strdup("\n") , 0};
    }

    if(match(lexer , '('))
    {
        push_index(lexer , 1);
        return (Token){TOKEN_LPAREN , strdup("(") , 0};
    }

    if(match(lexer , ')'))
    {
        push_index(lexer , 1);
        return (Token){TOKEN_RPAREN , strdup(")") , 0};
    }
    
    if(match(lexer , '#'))
    {
        skip(lexer);
        return extract(lexer);
    } 
    //else ("><&|;")

    if(match(lexer, ';')){
        push_index(lexer , 1);
        return (Token){TOKEN_SEMI, strdup(";") , 0};
    }

    if(match(lexer, '>'))
    {
        push_index(lexer , 1);
        if(match(lexer , '>'))
        {
            push_index(lexer , 1);
            return (Token){TOKEN_GT_GT , strdup(">>") , 0};
        }
        return (Token){TOKEN_GT , strdup(">") , 0};
    }

    if(match(lexer, '<'))
    {
        push_index(lexer , 1); 
        return (Token){TOKEN_LT , strdup("<") , 0};
    }

    if(match(lexer , '&'))
    {
        push_index(lexer , 1);
        if(match(lexer , '>'))
        {
            push_index(lexer , 1);
            if(match(lexer , '>'))
            {
                push_index(lexer , 1);
                return (Token){TOKEN_AND_GT_GT , strdup("&>>") , 0};
            }
            return (Token){TOKEN_AND_GT , strdup("&>") , 0};
        }
        if(match(lexer , '&'))
        {
            push_index(lexer , 1);
            return (Token){TOKEN_AND_AND , strdup("&&") , 0};
        }
        return (Token){TOKEN_AND , strdup("&") , 0};
    }

    if(match(lexer , '|'))
    {
        push_index(lexer , 1);
        if(match(lexer , '|'))
        {
            push_index(lexer , 1);
            return (Token){TOKEN_OR_OR , strdup("||") , 0};
        }
        if(match(lexer , '&'))
        {
            push_index(lexer , 1);
            return (Token){TOKEN_OR_AND , strdup("|&") , 0};
        }
        return (Token){TOKEN_OR , strdup("|") , 0};
    }

    return extract_word(lexer);    
}

Token extract_word(Lexer* lexer)
{
    bool flag = 0; // 1 - assign
    int _state = 0; // 0 - without str , 1 - solo , 2 - duo

    size_t start = lexer->index;

    while(!match_end(lexer))
    {
        char c = peek(lexer);
        if(!_state)
        {
            if(c == '\\')
            {
                push_index(lexer, 2);
                continue;
            }
            if(c == '\'')
            {
                _state = 1;
                push_index(lexer , 1);
                continue;
            }
            if(c == '\"')
            {
                _state = 2;
                push_index(lexer , 1);
                continue;
            }
            if(c == '\0' || match_oper_char(c) || c == ' ' || c == '\t') break;

            if(c == '=')
            {
                push_index(lexer , 1);
                char* str = lexer->input;
                if(lexer->index == start)
                {
                    return (Token){TOKEN_ERR , NULL , 0};
                }
                if((str[start] < 'A' || (str[start] > 'Z' && str[start] < 'a') || str[start] > 'z') && str[start]!='_')
                {
                    fprintf(stderr , "unknown char near =");
                    return (Token){TOKEN_ERR , NULL , 0};
                }
                for(size_t i = start ; str[i] != '='; ++i)
                {
                    char tmp = str[i];
                    if((tmp < 'A' || (tmp > 'Z' && tmp<'a') || tmp>'z') && (tmp<'0'|| tmp>'9') && tmp!='_')
                    {
                        fprintf(stderr , "unknown char near =");
                        return (Token){TOKEN_ERR , NULL , 0};
                    }
                }
                flag = true;
                continue;
            }

            if(c == '$' && lexer->input[lexer->index+1] == '(' && lexer->input[lexer->index+2] == '(')
            {
                push_index(lexer , 3);
                int paren = 2;
                while(!match_end(lexer) && !match_nl(lexer))
                {
                    if(peek(lexer) == '$' && lexer->input[lexer->index + 1] != '(')   
                    {
                        char* str = lexer->input;
                        if((str[lexer->index+1] < 'A' || (str[lexer->index+1] > 'Z' && str[lexer->index+1] < 'a') || str[lexer->index+1] > 'z') && str[lexer->index+1]!='_')
                        {
                            fprintf(stderr , "unknown char near $");
                            return (Token){TOKEN_ERR , NULL , 0};
                        }
                        for(size_t i = 1 ; str[lexer->index + i] != '\0' && !match_oper_char(str[lexer->index + i]) && (str[lexer->index + i] == ' ' || str[lexer->index + i] == '\t') && str[lexer->index + i] != '\"' && str[lexer->index + i] !='\'' && str[lexer->index + i] != '='; ++i)
                        {
                            char tmp = str[lexer->index + i];
                            if((tmp < 'A' || (tmp > 'Z' && tmp<'a') || tmp>'z') && (tmp<'0'|| tmp>'9') && tmp!='_')
                            {
                                fprintf(stderr , "unknown char near $");
                                return (Token){TOKEN_ERR , NULL , 0};
                            }
                        }
                        push_index(lexer , 1);
                        continue;
                    }

                    if(peek(lexer) == '(') ++paren;
                    else if(peek(lexer) == ')') --paren;
                    if(paren == 0)
                    {
                        push_index(lexer , 1);
                        break;
                    }
                    push_index(lexer , 1);
                }
                if(paren)
                {
                    fprintf(stderr , "not expect ))\n");
                    return (Token){TOKEN_ERR , NULL , 0};
                }
                continue;
            }

            if(c == '$' && lexer->input[lexer->index+1] == '(')
            {
                int paren = 1;
                push_index(lexer , 2);
                while(!match_end(lexer) && !match_nl(lexer))
                {
                    if(peek(lexer) == '(') ++paren;
                    else if(peek(lexer) == ')') --paren;
                    if(paren == 0)
                    {
                        push_index(lexer , 1);
                        break;
                    }
                    push_index(lexer , 1);
                }
                if(paren)
                {
                    fprintf(stderr , "not expect )\n");
                    return (Token){TOKEN_ERR , NULL , 0};
                }
                continue;
            }

            if(c == '$')
            {
                char* str = lexer->input;

                if(str[lexer->index+1]=='?'|| str[lexer->index+1]=='$'|| str[lexer->index+1]=='!')
                {
                    push_index(lexer , 2);
                    continue;
                }
                /*
                if((str[lexer->index+1] < 'A' || (str[lexer->index+1] > 'Z' && str[lexer->index+1] < 'a') || str[lexer->index+1] > 'z') && str[lexer->index+1]!='_')
                {
                    fprintf(stderr , "unknown char near $");
                    return (Token){TOKEN_ERR , NULL , 0};
                }
                for(size_t i = 2 ; str[lexer->index + i] != '\0' && !match_oper_char(str[lexer->index + i]) && (str[lexer->index + i] != ' ' || str[lexer->index + i] != '\t') && str[lexer->index + i] != '\"' && str[lexer->index + i] !='\'' && str[lexer->index + i] != '='; ++i)
                {
                    char tmp = str[lexer->index + i];
                    if((tmp < 'A' || (tmp > 'Z' && tmp<'a') || tmp>'z') && (tmp<'0'|| tmp>'9') && tmp!='_')
                    {
                        fprintf(stderr , "unknown char near $");
                        return (Token){TOKEN_ERR , NULL , 0};
                    }
                }
                */
                push_index(lexer , 1);
                continue;
            }

            push_index(lexer , 1);
        }
        else if (_state == 1)
        {
            if(c == '\'')
            {
                _state = 0;
                push_index(lexer , 1);
                continue;
            }
            push_index(lexer , 1);
        }
        else if (_state == 2)
        {
            if(c == '\\')
            {
                char next = lexer->input[lexer->index + 1];
                if(next == '\\' || next == '\"' || next == '\$')
                {
                    push_index(lexer , 2);
                    continue;
                }
            }

            if(c == '\"')
            {
                _state = 0;
                push_index(lexer , 1);
                continue;
            }

            if(c == '$' && lexer->input[lexer->index+1] == '(' && lexer->input[lexer->index+2] == '(')
            {
                push_index(lexer , 3);
                int paren = 2;
                while(!match_end(lexer) && !match_nl(lexer))
                {
                    if(peek(lexer) == '$' && lexer->input[lexer->index + 1] != '(')
                    {
                        char* str = lexer->input;
                        if((str[lexer->index+1] < 'A' || (str[lexer->index+1] > 'Z' && str[lexer->index+1] < 'a') || str[lexer->index+1] > 'z') && str[lexer->index+1]!='_')
                        {
                            fprintf(stderr , "unknown char near $");
                            return (Token){TOKEN_ERR , NULL , 0};
                        }
                        for(size_t i = 1 ; str[lexer->index + i] != '\0' && !match_oper_char(str[lexer->index + i]) && (str[lexer->index + i] == ' ' || str[lexer->index + i] == '\t') && str[lexer->index + i] != '\"' && str[lexer->index + i] !='\'' && str[lexer->index + i] != '='; ++i)
                        {
                            char tmp = str[lexer->index + i];
                            if((tmp < 'A' || (tmp > 'Z' && tmp<'a') || tmp>'z') && (tmp<'0'|| tmp>'9') && tmp!='_')
                            {
                                fprintf(stderr , "unknown char near $");
                                return (Token){TOKEN_ERR , NULL , 0};
                            }
                        }
                        push_index(lexer , 1);
                        continue;
                    }

                    if(peek(lexer) == '(') ++paren;
                    else if(peek(lexer) == ')') --paren;
                    if(paren == 0)
                    {
                        push_index(lexer , 1);
                        break;
                    }
                    push_index(lexer , 1);
                }
                if(paren)
                {
                    fprintf(stderr , "not expect ))\n");
                    return (Token){TOKEN_ERR , NULL , 0};
                }
                continue;
            }

            if(c == '$' && lexer->input[lexer->index+1] == '(')
            {
                int paren = 1;
                push_index(lexer , 2);
                while(!match_end(lexer) && !match_nl(lexer))
                {
                    if(peek(lexer) == '(') ++paren;
                    else if(peek(lexer) == ')') --paren;
                    if(paren == 0)
                    {
                        push_index(lexer , 1);
                        break;
                    }
                    push_index(lexer , 1);
                }
                if(paren)
                {
                    fprintf(stderr , "not expect )\n");
                    return (Token){TOKEN_ERR , NULL , 0};
                }
                continue;
            }

            if(c == '$')
            {
                char* str = lexer->input;

                if(str[lexer->index+1]=='?'|| str[lexer->index+1]=='$'|| str[lexer->index+1]=='!')
                {
                    push_index(lexer , 2);
                    continue;
                }

                if((str[lexer->index+1] < 'A' || (str[lexer->index+1] > 'Z' && str[lexer->index+1] < 'a') || str[lexer->index+1] > 'z') && str[lexer->index+1]!='_')
                {
                    fprintf(stderr , "unknown char near $");
                    return (Token){TOKEN_ERR , NULL , 0};
                }
                for(size_t i = 2 ; str[lexer->index + i] != '\0' && !match_oper_char(str[lexer->index + i]) && (str[lexer->index + i] == ' ' || str[lexer->index + i] == '\t') && str[lexer->index + i] != '\"' && str[lexer->index + i] !='\'' && str[lexer->index + i] != '='; ++i)
                {
                    char tmp = str[lexer->index + i];
                    if((tmp < 'A' || (tmp > 'Z' && tmp<'a') || tmp>'z') && (tmp<'0'|| tmp>'9') && tmp!='_')
                    {
                        fprintf(stderr , "unknown char near $");
                        return (Token){TOKEN_ERR , NULL , 0};
                    }
                }
                push_index(lexer , 1);
                continue;
            }

            //may be extract "\"
            push_index(lexer , 1);
        }
    }
    if(_state != 0)
    {
        return (Token){TOKEN_ERR , NULL , 0};
    }
    
    char* word = strndup(lexer->input + start, lexer->index - start);
    return (Token){TOKEN_WORD , word , flag}; 
}

static void skip(Lexer* lexer)
{
    while(!match_nl(lexer) && !match_end(lexer)) push_index(lexer , 1);
}

static void push_index(Lexer* lexer , size_t n)
{
    lexer->index += n;
}

static char peek (Lexer* lexer)
{
    return lexer->input[lexer->index];
}

static bool match(Lexer* lexer , char c)
{
    return peek(lexer) == c;
}

static bool match_oper_char(char c)
{
    char opers[] = {'&','>','<','|',';','(',')','\n','\0'};
    bool tmp = false;
    for(size_t i = 0 ; opers[i] != '\0' ; ++i)
    {
        if(c == opers[i]) tmp = true;
    }
    return tmp;
}

static bool match_space(Lexer* lexer)
{
    return isspace(peek(lexer));
}

static bool match_nl(Lexer* lexer)
{
    return peek(lexer) == '\n';
}

static bool match_end(Lexer* lexer)
{
    return peek(lexer) == '\0';
}