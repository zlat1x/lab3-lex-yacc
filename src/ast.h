
#ifndef MINIC_AST_H
#define MINIC_AST_H

typedef enum {
    AST_NUMBER,
    AST_VARIABLE,
    AST_BINARY,
    AST_UNARY,
    AST_DECL,
    AST_ASSIGN,
    AST_PRINT,
    AST_RETURN,
    AST_BLOCK,
    AST_IF,
    AST_WHILE,
    AST_PROGRAM
} AstKind;

typedef struct Ast Ast;
typedef struct AstList AstList;

struct AstList {
    Ast *node;
    AstList *next;
};

struct Ast {
    AstKind kind;
    long long number;
    char *name;
    const char *op;
    Ast *left;
    Ast *right;
    Ast *third;
    AstList *items;
};

Ast *ast_number(long long value);
Ast *ast_variable(char *name);
Ast *ast_binary(const char *op, Ast *lhs, Ast *rhs);
Ast *ast_unary(const char *op, Ast *operand);

Ast *ast_decl(char *name, Ast *initial);
Ast *ast_assign(char *name, Ast *value);
Ast *ast_print(Ast *value, int newline);
Ast *ast_return(Ast *value);

Ast *ast_block(void);
void ast_append(Ast *block, Ast *stmt);
Ast *ast_if(Ast *condition, Ast *then_branch, Ast *else_branch);
Ast *ast_while(Ast *condition, Ast *body);
Ast *ast_program(Ast *body);

void ast_free(Ast *node);
const char *ast_kind_name(AstKind kind);

#endif
