
#ifndef MINIC_JSON_H
#define MINIC_JSON_H

#include "ast.h"
#include <stdio.h>

/* Returns 0 on success, 1 if an error occurs. */
int write_ast_json(FILE *output, const Ast *root);

#endif
