
#include "runtime.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STEPS 1000000

typedef struct Var {
    const char *name;
    long long value;
    int depth;
    struct Var *next;
} Var;

typedef struct {
    Var *vars;
    int depth;
    int error;
    int returned;
    long long return_value;
    long long steps;
} Runtime;

static void fail(Runtime *rt, const char *message, const char *name)
{
    if (rt->error) {
        return;
    }

    fprintf(stderr, "Runtime error: %s", message);

    if (name != NULL) {
        fprintf(stderr, " '%s'", name);
    }

    fputc('\n', stderr);
    rt->error = 1;
}

static Var *lookup(Runtime *rt, const char *name)
{
    for (Var *var = rt->vars; var != NULL; var = var->next) {
        if (strcmp(var->name, name) == 0) {
            return var;
        }
    }

    return NULL;
}

static void push_scope(Runtime *rt)
{
    rt->depth++;
}

static void pop_scope(Runtime *rt)
{
    while (rt->vars != NULL && rt->vars->depth == rt->depth) {
        Var *next = rt->vars->next;
        free(rt->vars);
        rt->vars = next;
    }

    rt->depth--;
}

static void add_variable(Runtime *rt, const char *name, long long value)
{
    for (Var *var = rt->vars;
         var != NULL && var->depth == rt->depth;
         var = var->next) {
        if (strcmp(var->name, name) == 0) {
            fail(rt, "duplicate declaration of", name);
            return;
        }
    }

    Var *var = calloc(1, sizeof(*var));

    if (var == NULL) {
        fail(rt, "out of memory", NULL);
        return;
    }

    var->name = name;
    var->value = value;
    var->depth = rt->depth;
    var->next = rt->vars;
    rt->vars = var;
}

static long long eval(const Ast *node, Runtime *rt)
{
    if (node == NULL || rt->error) {
        return 0;
    }

    if (node->kind == AST_NUMBER) {
        return node->number;
    }

    if (node->kind == AST_VARIABLE) {
        Var *var = lookup(rt, node->name);

        if (var == NULL) {
            fail(rt, "undeclared variable", node->name);
            return 0;
        }

        return var->value;
    }

    if (node->kind == AST_UNARY) {
        long long value = eval(node->left, rt);

        if (rt->error) {
            return 0;
        }

        if (strcmp(node->op, "!") == 0) {
            return !value;
        }

        if (value == LLONG_MIN) {
            fail(rt, "integer overflow", NULL);
            return 0;
        }

        return -value;
    }

    if (node->kind == AST_BINARY) {
        long long a = eval(node->left, rt);

        if (rt->error) {
            return 0;
        }

        if (strcmp(node->op, "&&") == 0) {
            return a ? (eval(node->right, rt) != 0) : 0;
        }

        if (strcmp(node->op, "||") == 0) {
            return a ? 1 : (eval(node->right, rt) != 0);
        }

        long long b = eval(node->right, rt);

        if (rt->error) {
            return 0;
        }

        long long result;

        if (strcmp(node->op, "+") == 0) {
            if (__builtin_add_overflow(a, b, &result)) {
                fail(rt, "integer overflow", NULL);
                return 0;
            }
            return result;
        }

        if (strcmp(node->op, "-") == 0) {
            if (__builtin_sub_overflow(a, b, &result)) {
                fail(rt, "integer overflow", NULL);
                return 0;
            }
            return result;
        }

        if (strcmp(node->op, "*") == 0) {
            if (__builtin_mul_overflow(a, b, &result)) {
                fail(rt, "integer overflow", NULL);
                return 0;
            }
            return result;
        }

        if (strcmp(node->op, "/") == 0 ||
            strcmp(node->op, "%") == 0) {
            if (b == 0) {
                fail(rt, "division by zero", NULL);
                return 0;
            }

            if (a == LLONG_MIN && b == -1) {
                fail(rt, "integer overflow", NULL);
                return 0;
            }

            return strcmp(node->op, "/") == 0 ? a / b : a % b;
        }

        if (strcmp(node->op, "==") == 0) return a == b;
        if (strcmp(node->op, "!=") == 0) return a != b;
        if (strcmp(node->op, "<") == 0) return a < b;
        if (strcmp(node->op, ">") == 0) return a > b;
        if (strcmp(node->op, "<=") == 0) return a <= b;
        if (strcmp(node->op, ">=") == 0) return a >= b;
    }

    fail(rt, "invalid expression node", NULL);
    return 0;
}

static void execute(const Ast *node, Runtime *rt)
{
    if (node == NULL || rt->error || rt->returned) {
        return;
    }

    if (++rt->steps > MAX_STEPS) {
        fail(rt, "execution step limit exceeded", NULL);
        return;
    }

    switch (node->kind) {
        case AST_PROGRAM:
            execute(node->left, rt);
            break;

        case AST_BLOCK:
            push_scope(rt);

            for (AstList *item = node->items;
                 item != NULL && !rt->error && !rt->returned;
                 item = item->next) {
                execute(item->node, rt);
            }

            pop_scope(rt);
            break;

        case AST_DECL: {
            long long value = node->left
                ? eval(node->left, rt) : 0;

            if (!rt->error) {
                add_variable(rt, node->name, value);
            }
            break;
        }

        case AST_ASSIGN: {
            Var *var = lookup(rt, node->name);

            if (var == NULL) {
                fail(rt, "undeclared variable", node->name);
                break;
            }

            long long value = eval(node->left, rt);

            if (!rt->error) {
                var->value = value;
            }
            break;
        }

        case AST_PRINT: {
            long long value = eval(node->left, rt);

            if (!rt->error) {
                printf(node->number ? "%lld\n" : "%lld", value);
            }
            break;
        }

        case AST_RETURN:
            rt->return_value = eval(node->left, rt);

            if (!rt->error) {
                rt->returned = 1;
            }
            break;

        case AST_IF: {
            long long condition = eval(node->left, rt);

            if (!rt->error) {
                execute(condition ? node->right : node->third, rt);
            }
            break;
        }

        case AST_WHILE:
            while (!rt->error && !rt->returned) {
                long long condition = eval(node->left, rt);

                if (rt->error || !condition) {
                    break;
                }

                execute(node->right, rt);
            }
            break;

        default:
            fail(rt, "invalid statement node", NULL);
    }
}

int run_program(const Ast *root)
{
    Runtime rt = {0};

    execute(root, &rt);

    while (rt.vars != NULL) {
        Var *next = rt.vars->next;
        free(rt.vars);
        rt.vars = next;
    }

    return rt.error ? 1 : 0;
}
