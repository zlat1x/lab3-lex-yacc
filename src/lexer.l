
%option noyywrap yylineno nodefault
%x COMMENT

%{
#include "parser.tab.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
%}

%%

[ \t\r\n]+                    ;
"//"[^\n]*                     ;

"/*"                          { BEGIN(COMMENT); }
<COMMENT>"*/"                  { BEGIN(INITIAL); }
<COMMENT>\n                    ;
<COMMENT>.                     ;
<COMMENT><<EOF>>               {
    fprintf(stderr, "Unterminated comment at line %d\n", yylineno);
    return YYUNDEF;
}

"int"                          { return INT; }
"main"                         { return MAIN; }
"if"                           { return IF; }
"else"                         { return ELSE; }
"while"                        { return WHILE; }
"return"                       { return RETURN; }
"printf"                       { return PRINTF; }

"=="                           { return EQ; }
"!="                           { return NE; }
"<="                           { return LE; }
">="                           { return GE; }
"&&"                           { return AND; }
"||"                           { return OR; }

\"([^\"\\\n]|\\.)*\"        {
    if (strcmp(yytext, "\"%d\"") != 0 &&
        strcmp(yytext, "\"%d\\n\"") != 0) {
        fprintf(stderr, "Unsupported format at line %d\n", yylineno);
        return YYUNDEF;
    }

    yylval.text = strdup(yytext);
    if (!yylval.text) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    return FORMAT;
}

[0-9]+                        {
    errno = 0;
    char *end = NULL;
    long long value = strtoll(yytext, &end, 10);

    if (errno == ERANGE || *end != '\0') {
        fprintf(stderr, "Invalid integer at line %d\n", yylineno);
        return YYUNDEF;
    }

    yylval.integer = value;
    return NUMBER;
}

[a-zA-Z_][a-zA-Z_0-9]*        {
    yylval.text = strdup(yytext);
    if (!yylval.text) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    return IDENT;
}

[{}();,=+*/%<>!\-]            { return yytext[0]; }

.                              {
    fprintf(stderr, "Unexpected character at line %d: %s\n",
            yylineno, yytext);
    return YYUNDEF;
}

%%
