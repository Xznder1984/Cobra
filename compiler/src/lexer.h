#ifndef COBRA_LEXER_H
#define COBRA_LEXER_H

#include "utils.h"

typedef enum {
    TOK_IDENTIFIER, TOK_INTEGER, TOK_FLOAT, TOK_STRING, TOK_CHAR,

    TOK_FN, TOK_LET, TOK_CONST, TOK_VAR,
    TOK_IF, TOK_ELIF, TOK_ELSE,
    TOK_WHILE, TOK_FOR, TOK_IN,
    TOK_RETURN, TOK_BREAK, TOK_CONTINUE,
    TOK_STRUCT, TOK_CLASS, TOK_ENUM, TOK_TRAIT, TOK_IMPL,
    TOK_MATCH, TOK_IMPORT, TOK_FROM, TOK_PUB, TOK_MUT,
    TOK_ASYNC, TOK_AWAIT,
    TOK_TRUE, TOK_FALSE, TOK_NIL,
    TOK_SELF, TOK_SUPER,
    TOK_TRY, TOK_THROW,
    TOK_TYPE, TOK_AND, TOK_OR, TOK_NOT,
    TOK_AS, TOK_IS, TOK_WHERE,
    TOK_DEFER, TOK_UNSAFE, TOK_EXTERN,

    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_EQ, TOK_EQ_EQ, TOK_BANG, TOK_BANG_EQ,
    TOK_LT, TOK_GT, TOK_LT_EQ, TOK_GT_EQ,
    TOK_AMPERSAND, TOK_PIPE, TOK_CARET,
    TOK_LSHIFT, TOK_RSHIFT,
    TOK_PLUS_EQ, TOK_MINUS_EQ, TOK_STAR_EQ, TOK_SLASH_EQ,
    TOK_ARROW, TOK_FAT_ARROW,
    TOK_DOT, TOK_DOT_DOT, TOK_DOT_DOT_EQ,
    TOK_COLON, TOK_COMMA, TOK_SEMICOLON,
    TOK_AT, TOK_HASH, TOK_UNDERSCORE,
    TOK_QUESTION, TOK_PIPE_PIPE, TOK_AMPERSAND_AMPERSAND,
    TOK_PLUS_PLUS, TOK_MINUS_MINUS,

    TOK_LPAREN, TOK_RPAREN,
    TOK_LBRACKET, TOK_RBRACKET,
    TOK_LBRACE, TOK_RBRACE,

    TOK_INDENT, TOK_DEDENT, TOK_NEWLINE, TOK_EOF, TOK_ERROR,
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int length;
    int line;
    int column;
    union {
        long long int_value;
        double float_value;
    };
} Token;

typedef struct {
    const char *source;
    int source_len;
    int pos;
    int line;
    int column;
    int *indent_stack;
    int indent_capacity;
    int indent_count;
    Token current;
    Token peek;
    DiagnosticList *diags;
    int paren_depth;
} Lexer;

const char *token_type_name(TokenType type);

Lexer *lexer_create(const char *source, DiagnosticList *diags);
void lexer_free(Lexer *lexer);
Token lexer_next(Lexer *lexer);
Token lexer_peek(Lexer *lexer);
Token lexer_advance(Lexer *lexer);
int lexer_expect(Lexer *lexer, TokenType type);
int lexer_match(Lexer *lexer, TokenType type);

#endif
