#include "math_token.h"
#include "math_ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define DEFAULT_ARGS 4
#define MAX 1000

// Declaration of the parser

typedef struct {
	const Token *tokens;
	size_t index;
	size_t tokens_count;
} Parser;

static M_ASTNode *parse_add(Parser *);
static M_ASTNode *parse_mul(Parser *);
static M_ASTNode *parse_pow(Parser *);
static M_ASTNode *parse_unary(Parser *);
static M_ASTNode *parse_primary(Parser *);
static M_ASTNode *parse_func(Parser *);
static M_ASTNode *parse_group(Parser *);

static void advance(Parser *);
static const Token *current(Parser *);
static const Token *previous(Parser *);
static bool match(Parser *, MathTokenType);
static bool match_type(Parser* parser , MathTokenType type);

// Implementation of the parser

M_ASTNode *math_parse(const Token *tokens) 
{
	if(!tokens) return NULL;

	size_t count = 0;
	while(tokens[count].type != TOKEN_END && count < MAX) ++count;

	Parser parser = {tokens, 0 , count};
	return parse_add(&parser);
}

static M_ASTNode *parse_add(Parser *parser) 
{
	if(!parser) return NULL;
	M_ASTNode *left = parse_mul(parser);
	if(!left) return NULL;

	while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) 
	{
		const Token* op_token = previous(parser);
		if(!op_token || !op_token->value) return left;
		
		M_ASTNode *right = parse_mul(parser);
		if(!right) return left;

		M_ASTNode* base = math_create_binary(op_token->value, left, right);
		if(!base)
		{
			math_delete_ast(left);
			math_delete_ast(right);
			return NULL;
		}
		left = base;
	}
	return left;
}

static M_ASTNode *parse_mul(Parser *parser) 
{
	if(!parser) return NULL;
	M_ASTNode *left = parse_pow(parser);
	if(!left) return NULL;
	while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH)) 
	{
		const Token *op_token = previous(parser);
		if(!op_token || !op_token->value) return left;

		M_ASTNode *right = parse_pow(parser);
		if(!right) return left;
		
		M_ASTNode* base = math_create_binary(op_token->value, left, right);
		if(!base)
		{
			math_delete_ast(left);
			math_delete_ast(right);
			return NULL;
		}
		left = base;
	}
	return left;
}

static M_ASTNode *parse_pow(Parser *parser) 
{
	if(!parser) return NULL;
	M_ASTNode *left = parse_unary(parser);
	if(!left) return NULL;
	
	if (match(parser, TOKEN_CARET)) 
	{
		M_ASTNode *right = parse_pow(parser);
		if(!right) return left;

		M_ASTNode* base = math_create_binary("^", left, right);
		if(!base)
		{
			math_delete_ast(left);
			math_delete_ast(right);
			return NULL;
		}
		return base;
	}
	return left;
}

static M_ASTNode *parse_unary(Parser *parser) 
{
	if(!parser) return NULL;

	if (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) 
	{
		const Token* op_token = previous(parser);
		if(!op_token || !op_token->value) return NULL;

		M_ASTNode *right = parse_unary(parser);
		if(!right) return NULL;
		M_ASTNode* base = math_create_unary(op_token->value, right);
		if(!base)
		{
			math_delete_ast(base);
			return NULL;
		}
		return base;
	}
	return parse_primary(parser);
}

static M_ASTNode *parse_primary(Parser *parser) 
{
	if(!parser) return NULL;
	const Token* token = current(parser);
	if(!token) return NULL;

	if (match(parser, TOKEN_ID))
		return parse_func(parser);

	if (match(parser, TOKEN_LPAREN))
		return parse_group(parser);

	if (match(parser, TOKEN_NUM))
	{
		const Token* num_token = previous(parser);
		if(!num_token || !num_token->value) return NULL;
		return math_create_number(num_token->value);
	}

	fprintf(stderr, "Unexpected token: %s\n", current(parser)->value);
	return NULL;
}

static M_ASTNode *parse_func(Parser *parser) 
{
	if(!parser) return NULL;
	const Token* name_token = previous(parser);
	if(!name_token || !name_token->value) return NULL;

	if (match(parser, TOKEN_LPAREN)) 
	{
		size_t capacity = DEFAULT_ARGS, i = 0;
		M_ASTNode **args = (M_ASTNode **) malloc(sizeof(M_ASTNode *) * capacity);
		if(!args)
		{
			fprintf(stderr , "malloc is not init memory(malloc in math_parser)\n");
			return NULL;
		}

		if (!match_type(parser, TOKEN_RPAREN)) 
		{
			do 
			{
				if (i == capacity) 
				{
					capacity += DEFAULT_ARGS;
					M_ASTNode** new = (M_ASTNode **) realloc(args, sizeof(M_ASTNode *) * capacity);
					if(!new)
					{
						free(args);
						return NULL;
					}
					args = new;
				}

				M_ASTNode* arg = parse_add(parser);
				if(!arg)
				{
					for(size_t j = 0 ; j < i ; ++j)
					{
						math_delete_ast(args[j]);
					}
					free(args);
					return NULL;
				}
				args[i++] = arg;
			} while (match(parser, TOKEN_COMMA));
			if(!match(parser , TOKEN_RPAREN))
			{
				for(size_t j = 0 ; j < i ; ++j)
				{
					math_delete_ast(args[j]);
				}
				free(args);
				return NULL;
			}
		} 
		M_ASTNode* base = math_create_func(name_token->value, i, args);
		if(!base)
		{
			for(size_t j = 0 ; j < i ; ++j)
			{
				math_delete_ast(args[j]);
			}
			free(args);
			return NULL;
		}
		return base;
	}
	return math_create_var(name_token->value);
}

static M_ASTNode *parse_group(Parser *parser) 
{
	if(!parser) return NULL;
	M_ASTNode *group = parse_add(parser);
	if(!group) return NULL;

	if(!match(parser , TOKEN_RPAREN))
	{
		math_delete_ast(group);
		return NULL;
	}
	return group;
}

static void advance(Parser *parser) 
{
	if(parser) ++parser->index;
}

static const Token *current(Parser *parser) 
{
	if(!parser) return NULL;
	if(parser->index >= parser->tokens_count)
	{
		static const Token end_token = {TOKEN_END , NULL};
		return &end_token;
	}
	return &parser->tokens[parser->index];
}

static const Token *previous(Parser *parser) 
{
	if(!parser || parser->index == 0)
	{
		fprintf(stderr , "no previous token\n");
		return NULL;
	}
	return &parser->tokens[parser->index - 1];
}

static bool match(Parser *parser, MathTokenType type) 
{
	if(!parser) return false;
	if (current(parser)->type == type) {
		advance(parser);
		return true;
	}
	return false;
}

static bool match_type(Parser* parser , MathTokenType type)
{
	if(!parser) return false;
	return current(parser)->type == type;
}