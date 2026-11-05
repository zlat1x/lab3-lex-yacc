
#include "ast.h"
#include "codegen.h"
#include "json.h"
#include "runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FILE *yyin;
extern int yylineno;
extern Ast *parsed_program;

int yyparse(void);

int main(int argc, char *argv[])
{
    int emit_python = (argc == 4 &&
                       strcmp(argv[1], "--emit-python") == 0);

    int emit_json = (argc == 4 &&
                     strcmp(argv[1], "--emit-json") == 0);

    int export_mode = emit_python || emit_json;

    if (argc != 2 && !export_mode) {
        fprintf(stderr,
                "Usage:\n"
                "  %s <source-file>\n"
                "  %s --emit-python <source-file> <output.py>\n"
                "  %s --emit-json <source-file> <output.json>\n",
                argv[0], argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    const char *source_path = export_mode ? argv[2] : argv[1];

    if (export_mode && strcmp(source_path, argv[3]) == 0) {
        fprintf(stderr, "Error: input and output paths must differ\n");
        return EXIT_FAILURE;
    }

    FILE *source = fopen(source_path, "r");

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

    int status = 0;

    if (export_mode) {
        FILE *output = fopen(argv[3], "w");

        if (output == NULL) {
            perror("Cannot create output file");
            ast_free(parsed_program);
            return EXIT_FAILURE;
        }

        int generation_status;

        if (emit_python) {
            generation_status = generate_python(output, parsed_program);
        } else {
            generation_status = write_ast_json(output, parsed_program);
        }

        int close_status = fclose(output);

        if (generation_status != 0 || close_status != 0) {
            fprintf(stderr, "Error: code export failed\n");
            remove(argv[3]);
            status = 1;
        } else if (emit_python) {
            printf("Generated Python: %s\n", argv[3]);
        } else {
            printf("Generated AST JSON: %s\n", argv[3]);
        }
    } else {
        status = run_program(parsed_program);
    }

    ast_free(parsed_program);

    return status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
