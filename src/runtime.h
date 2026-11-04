
#ifndef MINIC_RUNTIME_H
#define MINIC_RUNTIME_H

#include "ast.h"

/* Returns 0 on success and 1 if an execution error occurs. */
int run_program(const Ast *root);

#endif
