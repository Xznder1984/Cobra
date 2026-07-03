#include "lexer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char *keywords[] = {
    "fn", "let", "const", "var",
    "if", "elif", "else",
    "while", "for", "in",
    "return", "break", "continue",
    "struct", "class", "enum", "trait", "impl",
    "match", "import", "from", "pub", "mut",
    "async", "await",
    "true", "false", "nil",
    "self", "super",
    "try", "throw",
    "type", "and", "or", "not",
    "as", "is", "where",
    "defer", "unsafe", "extern", "use",
    NULL
};

static TokenType keyword_types[] = {
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
    TOK_DEFER, TOK_UNSAFE, TOK_EXTERN, TOK_USE,
};

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOK_IDENTIFIER: return "identifier";
        case TOK_INTEGER: return "integer";
        case TOK_FLOAT: return "float";
        case TOK_STRING: return "string";
        case TOK_CHAR: return "char";
        case TOK_FN: return "fn";
        case TOK_LET: return "let";
        case TOK_CONST: return "const";
        case TOK_VAR: return "var";
        case TOK_IF: return "if";
        case TOK_ELIF: return "elif";
        case TOK_ELSE: return "else";
        case TOK_WHILE: return "while";
        case TOK_FOR: return "for";
        case TOK_IN: return "in";
        case TOK_RETURN: return "return";
        case TOK_BREAK: return "break";
        case TOK_CONTINUE: return "continue";
        case TOK_STRUCT: return "struct";
        case TOK_CLASS: return "class";
        case TOK_ENUM: return "enum";
        case TOK_TRAIT: return "trait";
        case TOK_IMPL: return "impl";
        case TOK_MATCH: return "match";
        case TOK_IMPORT: return "import";
        case TOK_FROM: return "from";
        case TOK_PUB: return "pub";
        case TOK_MUT: return "mut";
        case TOK_ASYNC: return "async";
        case TOK_AWAIT: return "await";
        case TOK_TRUE: return "true";
        case TOK_FALSE: return "false";
        case TOK_NIL: return "nil";
        case TOK_SELF: return "self";
        case TOK_SUPER: return "super";
        case TOK_TRY: return "try";
        case TOK_THROW: return "throw";
        case TOK_TYPE: return "type";
        case TOK_AND: return "and";
        case TOK_OR: return "or";
        case TOK_NOT: return "not";
        case TOK_AS: return "as";
        case TOK_IS: return "is";
        case TOK_WHERE: return "where";
        case TOK_DEFER: return "defer";
        case TOK_UNSAFE: return "unsafe";
        case TOK_EXTERN: return "extern";
        case TOK_USE: return "use";
        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_STAR: return "*";
        case TOK_SLASH: return "/";
        case TOK_PERCENT: return "%";
        case TOK_EQ: return "=";
        case TOK_EQ_EQ: return "==";
        case TOK_BANG: return "!";
        case TOK_BANG_EQ: return "!=";
        case TOK_LT: return "<";
        case TOK_GT: return ">";
        case TOK_LT_EQ: return "<=";
        case TOK_GT_EQ: return ">=";
        case TOK_AMPERSAND: return "&";
        case TOK_PIPE: return "|";
        case TOK_CARET: return "^";
        case TOK_LSHIFT: return "<<";
        case TOK_RSHIFT: return ">>";
        case TOK_PLUS_EQ: return "+=";
        case TOK_MINUS_EQ: return "-=";
        case TOK_STAR_EQ: return "*=";
        case TOK_SLASH_EQ: return "/=";
        case TOK_ARROW: return "->";
        case TOK_FAT_ARROW: return "=>";
        case TOK_DOT: return ".";
        case TOK_DOT_DOT: return "..";
        case TOK_DOT_DOT_EQ: return "..=";
        case TOK_COLON: return ":";
        case TOK_COMMA: return ",";
        case TOK_SEMICOLON: return ";";
        case TOK_AT: return "@";
        case TOK_HASH: return "#";
        case TOK_UNDERSCORE: return "_";
        case TOK_QUESTION: return "?";
        case TOK_PIPE_PIPE: return "||";
        case TOK_AMPERSAND_AMPERSAND: return "&&";
        case TOK_PLUS_PLUS: return "++";
        case TOK_MINUS_MINUS: return "--";
        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_LBRACKET: return "[";
        case TOK_RBRACKET: return "]";
        case TOK_LBRACE: return "{";
        case TOK_RBRACE: return "}";
        case TOK_INDENT: return "INDENT";
        case TOK_DEDENT: return "DEDENT";
        case TOK_NEWLINE: return "newline";
        case TOK_EOF: return "EOF";
        case TOK_ERROR: return "error";
        default: return "unknown";
    }
}

