#pragma once

#include <stddef.h>

typedef enum {
	M_NODE_BINARY,
	M_NODE_UNARY,
	M_NODE_FUNC,
	M_NODE_GROUP,
	M_NODE_VAR,
	M_NODE_NUM,
} MathNodeType;

typedef struct M_ASTNode {
	MathNodeType type;

	union {
		struct {
			char *op;
			struct M_ASTNode* left;
			struct M_ASTNode* right;
		} binary; // NODE_BINARY

		struct {
			char *op;
			struct M_ASTNode* base;
		} unary; // NODE_UNARY

		struct {
			char* name;
			size_t argc;
			struct M_ASTNode** args;
		} func; // NODE_FUNC

		struct M_ASTNode* group; // NODE_GROUP

		char *var; // NODE_VAR

		double num; // NODE_NUM
	};

} M_ASTNode;

M_ASTNode* math_create_node(MathNodeType);
M_ASTNode* math_create_binary(const char *, M_ASTNode*, M_ASTNode*);
M_ASTNode* math_create_unary(const char *, M_ASTNode*);
M_ASTNode* math_create_func(const char*, size_t, M_ASTNode**);
M_ASTNode* math_create_group(M_ASTNode*);
M_ASTNode* math_create_var(const char*);
M_ASTNode* math_create_number(const char*);

void math_delete_ast(M_ASTNode*);

