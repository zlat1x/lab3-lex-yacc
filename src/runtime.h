
#ifndef MINIC_RUNTIME_H
#define MINIC_RUNTIME_H

#include "ast.h"

/* Returns 0 on success and 1 on runtime error.
   The program exit code is stored in exit_code. */
int run_program(const Ast *root, int *exit_code);

#endif
