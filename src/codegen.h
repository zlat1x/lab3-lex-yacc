
#ifndef MINIC_CODEGEN_H
#define MINIC_CODEGEN_H

#include <stdio.h>
#include "ast.h"

int generate_python(FILE *output, const Ast *root);

#endif
