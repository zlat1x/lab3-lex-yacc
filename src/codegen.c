
#include "codegen.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    FILE *output;
    int error;
} Generator;

static void indent(Generator *gen, int level)
{
    for (int i = 0; i < level; ++i) {
        fputs("    ", gen->output);
    }
}

static void expression(Generator *gen, const Ast *node)
{
    FILE *out = gen->output;

    if (node == NULL) {
        gen->error = 1;
        return;
    }

    switch (node->kind) {
        case AST_NUMBER:
            fprintf(out, "%lld", node->number);
            break;

        case AST_VARIABLE:
            fprintf(out, "_get(\"%s\")", node->name);
            break;

        case AST_UNARY:
            if (strcmp(node->op, "!") == 0) {
                fputs("int(not (", out);
                expression(gen, node->left);
                fputs("))", out);
            } else if (strcmp(node->op, "-") == 0) {
                fputs("_check(-(", out);
                expression(gen, node->left);
                fputs("))", out);
            } else {
                gen->error = 1;
            }
            break;

        case AST_BINARY: {
            const char *op = node->op;

            if (strcmp(op, "&&") == 0 ||
                strcmp(op, "||") == 0) {
                fputs("int(bool(", out);
                expression(gen, node->left);
                fputs(strcmp(op, "&&") == 0
                    ? ") and bool(" : ") or bool(", out);
                expression(gen, node->right);
                fputs("))", out);

            } else if (strcmp(op, "/") == 0 ||
                       strcmp(op, "%") == 0) {
                fputs(strcmp(op, "/") == 0
                    ? "_div(" : "_mod(", out);
                expression(gen, node->left);
                fputs(", ", out);
                expression(gen, node->right);
                fputc(')', out);

            } else if (strcmp(op, "+") == 0 ||
                       strcmp(op, "-") == 0 ||
                       strcmp(op, "*") == 0) {
                fputs("_check((", out);
                expression(gen, node->left);
                fprintf(out, ") %s (", op);
                expression(gen, node->right);
                fputs("))", out);

            } else if (strcmp(op, "==") == 0 ||
                       strcmp(op, "!=") == 0 ||
                       strcmp(op, "<") == 0 ||
                       strcmp(op, "<=") == 0 ||
                       strcmp(op, ">") == 0 ||
                       strcmp(op, ">=") == 0) {
                fputs("int((", out);
                expression(gen, node->left);
                fprintf(out, ") %s (", op);
                expression(gen, node->right);
                fputs("))", out);

            } else {
                gen->error = 1;
            }
            break;
        }

        default:
            gen->error = 1;
    }
}

static void statement(Generator *gen, const Ast *node, int level);

static void block(Generator *gen, const Ast *node, int level)
{
    if (node == NULL || node->kind != AST_BLOCK) {
        gen->error = 1;
        return;
    }

    indent(gen, level);
    fputs("_step()\n", gen->output);

    indent(gen, level);
    fputs("with _scope():\n", gen->output);

    if (node->items == NULL) {
        indent(gen, level + 1);
        fputs("pass\n", gen->output);
    }

    for (const AstList *item = node->items;
         item != NULL; item = item->next) {
        statement(gen, item->node, level + 1);
    }
}

static void statement(Generator *gen, const Ast *node, int level)
{
    if (node == NULL) {
        gen->error = 1;
        return;
    }

    if (node->kind == AST_BLOCK) {
        block(gen, node, level);
        return;
    }

    FILE *out = gen->output;

    indent(gen, level);
    fputs("_step()\n", out);
    indent(gen, level);

    switch (node->kind) {
        case AST_DECL:
            fprintf(out, "_declare(\"%s\", ", node->name);
            if (node->left != NULL) {
                expression(gen, node->left);
            } else {
                fputc('0', out);
            }
            fputs(")\n", out);
            break;

        case AST_ASSIGN:
            fprintf(out, "_assign(\"%s\", ", node->name);
            expression(gen, node->left);
            fputs(")\n", out);
            break;

        case AST_PRINT:
            fputs("print(", out);
            expression(gen, node->left);
            if (node->number) {
                fputs(")\n", out);
            } else {
                fputs(", end=\"\")\n", out);
            }
            break;

        case AST_RETURN:
            fputs("return ", out);
            expression(gen, node->left);
            fputc('\n', out);
            break;

        case AST_IF:
            fputs("if ", out);
            expression(gen, node->left);
            fputs(":\n", out);
            block(gen, node->right, level + 1);

            if (node->third != NULL) {
                indent(gen, level);
                fputs("else:\n", out);
                block(gen, node->third, level + 1);
            }
            break;

        case AST_WHILE:
            fputs("while ", out);
            expression(gen, node->left);
            fputs(":\n", out);
            block(gen, node->right, level + 1);
            break;

        default:
            gen->error = 1;
    }
}

int generate_python(FILE *output, const Ast *root)
{
    if (output == NULL || root == NULL ||
        root->kind != AST_PROGRAM) {
        return 1;
    }

    Generator gen = {output, 0};

    fputs(
        "# Python code generated by MiniC from a restricted C program.\n"
        "from contextlib import contextmanager\n"
        "import sys\n\n"
        "MIN_I64 = -(1 << 63)\n"
        "MAX_I64 = (1 << 63) - 1\n"
        "_scopes = []\n"
        "_steps = 0\n\n"

        "def _step():\n"
        "    global _steps\n"
        "    _steps += 1\n"
        "    if _steps > 1000000:\n"
        "        raise RuntimeError('execution step limit exceeded')\n\n"

        "@contextmanager\n"
        "def _scope():\n"
        "    _scopes.append({})\n"
        "    try:\n"
        "        yield\n"
        "    finally:\n"
        "        _scopes.pop()\n\n"

        "def _declare(name, value):\n"
        "    scope = _scopes[-1]\n"
        "    if name in scope:\n"
        "        raise RuntimeError('duplicate declaration of ' + name)\n"
        "    scope[name] = value\n\n"

        "def _get(name):\n"
        "    for scope in reversed(_scopes):\n"
        "        if name in scope:\n"
        "            return scope[name]\n"
        "    raise RuntimeError('undeclared variable ' + name)\n\n"

        "def _assign(name, value):\n"
        "    for scope in reversed(_scopes):\n"
        "        if name in scope:\n"
        "            scope[name] = value\n"
        "            return\n"
        "    raise RuntimeError('undeclared variable ' + name)\n\n"

        "def _check(value):\n"
        "    if value < MIN_I64 or value > MAX_I64:\n"
        "        raise RuntimeError('integer overflow')\n"
        "    return value\n\n"

        "def _div(a, b):\n"
        "    if b == 0:\n"
        "        raise RuntimeError('division by zero')\n"
        "    return _check((abs(a) // abs(b)) * "
        "(-1 if (a < 0) != (b < 0) else 1))\n\n"

        "def _mod(a, b):\n"
        "    return a - _div(a, b) * b\n\n"

        "def main():\n"
        "    _step()\n",
        output
    );

    block(&gen, root->left, 1);

    fputs(
        "\nif __name__ == '__main__':\n"
        "    try:\n"
        "        sys.exit(main())\n"
        "    except RuntimeError as error:\n"
        "        print('Runtime error:', error, file=sys.stderr)\n"
        "        sys.exit(1)\n",
        output
    );

    if (gen.error) {
        fputs("Code generation error: unsupported AST node\n", stderr);
    }

    return gen.error || ferror(output);
}
