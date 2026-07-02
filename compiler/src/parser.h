#ifndef COBRA_PARSER_H
#define COBRA_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct Parser {
    Lexer *lexer;
    Token current;
    DiagnosticList *diags;
    int had_error;
} Parser;

Parser *parser_create(Lexer *lexer, DiagnosticList *diags);
void parser_free(Parser *parser);
Node *parser_parse(Parser *parser);

#endif
