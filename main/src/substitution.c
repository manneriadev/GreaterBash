#include "substitution.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

char* sub(const char* input , hash_table* table);
Variable* take_variable(hash_table* table , size_t* var_count);
static bool is_varname_char(char ch);

extern void process_table(ASTNode* root);

// 2->32->16->word split that expand from $->4,8->1.

hash_table* table = NULL;

extern int last_exit_code;
extern pid_t last_bg_pid;

char* sub(const char* input , hash_table* table)
{
    if(!input || !table) return NULL;

    size_t len = strlen(input);
    size_t capacity = len*2 + 128;

    char* output = malloc(sizeof(char) * capacity);
    if(!output)
    {
        fprintf(stderr , "malloc is not init mem(malloc in substitution for new buf - expand)\n");
        return NULL;
    }

    int mode = 0;
    size_t pos = 0;
    size_t i = 0;
    while(i < len)
    {
        if(pos >= capacity - 2)
        {
            capacity += 128;
            output = realloc(output , sizeof(char) * capacity);
            if(!output)
            {
                free(output);
                return NULL;
            }
        }

        if(mode == 1)
        {
            if(input[i] == '\'')
            {
                mode = 0;
                ++i;
                continue;
            }
            output[pos++] = input[i++];
            continue;
        }

        if(mode == 2)
        {
            if(input[i] == '"')
            {
                mode = 0;
                ++i;
                continue;
            }
        
            if(input[i] == '\\' && i + 1 < len)
            {
                char t = input[i+1];
                if(t=='\\'||t=='"'||t=='$')
                {
                    output[pos++] = t;
                    i += 2;
                    continue;
                }
                output[pos++] = input[i++];
                continue;
            }

            if(i + 2 < len && input[i] == '$' && input[i+1] == '(' && input[i+2] == '(')
            {
                i+=3;
                size_t expr_start = i;
                int paren_count = 2;

                while(i < len && paren_count > 0)
                {
                    if(input[i] == ')') --paren_count;
                    else if(input[i] == '(') ++paren_count;
                    ++i;
                }
                
                if(paren_count != 0)
                {
                    fprintf(stderr , "unbalansed () in math expr\n");
                    free(output);
                    return NULL;
                }

                size_t epxr_len = i - expr_start - 2;
                char* expr = strndup(input + expr_start , epxr_len);
                if(!expr)
                {
                    fprintf(stderr, "not expect expr name(in substitution)");
                    free(input);
                    return NULL;
                }

                char* expanded_expr = sub(expr , table);
                free(expr);
                if(!expanded_expr)
                {
                    fprintf(stderr , "unvalid expand $(())\n");
                    free(output);
                    return NULL;
                }

                M_ASTNode* ast = create_expression(expanded_expr);
                free(expanded_expr);
                if(ast)
                {
                    size_t var_count = 0;
                    Variable* vars = take_variable(table , &var_count);
                    double resd = evaluate_expression(ast , vars , var_count , NULL , 0);
                    
                    if(vars)
                    {
                        for(size_t j = 0 ; j < var_count ; ++j)
                        {
                            free(vars[j].name);
                        }
                        free(vars);
                    }

                    delete_expression(ast);

                    char result[128];
                    snprintf(result , sizeof(result), "%.4g" , resd);
                    size_t elen = strlen(result);

                    if(pos+elen >= capacity)
                    {
                        capacity = pos + elen + 128;
                        output = realloc(output , sizeof(char) * capacity);
                        if(!output)
                        {
                            free(output);
                            return NULL;
                        }
                    }
                    memcpy(output+pos , result , elen);
                    pos += elen;
                }
                else
                {
                    output[pos++] = '0';
                }
                continue;
            }

            if(i + 1 < len && input[i] == '$' && input[i+1] == '(')
            {
                
            }

            if(i + 1 < len && input[i] == '$' && (input[i+1] == '$' || input[i+1] == '?' || input[i+1] == '!'))
            {
                char tmp[32];
                snprintf(tmp , sizeof(tmp) , "%d" , (input[i+1] == '$') ? (int)getpid() : (input[i+1] == '?') ? last_exit_code : last_bg_pid);
                size_t tmplen = strlen(tmp);

                if(pos + tmplen >= capacity)
                {
                    capacity = pos + tmplen + 128;
                    output = realloc(output , sizeof(char) * capacity);
                    if(!output)
                    {
                        fprintf(stderr , "malloc is not init mem(in substitution\n)");
                        free(output);
                        return NULL;
                    }
                }

                memcpy(output+pos , tmp , tmplen);
                pos += tmplen;
                i+=2;
                continue;
            }
                
            if(i + 1 < len && input[i] == '$' && is_varname_char(input[i+1]))
            {
                ++i;
                size_t var_start = i;
                while(i < len && is_varname_char(input[i])) ++i;
                
                size_t var_len = i - var_start;
                char* var_name = strndup(input + var_start , var_len);

                if(var_name)
                {
                    char* value = table_get(table , var_name);
                    if(!value) value = getenv(var_name);
                    if(value)
                    {
                        size_t vlen = strlen(value);
                        if(pos+vlen >= capacity)
                        {
                            capacity = pos + vlen + 128;
                            output = realloc(output , sizeof(char) * capacity);
                            if(!output)
                            {
                                free(output);
                                free(var_name);
                                return NULL;
                            }
                        }
                        memcpy(output+pos , value , vlen);
                        pos += vlen;
                    }
                    free(var_name);
                }
                continue;
            }

            output[pos++] = input[i++];
            continue;
        }

        // mode = 0

        if(input[i] == '\\' && i + 1 < len)
        {
            output[pos++] = input[i + 1];
            i += 2;
            continue;
        }

        if(input[i] == '\'')
        {
            mode = 1;
            ++i;
            continue;
        }   

        if(input[i] == '"')
        {
            mode = 2;
            ++i;
            continue;
        }

        if(i + 2 < len && input[i] == '$' && input[i+1] == '(' && input[i+2] == '(')
        {
            i+=3;
            size_t expr_start = i;
            int paren_count = 2;

            while(i < len && paren_count > 0)
            {
                if(input[i] == ')') --paren_count;
                else if(input[i] == '(') ++paren_count;
                ++i;
            }
            
            if(paren_count != 0)
            {
                fprintf(stderr , "unbalansed () in math expr\n");
                free(output);
                return NULL;
            }

            size_t epxr_len = i - expr_start - 2;
            char* expr = strndup(input + expr_start , epxr_len);
            if(!expr)
            {
                fprintf(stderr, "not expect expr name(in substitution)");
                free(input);
                return NULL;
            }

            char* expanded_expr = sub(expr , table);
            free(expr);
            if(!expanded_expr)
            {
                fprintf(stderr , "unvalid expand $(())\n");
                free(output);
                return NULL;
            }

            M_ASTNode* ast = create_expression(expanded_expr);
            free(expanded_expr);
            if(ast)
            {
                size_t var_count = 0;
                Variable* vars = take_variable(table , &var_count);
                double resd = evaluate_expression(ast , vars , var_count , NULL , 0);
                
                if(vars)
                {
                    for(size_t j = 0 ; j < var_count ; ++j)
                    {
                        free(vars[j].name);
                    }
                    free(vars);
                }

                delete_expression(ast);

                char result[128];
                snprintf(result , sizeof(result), "%.4g" , resd);
                size_t elen = strlen(result);

                if(pos+elen >= capacity)
                {
                    capacity = pos + elen + 128;
                    output = realloc(output , sizeof(char) * capacity);
                    if(!output)
                    {
                        free(output);
                        return NULL;
                    }
                }
                memcpy(output+pos , result , elen);
                pos += elen;
            }
            else
            {
                output[pos++] = '0';
            }
            continue;
        }

        if(i + 1 < len && input[i] == '$' && input[i+1] == '(')
        {
            
        }

        if(i + 1 < len && input[i] == '$' && (input[i+1] == '$' || input[i+1] == '?' || input[i+1] == '!'))
        {
            char tmp[32];
            snprintf(tmp , sizeof(tmp) , "%d" , (input[i+1] == '$') ? (int)getpid() : (input[i+1] == '?') ? last_exit_code : last_bg_pid);
            size_t tmplen = strlen(tmp);

            if(pos + tmplen >= capacity)
            {
                capacity = pos + tmplen + 128;
                output = realloc(output , sizeof(char) * capacity);
                if(!output)
                {
                    fprintf(stderr , "malloc is not init mem(in substitution\n)");
                    free(output);
                    return NULL;
                }
            }

            memcpy(output+pos , tmp , tmplen);
            pos += tmplen;
            i+=2;
            continue;
        }

        if(i + 1 < len && input[i] == '$' && is_varname_char(input[i+1]))
        {
            ++i;

            size_t var_start = i;
            while(i < len && is_varname_char(input[i])) ++i;
            
            size_t var_len = i - var_start;
            char* var_name = strndup(input + var_start , var_len);

            if(var_name)
            {
                char* value = table_get(table , var_name);
                if(!value) value = getenv(var_name);
                if(value)
                {
                    size_t vlen = strlen(value);
                    if(pos+vlen >= capacity)
                    {
                        capacity = pos + vlen + 128;
                        output = realloc(output , sizeof(char) * capacity);
                        if(!output)
                        {
                            free(output);
                            free(var_name);
                            return NULL;
                        }
                    }
                    memcpy(output+pos , value , vlen);
                    pos += vlen;
                }
                free(var_name);
            }
            continue;
        }

        output[pos++] = input[i++];
    }
    output[pos] = '\0';
    return output;
}       

