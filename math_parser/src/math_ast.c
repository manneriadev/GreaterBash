#include "math_ast.h"

#include <stdlib.h>
#include <string.h>

M_ASTNode* math_create_node(MathNodeType type) {
	M_ASTNode* node = (M_ASTNode*) malloc(sizeof(M_ASTNode));
	node->type = type;
	return node;
}

M_ASTNode* math_create_binary(const char *op, M_ASTNode *left, M_ASTNode *right) {
	M_ASTNode* node = math_create_node(M_NODE_BINARY);
	node->binary.op = strdup(op);
	node->binary.left = left;
	node->binary.right = right;
	return node;
}

M_ASTNode* math_create_unary(const char *op, M_ASTNode *base) {
	M_ASTNode* node = math_create_node(M_NODE_UNARY);
	node->unary.op = strdup(op);
	node->unary.base = base;
	return node;
}

M_ASTNode* math_create_func(const char *name, size_t argc, M_ASTNode **args) {
	M_ASTNode* node = math_create_node(M_NODE_FUNC);
	node->func.name = strdup(name);
	node->func.argc = argc;
	node->func.args = args;
	return node;
}

M_ASTNode* math_create_group(M_ASTNode *group) {
	M_ASTNode* node = math_create_node(M_NODE_GROUP);
	node->group = group;
	return node;
}

M_ASTNode* math_create_var(const char *name) {
	M_ASTNode* node = math_create_node(M_NODE_VAR);
	node->var = strdup(name);
	return node;
}

M_ASTNode* math_create_number(const char *value) {
	M_ASTNode* node = math_create_node(M_NODE_NUM);
	node->num = atof(value);
	return node;
}

void math_delete_ast(M_ASTNode *root) {
	if (root == NULL) return;
	switch (root->type) {
		case M_NODE_BINARY:
			math_delete_ast(root->binary.left);
			math_delete_ast(root->binary.right);
			free(root->binary.op);
			break;
		case M_NODE_UNARY:
			math_delete_ast(root->unary.base);
			free(root->unary.op);
			break;
		case M_NODE_FUNC:
			for (size_t i = 0; i < root->func.argc; ++i) {
				math_delete_ast(root->func.args[i]);
			}
			free(root->func.name);
			free(root->func.args);
			break;
		case M_NODE_GROUP:
			math_delete_ast(root->group);
			break;
		case M_NODE_VAR:
			free(root->var);
			break;
		case M_NODE_NUM:
			break;
	}
	free(root);
}