static TokenType lookup_keyword(const char *start, int len) {
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strlen(keywords[i]) == (size_t)len &&
            strncmp(keywords[i], start, len) == 0) {
            return keyword_types[i];
        }
    }
    return TOK_IDENTIFIER;
}

static Token make_token(Lexer *l, TokenType type) {
    Token t;
    t.type = type;
    t.start = l->source + l->pos;
    t.length = 0;
    t.line = l->line;
    t.column = l->column;
    t.int_value = 0;
    t.float_value = 0.0;
    return t;
}

static Token make_token_len(Lexer *l, TokenType type, int len) {
    Token t = make_token(l, type);
    t.length = len;
    return t;
}

static Token error_token(Lexer *l, const char *msg) {
    Token t = make_token(l, TOK_ERROR);
    t.length = (int)strlen(msg);
    diag_add(l->diags, DIAG_ERROR, l->line, l->column, "%s", msg);
    return t;
}

static char advance(Lexer *l) {
    if (l->pos >= l->source_len) return '\0';
    char c = l->source[l->pos++];
    if (c == '\n') {
        l->line++;
        l->column = 1;
    } else {
        l->column++;
    }
    return c;
}

static char peek(Lexer *l, int offset) {
    int p = l->pos + offset;
    if (p >= l->source_len) return '\0';
    return l->source[p];
}

static char current_char(Lexer *l) {
    return peek(l, 0);
}

static void skip_line_comment(Lexer *l) {
    while (peek(l, 0) != '\0' && peek(l, 0) != '\n') {
        advance(l);
    }
}

static void skip_block_comment(Lexer *l) {
    while (peek(l, 0) != '\0') {
        if (peek(l, 0) == '*' && peek(l, 1) == '/') {
            advance(l);
            advance(l);
            return;
        }
        advance(l);
    }
    diag_add(l->diags, DIAG_ERROR, l->line, l->column, "Unterminated block comment");
}

static int count_indentation(Lexer *l) {
    int col = 0;
    while (1) {
        char c = current_char(l);
        if (c == ' ') {
            col++;
            advance(l);
        } else if (c == '\t') {
            col += 8;
            advance(l);
        } else if (c == '\n' || c == '\r') {
            advance(l);
            col = 0;
        } else if (c == '#') {
            skip_line_comment(l);
            col = 0;
        } else {
            break;
        }
    }
    return col;
}

static int compute_indent_change(Lexer *l) {
    if (l->paren_depth > 0) return 0;

    int col = count_indentation(l);
    if (current_char(l) == '\0') return 0;

    int current_indent = l->indent_count > 0
        ? l->indent_stack[l->indent_count - 1]
        : 0;

    if (col > current_indent) {
        if (l->indent_count >= l->indent_capacity) {
            l->indent_capacity *= 2;
            l->indent_stack = realloc(l->indent_stack, l->indent_capacity * sizeof(int));
        }
        l->indent_stack[l->indent_count++] = col;
        return 1;
    } else if (col < current_indent) {
        int dedents = 0;
        while (l->indent_count > 0 && col < l->indent_stack[l->indent_count - 1]) {
            l->indent_count--;
            dedents++;
        }
        if (l->indent_count == 0 && col > 0) {
            diag_add(l->diags, DIAG_ERROR, l->line, l->column,
                     "Indentation mismatch");
        }
        return -dedents;
    }
    return 0;
}

static Token read_string(Lexer *l, char quote) {
    Token t = make_token(l, TOK_STRING);
    int start_pos = l->pos;
    while (1) {
        char c = advance(l);
        if (c == '\0') {
            diag_add(l->diags, DIAG_ERROR, l->line, l->column, "Unterminated string literal");
            t.type = TOK_ERROR;
            return t;
        }
        if (c == '\\') {
            advance(l);
        } else if (c == quote) {
            break;
        }
    }
    t.length = l->pos - start_pos - 1;
    t.start = l->source + start_pos;
    return t;
}

