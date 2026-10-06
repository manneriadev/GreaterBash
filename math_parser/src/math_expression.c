#include "math_expression.h"

#include "math_token.h"

#include <stdio.h>
#include <stdlib.h>

// Parsing related functions

Token *math_tokenize(const char*);
M_ASTNode *math_parse(const Token*);

// Implementation of the expression.h functions

M_ASTNode *create_expression(const char *input) 
{
	if(!input)
	{
		fprintf(stderr, "input string is NULL(in math_expr)\n");
		return NULL;
	}
	Token *tokens = math_tokenize(input);
	if(!tokens)
	{
		fprintf(stderr, "tokens array is NULL(in math_expr)\n");
		return NULL;
	}

	M_ASTNode *root = math_parse(tokens);
	for (size_t i = 0; tokens[i].type != TOKEN_END; ++i) {
		free(tokens[i].value);
	}
	free(tokens);
	return root;
}

void delete_expression(M_ASTNode *root) {
	math_delete_ast(root);
}

// Print the expression in infix notation

static void print_expression_helper(M_ASTNode *);

void print_expression(M_ASTNode *root) {
	if(!root) return;
	print_expression_helper(root);
	printf("\n");
}

static void print_expression_helper(M_ASTNode *root) {
	if (root == NULL) return;
	switch (root->type) {
		case M_NODE_BINARY:
			print_expression_helper(root->binary.left);
			printf("%s", root->binary.op);
			print_expression_helper(root->binary.right);
			break;
		case M_NODE_UNARY:
			printf("%s", root->unary.op);
			print_expression_helper(root->unary.base);
			break;
		case M_NODE_FUNC:
			printf("%s(", root->func.name);
			for (size_t i = 0; i < root->func.argc; ++i) {
				print_expression_helper(root->func.args[i]);
				if (i < root->func.argc - 1) {
					printf(", ");
				}
			}
			printf(")");
			break;
		case M_NODE_GROUP:
			printf("(");
			print_expression_helper(root->group);
			printf(")");
			break;
		case M_NODE_VAR:
			printf("%s", root->var);
			break;
		default: // NODE_NUM
			printf("%g", root->num);
			break;
	}
}