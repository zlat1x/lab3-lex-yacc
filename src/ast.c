
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

static void *checked_calloc(size_t count, size_t size)
{
    void *memory = calloc(count, size);
    if (memory == NULL) {
        fprintf(stderr, "Error: not enough memory\n");
        exit(EXIT_FAILURE);
    }
    return memory;
}

static Ast *make_node(AstKind kind, Ast *left, Ast *right, Ast *third)
{
    Ast *node = checked_calloc(1, sizeof(*node));
    node->kind = kind;
    node->left = left;
    node->right = right;
    node->third = third;
    return node;
}

Ast *ast_number(long long value)
{
    Ast *node = make_node(AST_NUMBER, NULL, NULL, NULL);
    node->number = value;
    return node;
}

Ast *ast_variable(char *name)
{
    Ast *node = make_node(AST_VARIABLE, NULL, NULL, NULL);
    node->name = name;
    return node;
}

Ast *ast_binary(const char *op, Ast *lhs, Ast *rhs)
{
    Ast *node = make_node(AST_BINARY, lhs, rhs, NULL);
    node->op = op;
    return node;
}

Ast *ast_unary(const char *op, Ast *operand)
{
    Ast *node = make_node(AST_UNARY, operand, NULL, NULL);
    node->op = op;
    return node;
}

Ast *ast_decl(char *name, Ast *initial)
{
    Ast *node = make_node(AST_DECL, initial, NULL, NULL);
    node->name = name;
    return node;
}

Ast *ast_assign(char *name, Ast *value)
{
    Ast *node = make_node(AST_ASSIGN, value, NULL, NULL);
    node->name = name;
    return node;
}

Ast *ast_print(Ast *value, int newline)
{
    Ast *node = make_node(AST_PRINT, value, NULL, NULL);
    node->number = newline;
    return node;
}

Ast *ast_return(Ast *value)
{
    return make_node(AST_RETURN, value, NULL, NULL);
}

Ast *ast_block(void)
{
    return make_node(AST_BLOCK, NULL, NULL, NULL);
}

void ast_append(Ast *block, Ast *statement)
{
    AstList **position = &block->items;

    while (*position != NULL) {
        position = &(*position)->next;
    }

    *position = checked_calloc(1, sizeof(**position));
    (*position)->node = statement;
}

Ast *ast_if(Ast *condition, Ast *then_branch, Ast *else_branch)
{
    return make_node(AST_IF, condition, then_branch, else_branch);
}

Ast *ast_while(Ast *condition, Ast *body)
{
    return make_node(AST_WHILE, condition, body, NULL);
}

Ast *ast_program(Ast *body)
{
    return make_node(AST_PROGRAM, body, NULL, NULL);
}

void ast_free(Ast *node)
{
    if (node == NULL) {
        return;
    }

    ast_free(node->left);
    ast_free(node->right);
    ast_free(node->third);

    AstList *item = node->items;

    while (item != NULL) {
        AstList *next = item->next;
        ast_free(item->node);
        free(item);
        item = next;
    }

    free(node->name);
    free(node);
}

const char *ast_kind_name(AstKind kind)
{
    switch (kind) {
        case AST_NUMBER: return "number";
        case AST_VARIABLE: return "variable";
        case AST_BINARY: return "binary";
        case AST_UNARY: return "unary";
        case AST_DECL: return "declaration";
        case AST_ASSIGN: return "assignment";
        case AST_PRINT: return "printf";
        case AST_RETURN: return "return";
        case AST_BLOCK: return "block";
        case AST_IF: return "if";
        case AST_WHILE: return "while";
        case AST_PROGRAM: return "program";
    }

    return "unknown";
}