static Token read_number(Lexer *l) {
    Token t = make_token(l, TOK_INTEGER);
    int start_pos = l->pos - 1;
    int is_float = 0;

    while (isdigit(peek(l, 0))) advance(l);

    if (peek(l, 0) == '.' && isdigit(peek(l, 1))) {
        is_float = 1;
        advance(l);
        while (isdigit(peek(l, 0))) advance(l);
    }

    if (peek(l, 0) == 'e' || peek(l, 0) == 'E') {
        is_float = 1;
        advance(l);
        if (peek(l, 0) == '+' || peek(l, 0) == '-') advance(l);
        while (isdigit(peek(l, 0))) advance(l);
    }

    int len = l->pos - start_pos;
    t.length = len;

    char buf[64];
    if (len >= 64) len = 63;
    strncpy(buf, l->source + start_pos, len);
    buf[len] = '\0';

    if (is_float) {
        t.type = TOK_FLOAT;
        t.float_value = strtod(buf, NULL);
    } else {
        t.int_value = strtoll(buf, NULL, 10);
    }

    return t;
}

static Token read_identifier(Lexer *l) {
    int start_pos = l->pos - 1;
    while (isalnum(peek(l, 0)) || peek(l, 0) == '_') advance(l);
    int len = l->pos - start_pos;

    TokenType kw = lookup_keyword(l->source + start_pos, len);
    Token t = make_token_len(l, kw, len);
    t.start = l->source + start_pos;
    return t;
}

Lexer *lexer_create(const char *source, DiagnosticList *diags) {
    Lexer *l = calloc(1, sizeof(Lexer));
    l->source = source;
    l->source_len = (int)strlen(source);
    l->pos = 0;
    l->line = 1;
    l->column = 1;
    l->indent_capacity = 64;
    l->indent_stack = malloc(l->indent_capacity * sizeof(int));
    l->indent_count = 0;
    l->diags = diags;
    l->paren_depth = 0;
    l->current.type = TOK_EOF;
    l->peek.type = TOK_EOF;
    return l;
}

void lexer_free(Lexer *lexer) {
    if (lexer) {
        free(lexer->indent_stack);
        free(lexer);
    }
}

