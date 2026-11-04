
#include "ast.h"
#include "runtime.h"

#include <stdio.h>
#include <stdlib.h>

extern FILE *yyin;
extern int yylineno;
extern Ast *parsed_program;

int yyparse(void);

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *source = fopen(argv[1], "r");

    if (source == NULL) {
        perror("Cannot open source file");
        return EXIT_FAILURE;
    }

    yyin = source;
    yylineno = 1;

    int parse_status = yyparse();
    fclose(source);

    if (parse_status != 0 || parsed_program == NULL) {
        ast_free(parsed_program);
        return EXIT_FAILURE;
    }

    int run_status = run_program(parsed_program);

    ast_free(parsed_program);

    return run_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
