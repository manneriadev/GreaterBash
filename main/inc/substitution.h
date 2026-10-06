#pragma once
#include "ast.h"
#include "execute.h"
#include "tokens.h"
#include "hash_table.h"
#include "math_expression.h"

void expand(ASTNode* root);
char* sub(const char* input , hash_table* table);


//firstable i should axpand $(()) -> $() -> $var -> word split ->
// -> assignment -> all string