Token lexer_next(Lexer *l) {
    while (1) {
        char c = current_char(l);
        if (c == '\0') {
            if (l->indent_count > 0) {
                l->indent_count--;
                return make_token_len(l, TOK_DEDENT, 0);
            }
            return make_token(l, TOK_EOF);
        }

        if (c == '\n') {
            advance(l);
            if (l->paren_depth > 0) continue;

            int change = compute_indent_change(l);
            if (change > 0) {
                return make_token_len(l, TOK_INDENT, 0);
            } else if (change < 0) {
                return make_token_len(l, TOK_DEDENT, 0);
            }
            if (current_char(l) == '\0') {
                while (l->indent_count > 0) {
                    l->indent_count--;
                    return make_token_len(l, TOK_DEDENT, 0);
                }
                return make_token(l, TOK_EOF);
            }
            return make_token(l, TOK_NEWLINE);
        }

        if (c == ' ' || c == '\t' || c == '\r') {
            advance(l);
            continue;
        }

        if (c == '#') {
            skip_line_comment(l);
            continue;
        }

        break;
    }

    Token t;
    char c = advance(l);

    switch (c) {
        case '(': t = make_token(l, TOK_LPAREN); l->paren_depth++; break;
        case ')': t = make_token(l, TOK_RPAREN); if (l->paren_depth > 0) l->paren_depth--; break;
        case '[': t = make_token(l, TOK_LBRACKET); l->paren_depth++; break;
        case ']': t = make_token(l, TOK_RBRACKET); if (l->paren_depth > 0) l->paren_depth--; break;
        case '{': t = make_token(l, TOK_LBRACE); l->paren_depth++; break;
        case '}': t = make_token(l, TOK_RBRACE); if (l->paren_depth > 0) l->paren_depth--; break;
        case ',': t = make_token(l, TOK_COMMA); break;
        case ';': t = make_token(l, TOK_SEMICOLON); break;
        case ':': t = make_token(l, TOK_COLON); break;
        case '@': t = make_token(l, TOK_AT); break;
        case '?': t = make_token(l, TOK_QUESTION); break;
        case '_': t = make_token(l, TOK_UNDERSCORE); break;

        case '+':
            if (peek(l, 0) == '=') { advance(l); t = make_token(l, TOK_PLUS_EQ); }
            else if (peek(l, 0) == '+') { advance(l); t = make_token(l, TOK_PLUS_PLUS); }
            else t = make_token(l, TOK_PLUS);
            break;

        case '-':
            if (peek(l, 0) == '=') { advance(l); t = make_token(l, TOK_MINUS_EQ); }
            else if (peek(l, 0) == '>') { advance(l); t = make_token(l, TOK_ARROW); }
            else if (peek(l, 0) == '-') { advance(l); t = make_token(l, TOK_MINUS_MINUS); }
            else t = make_token(l, TOK_MINUS);
            break;

        case '*':
            if (peek(l, 0) == '=') { advance(l); t = make_token(l, TOK_STAR_EQ); }
            else t = make_token(l, TOK_STAR);
            break;

        case '/':
            if (peek(l, 0) == '/') { advance(l); skip_line_comment(l); return lexer_next(l); }
            else if (peek(l, 0) == '*') { advance(l); skip_block_comment(l); return lexer_next(l); }
            else if (peek(l, 0) == '=') { advance(l); t = make_token(l, TOK_SLASH_EQ); }
            else t = make_token(l, TOK_SLASH);
            break;

        case '%': t = make_token(l, TOK_PERCENT); break;

        case '=':
            if (peek(l, 0) == '=') { advance(l); t = make_token(l, TOK_EQ_EQ); }
            else if (peek(l, 0) == '>') { advance(l); t = make_token(l, TOK_FAT_ARROW); }
            else t = make_token(l, TOK_EQ);
            break;

        case '!':
            if (peek(l, 0) == '=') { advance(l); t = make_token(l, TOK_BANG_EQ); }
            else t = make_token(l, TOK_BANG);
            break;

        case '<':
            if (peek(l, 0) == '=') { advance(l); t = make_token(l, TOK_LT_EQ); }
            else if (peek(l, 0) == '<') { advance(l); t = make_token(l, TOK_LSHIFT); }
            else t = make_token(l, TOK_LT);
            break;

        case '>':
            if (peek(l, 0) == '=') { advance(l); t = make_token(l, TOK_GT_EQ); }
            else if (peek(l, 0) == '>') { advance(l); t = make_token(l, TOK_RSHIFT); }
            else t = make_token(l, TOK_GT);
            break;

        case '&':
            if (peek(l, 0) == '&') { advance(l); t = make_token(l, TOK_AMPERSAND_AMPERSAND); }
            else t = make_token(l, TOK_AMPERSAND);
            break;

        case '|':
            if (peek(l, 0) == '|') { advance(l); t = make_token(l, TOK_PIPE_PIPE); }
            else t = make_token(l, TOK_PIPE);
            break;

        case '^': t = make_token(l, TOK_CARET); break;

        case '.':
            if (peek(l, 0) == '.') {
                advance(l);
                if (peek(l, 0) == '=') { advance(l); t = make_token(l, TOK_DOT_DOT_EQ); }
                else t = make_token(l, TOK_DOT_DOT);
            } else t = make_token(l, TOK_DOT);
            break;

        case '"': case '\'':
            t = read_string(l, c);
            break;

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            t = read_number(l);
            break;

        default:
            if (isalpha(c) || c == '_') {
                t = read_identifier(l);
            } else {
                char msg[64];
                snprintf(msg, sizeof(msg), "Unexpected character '%c' (0x%02x)", c, c);
                t = error_token(l, msg);
            }
            break;
    }

    if (t.length == 0) {
        t.length = l->pos - (int)(t.start - l->source);
    }
    return t;
}

Token lexer_peek(Lexer *lexer) {
    if (lexer->peek.type == TOK_EOF) {
        lexer->peek = lexer_next(lexer);
    }
    return lexer->peek;
}

Token lexer_advance(Lexer *lexer) {
    if (lexer->peek.type != TOK_EOF) {
        lexer->current = lexer->peek;
        lexer->peek.type = TOK_EOF;
        lexer->peek.start = NULL;
    } else {
        lexer->current = lexer_next(lexer);
    }
    return lexer->current;
}

int lexer_expect(Lexer *lexer, TokenType type) {
    Token t = lexer_advance(lexer);
    if (t.type != type) {
        diag_add(lexer->diags, DIAG_ERROR, t.line, t.column,
                 "Expected '%s', got '%s'",
                 token_type_name(type), token_type_name(t.type));
        return 0;
    }
    return 1;
}

int lexer_match(Lexer *lexer, TokenType type) {
    Token t = lexer_peek(lexer);
    if (t.type == type) {
        lexer_advance(lexer);
        return 1;
    }
    return 0;
}
