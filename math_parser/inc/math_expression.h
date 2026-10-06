#pragma once

#include "math_ast.h"

typedef struct {
	const char *name;
	double value;
} Variable;

typedef struct {
	const char *name;
	double (*func)(double*, size_t);
} Function;

M_ASTNode *create_expression(const char *);
void delete_expression(M_ASTNode *);
void print_expression(M_ASTNode *);
double evaluate_expression(M_ASTNode *, Variable *, size_t, Function *, size_t);