Variable* take_variable(hash_table* table , size_t* var_count)
{
    if(!table || !var_count) return NULL;

    *var_count = 0;

    size_t capacity = 32;
    Variable* vars = malloc(sizeof(Variable)*capacity);
    if(!vars)
    {
        fprintf(stderr , "malloc is not init mem(this try in substitution - variable array)\n");
        return NULL;
    }

    size_t index = 0;
    for(size_t i = 0 ; i < HASH_TABLE_SIZE ; ++i)
    {
        node_tt* tmp = table->buckets[i];
        while(tmp)
        {
            if(index >= capacity)
            {
                capacity += 32;
                vars = realloc(vars , sizeof(Variable)*capacity);
                if(!vars)
                {
                    fprintf(stderr , "malloc is not init mem(this try in substitution - variable array)\n");
                    free(vars);
                    return NULL;
                }
            }

            Variable vr;
            vr.name = strdup(tmp->key);
            if(!vr.name)
            {
                for(size_t j = 0 ; j < index ; ++j) free(vars[j].name);
            }
            vr.value = atof(tmp->value);
            vars[index++] = vr;
            tmp = tmp->next;
        }
    }
    *var_count = index;
    return vars;
}

void expand(ASTNode* root)
{
    if(!root) return;

    switch(root->type)
    {
        case NODE_BINARY:
            expand(root->binary.left);
            expand(root->binary.right);
            break;
        case NODE_AND_GT:
        case NODE_AND_GT_GT:
        case NODE_GT:
        case NODE_GT_GT:
        case NODE_LT:
            expand(root->unary.base);
            break;
        case NODE_GROUP:    
            hash_table* prev = table;
            hash_table* new = copy_table(prev);
            table = new;
            process_table(root->unary.base);
            expand(root->unary.base);
            table = prev;
            table_free(new);
            break;
        case NODE_CMD:
            if(root->cmd.redirs)
            {
                for(size_t i = 0 ; root->cmd.redirs[i] ; ++i) expand(root->cmd.redirs[i]);
            }
            if(root->cmd.args)
            {
                for(size_t i = 0 ; root->cmd.args[i] ; ++i) expand(root->cmd.args[i]);
            }
            break;
        case NODE_ARG:
            if(root->string.name)
            {
                char* expanded = sub(root->string.name , table);
                if(expanded)
                {
                    free(root->string.name);
                    root->string.name = expanded;
                }
            }
            break;
        default: break;
    }
}

static bool is_varname_char(char ch)
{
    return isalnum(ch) || ch == '_';
}