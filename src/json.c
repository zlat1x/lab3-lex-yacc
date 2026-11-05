
#include "json.h"

#include <stdio.h>

static void indent(FILE *output, int depth)
{
    for (int i = 0; i < depth; ++i) {
        fputs("  ", output);
    }
}

static void write_escaped(FILE *output, const char *text)
{
    if (text == NULL) {
        return;
    }

    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        switch (*p) {
            case '"': fputs("\\\"", output); break;
            case '\\': fputs("\\\\", output); break;
            case '\n': fputs("\\n", output); break;
            case '\r': fputs("\\r", output); break;
            case '\t': fputs("\\t", output); break;
            default:
                if (*p < 32) {
                    fprintf(output, "\\u%04x", (unsigned int)*p);
                } else {
                    fputc(*p, output);
                }
        }
    }
}

static void write_string(FILE *output, const char *text)
{
    fputc('"', output);
    write_escaped(output, text);
    fputc('"', output);
}

static void write_label(FILE *output, const Ast *node)
{
    char number[32];
    fputc('"', output);

    switch (node->kind) {
        case AST_NUMBER:
            snprintf(number, sizeof(number), "%lld", node->number);
            write_escaped(output, number);
            break;

        case AST_VARIABLE:
            write_escaped(output, node->name);
            break;

        case AST_BINARY:
        case AST_UNARY:
            write_escaped(output, node->op);
            break;

        case AST_DECL:
            write_escaped(output, "int ");
            write_escaped(output, node->name);
            break;

        case AST_ASSIGN:
            write_escaped(output, node->name);
            write_escaped(output, " =");
            break;

        case AST_PRINT:
            write_escaped(output,
                node->number ? "printf(%d\\n)" : "printf(%d)");
            break;

        default:
            write_escaped(output, ast_kind_name(node->kind));
            break;
    }

    fputc('"', output);
}

static void write_node(FILE *output, const Ast *node, int depth)
{
    fputs("{\n", output);

    indent(output, depth + 1);
    fputs("\"type\": ", output);
    write_string(output, ast_kind_name(node->kind));
    fputs(",\n", output);

    indent(output, depth + 1);
    fputs("\"label\": ", output);
    write_label(output, node);
    fputs(",\n", output);

    indent(output, depth + 1);
    fputs("\"children\": [", output);

    int first = 1;
    const Ast *children[] = {
        node->left, node->right, node->third
    };

    for (int i = 0; i < 3; ++i) {
        if (children[i] == NULL) {
            continue;
        }

        fputs(first ? "\n" : ",\n", output);
        indent(output, depth + 2);
        write_node(output, children[i], depth + 2);
        first = 0;
    }

    for (const AstList *item = node->items;
         item != NULL; item = item->next) {

        if (item->node == NULL) {
            continue;
        }

        fputs(first ? "\n" : ",\n", output);
        indent(output, depth + 2);
        write_node(output, item->node, depth + 2);
        first = 0;
    }

    if (!first) {
        fputc('\n', output);
        indent(output, depth + 1);
    }

    fputs("]\n", output);
    indent(output, depth);
    fputc('}', output);
}

int write_ast_json(FILE *output, const Ast *root)
{
    if (output == NULL || root == NULL) {
        return 1;
    }

    write_node(output, root, 0);
    fputc('\n', output);

    return ferror(output) ? 1 : 0;
}
