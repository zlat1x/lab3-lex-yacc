
%{
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

int yylex(void);
void yyerror(const char *message);
extern int yylineno;

Ast *parsed_program = NULL;
%}

%code requires {
#include "ast.h"
}

%define parse.error detailed
%expect 0

%union {
    long long integer;
    char *text;
    Ast *node;
}

%token <integer> NUMBER
%token <text> IDENT FORMAT
%token INT MAIN IF ELSE WHILE RETURN PRINTF
%token EQ NE LE GE AND OR

%type <node> block statements statement expression

%left OR
%left AND
%left EQ NE
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'
%precedence '!' UMINUS

%destructor { ast_free($$); } <node>
%destructor { free($$); } <text>

%start program

%%

program:
    INT MAIN '(' ')' block
    {
        parsed_program = ast_program($5);
    }
    ;

block:
    '{' statements '}'
    {
        $$ = $2;
    }
    ;

statements:
    %empty
    {
        $$ = ast_block();
    }
    | statements statement
    {
        ast_append($1, $2);
        $$ = $1;
    }
    ;

statement:
    INT IDENT ';'
    {
        $$ = ast_decl($2, NULL);
    }
    | INT IDENT '=' expression ';'
    {
        $$ = ast_decl($2, $4);
    }
    | IDENT '=' expression ';'
    {
        $$ = ast_assign($1, $3);
    }
    | PRINTF '(' FORMAT ',' expression ')' ';'
    {
        $$ = ast_print($5, $3[3] == '\\');
        free($3);
    }
    | RETURN expression ';'
    {
        $$ = ast_return($2);
    }
    | IF '(' expression ')' block
    {
        $$ = ast_if($3, $5, NULL);
    }
    | IF '(' expression ')' block ELSE block
    {
        $$ = ast_if($3, $5, $7);
    }
    | WHILE '(' expression ')' block
    {
        $$ = ast_while($3, $5);
    }
    | block
    {
        $$ = $1;
    }
    ;

expression:
    NUMBER
    {
        $$ = ast_number($1);
    }
    | IDENT
    {
        $$ = ast_variable($1);
    }
    | '(' expression ')'
    {
        $$ = $2;
    }
    | '-' expression %prec UMINUS
    {
        $$ = ast_unary("-", $2);
    }
    | '!' expression
    {
        $$ = ast_unary("!", $2);
    }
    | expression '+' expression
    {
        $$ = ast_binary("+", $1, $3);
    }
    | expression '-' expression
    {
        $$ = ast_binary("-", $1, $3);
    }
    | expression '*' expression
    {
        $$ = ast_binary("*", $1, $3);
    }
    | expression '/' expression
    {
        $$ = ast_binary("/", $1, $3);
    }
    | expression '%' expression
    {
        $$ = ast_binary("%", $1, $3);
    }
    | expression EQ expression
    {
        $$ = ast_binary("==", $1, $3);
    }
    | expression NE expression
    {
        $$ = ast_binary("!=", $1, $3);
    }
    | expression LE expression
    {
        $$ = ast_binary("<=", $1, $3);
    }
    | expression GE expression
    {
        $$ = ast_binary(">=", $1, $3);
    }
    | expression '<' expression
    {
        $$ = ast_binary("<", $1, $3);
    }
    | expression '>' expression
    {
        $$ = ast_binary(">", $1, $3);
    }
    | expression AND expression
    {
        $$ = ast_binary("&&", $1, $3);
    }
    | expression OR expression
    {
        $$ = ast_binary("||", $1, $3);
    }
    ;

%%

void yyerror(const char *message)
{
    fprintf(stderr, "Syntax error on line %d: %s\n",
            yylineno, message);
}
