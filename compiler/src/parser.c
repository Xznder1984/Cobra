#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

Parser *parser_create(Lexer *lexer, DiagnosticList *diags) {
    Parser *p = calloc(1, sizeof(Parser));
    p->lexer = lexer;
    p->diags = diags;
    p->current = lexer_advance(lexer);
    p->had_error = 0;
    return p;
}

void parser_free(Parser *parser) {
    free(parser);
}

static Token peek(Parser *p) {
    return p->current;
}

static Token advance(Parser *p) {
    Token t = p->current;
    p->current = lexer_advance(p->lexer);
    return t;
}

static int match(Parser *p, TokenType type) {
    if (p->current.type == type) {
        advance(p);
        return 1;
    }
    return 0;
}

static int check(Parser *p, TokenType type) {
    return p->current.type == type;
}

static Token consume(Parser *p, TokenType type, const char *msg) {
    if (p->current.type == type) return advance(p);
    diag_add(p->diags, DIAG_ERROR, p->current.line, p->current.column,
             "%s (expected '%s', got '%s')", msg,
             token_type_name(type), token_type_name(p->current.type));
    p->had_error = 1;
    Token t;
    t.type = TOK_ERROR;
    return t;
}

static void synchronize(Parser *p) {
    advance(p);
    while (1) {
        if (peek(p).type == TOK_EOF) return;
        if (peek(p).type == TOK_NEWLINE) { advance(p); return; }
        switch (peek(p).type) {
            case TOK_FN: case TOK_LET: case TOK_CONST:
            case TOK_IF: case TOK_WHILE: case TOK_FOR:
            case TOK_RETURN: case TOK_STRUCT: case TOK_CLASS:
            case TOK_ENUM: case TOK_TRAIT: case TOK_IMPORT:
            case TOK_PUB: case TOK_DEDENT:
                return;
            default: advance(p);
        }
    }
}

static Node *parse_stmt(Parser *p);
static Node *parse_expr(Parser *p);
static Node *parse_assignment(Parser *p);

static Node *parse_block(Parser *p, int need_indent) {
    Node *block = node_create(NODE_BLOCK, p->current.line, p->current.column);

    if (need_indent) {
        if (!match(p, TOK_INDENT)) {
            if (check(p, TOK_NEWLINE)) {
                advance(p);
                if (!match(p, TOK_INDENT)) {
                    diag_add(p->diags, DIAG_ERROR, peek(p).line, peek(p).column,
                             "Expected indented block");
                    p->had_error = 1;
                    return block;
                }
            }
        }
    }

    while (!check(p, TOK_DEDENT) && !check(p, TOK_EOF)) {
        if (match(p, TOK_NEWLINE)) continue;
        Node *stmt = parse_stmt(p);
        if (stmt) {
            node_list_add(&block->data.block.statements, stmt);
        } else {
            synchronize(p);
        }
        while (match(p, TOK_NEWLINE));
    }

    if (need_indent) {
        match(p, TOK_DEDENT);
    }

    return block;
}

static Node *parse_fn_def(Parser *p) {
    Token fn_tok = advance(p);
    Token name_tok = consume(p, TOK_IDENTIFIER, "Expected function name");
    if (name_tok.type == TOK_ERROR) return NULL;

    Node *node = node_create(NODE_FN_DEF, fn_tok.line, fn_tok.column);
    node->data.fn_def.name = str_ndup(name_tok.start, name_tok.length);
    node->data.fn_def.is_public = 0;
    node->data.fn_def.is_async = 0;
    node->data.fn_def.return_type = NULL;
    node_list_init(&node->data.fn_def.params);
    node_list_init(&node->data.fn_def.body);
    node_list_init(&node->data.fn_def.type_params);

    consume(p, TOK_LPAREN, "Expected '(' after function name");

    if (!check(p, TOK_RPAREN)) {
        do {
            if (check(p, TOK_IDENTIFIER)) {
                Token pname = advance(p);
                Node *param = node_create(NODE_PARAM, pname.line, pname.column);
                param->data.param.name = str_ndup(pname.start, pname.length);
                param->data.param.type = NULL;
                param->data.param.default_value = NULL;

                if (match(p, TOK_COLON)) {
                    if (check(p, TOK_IDENTIFIER)) {
                        Token tname = advance(p);
                        param->data.param.type = type_create_named(
                            str_ndup(tname.start, tname.length));
                    }
                }

                if (match(p, TOK_EQ)) {
                    param->data.param.default_value = parse_expr(p);
                }

                node_list_add(&node->data.fn_def.params, param);
            }
        } while (match(p, TOK_COMMA));
    }

    consume(p, TOK_RPAREN, "Expected ')' after parameters");

    if (match(p, TOK_ARROW)) {
        if (check(p, TOK_IDENTIFIER)) {
            Token tname = advance(p);
            node->data.fn_def.return_type = type_create_named(
                str_ndup(tname.start, tname.length));
        } else if (check(p, TOK_LPAREN)) {
            advance(p);
            Type **params = NULL;
            int pcount = 0;
            if (!check(p, TOK_RPAREN)) {
                do {
                    if (check(p, TOK_IDENTIFIER)) {
                        Token t = advance(p);
                        params = realloc(params, (pcount + 1) * sizeof(Type *));
                        params[pcount++] = type_create_named(
                            str_ndup(t.start, t.length));
                    }
                } while (match(p, TOK_COMMA));
            }
            consume(p, TOK_RPAREN, "Expected ')'");
            Type *ret = NULL;
            if (match(p, TOK_ARROW)) {
                if (check(p, TOK_IDENTIFIER)) {
                    Token rt = advance(p);
                    ret = type_create_named(str_ndup(rt.start, rt.length));
                }
            }
            node->data.fn_def.return_type = type_create_fn(params, pcount, ret);
        }
    }

    if (check(p, TOK_COLON)) {
        advance(p);
        Node *fn_body = parse_block(p, 1);
        node->data.fn_def.body = fn_body->data.block.statements;
        free(fn_body);
    } else if (match(p, TOK_FAT_ARROW)) {
        Node *expr = parse_expr(p);
        Node *ret_stmt = node_create(NODE_RETURN, expr->line, expr->column);
        ret_stmt->data.return_stmt.value = expr;
        node_list_add(&node->data.fn_def.body, ret_stmt);
    }

    return node;
}

static Node *parse_let_decl(Parser *p) {
    Token let_tok = advance(p);
    int is_mut = match(p, TOK_MUT);
    Token name_tok = consume(p, TOK_IDENTIFIER, "Expected variable name");
    if (name_tok.type == TOK_ERROR) return NULL;

    Node *node = node_create(NODE_LET_DECL, let_tok.line, let_tok.column);
    node->data.let_decl.name = str_ndup(name_tok.start, name_tok.length);
    node->data.let_decl.is_mut = is_mut;
    node->data.let_decl.type = NULL;
    node->data.let_decl.value = NULL;

    if (match(p, TOK_COLON)) {
        if (check(p, TOK_IDENTIFIER)) {
            Token tname = advance(p);
            node->data.let_decl.type = type_create_named(
                str_ndup(tname.start, tname.length));
        }
    }

    if (match(p, TOK_EQ)) {
        node->data.let_decl.value = parse_expr(p);
    }

    return node;
}

static Node *parse_const_decl(Parser *p) {
    Token tok = advance(p);
    Token name_tok = consume(p, TOK_IDENTIFIER, "Expected constant name");
    if (name_tok.type == TOK_ERROR) return NULL;

    Node *node = node_create(NODE_CONST_DECL, tok.line, tok.column);
    node->data.const_decl.name = str_ndup(name_tok.start, name_tok.length);
    node->data.const_decl.type = NULL;
    node->data.const_decl.value = NULL;

    if (match(p, TOK_COLON)) {
        if (check(p, TOK_IDENTIFIER)) {
            Token tname = advance(p);
            node->data.const_decl.type = type_create_named(
                str_ndup(tname.start, tname.length));
        }
    }

    if (match(p, TOK_EQ)) {
        node->data.const_decl.value = parse_expr(p);
    }

    return node;
}

static Node *parse_struct_def(Parser *p) {
    Token tok = advance(p);
    Token name_tok = consume(p, TOK_IDENTIFIER, "Expected struct name");
    if (name_tok.type == TOK_ERROR) return NULL;

    Node *node = node_create(NODE_STRUCT_DEF, tok.line, tok.column);
    node->data.struct_def.name = str_ndup(name_tok.start, name_tok.length);
    node->data.struct_def.is_public = 0;
    node_list_init(&node->data.struct_def.fields);
    node_list_init(&node->data.struct_def.type_params);

    consume(p, TOK_COLON, "Expected ':' after struct name");
    consume(p, TOK_NEWLINE, "Expected newline after struct definition");

    if (match(p, TOK_INDENT)) {
        while (!check(p, TOK_DEDENT) && !check(p, TOK_EOF)) {
            if (match(p, TOK_NEWLINE)) continue;

            Token fname = consume(p, TOK_IDENTIFIER, "Expected field name");
            consume(p, TOK_COLON, "Expected ':' after field name");

            if (check(p, TOK_IDENTIFIER)) {
                Token ft = advance(p);
                Node *field = node_create(NODE_PARAM, fname.line, fname.column);
                field->data.param.name = str_ndup(fname.start, fname.length);
                field->data.param.type = type_create_named(
                    str_ndup(ft.start, ft.length));
                field->data.param.default_value = NULL;

                if (match(p, TOK_EQ)) {
                    field->data.param.default_value = parse_expr(p);
                }

                node_list_add(&node->data.struct_def.fields, field);
            }

            match(p, TOK_NEWLINE);
        }
        match(p, TOK_DEDENT);
    }

    return node;
}

static Node *parse_class_def(Parser *p) {
    Token tok = advance(p);
    Token name_tok = consume(p, TOK_IDENTIFIER, "Expected class name");
    if (name_tok.type == TOK_ERROR) return NULL;

    Node *node = node_create(NODE_CLASS_DEF, tok.line, tok.column);
    node->data.class_def.name = str_ndup(name_tok.start, name_tok.length);
    node->data.class_def.is_public = 0;
    node_list_init(&node->data.class_def.fields);
    node_list_init(&node->data.class_def.methods);
    node_list_init(&node->data.class_def.type_params);

    consume(p, TOK_COLON, "Expected ':' after class name");
    consume(p, TOK_NEWLINE, "Expected newline");
    if (match(p, TOK_INDENT)) {
        while (!check(p, TOK_DEDENT) && !check(p, TOK_EOF)) {
            if (match(p, TOK_NEWLINE)) continue;

            if (check(p, TOK_FN)) {
                Node *method = parse_fn_def(p);
                if (method) node_list_add(&node->data.class_def.methods, method);
            } else if (check(p, TOK_IDENTIFIER)) {
                Token fname = advance(p);
                consume(p, TOK_COLON, "Expected ':'");
                if (check(p, TOK_IDENTIFIER)) {
                    Token ft = advance(p);
                    Node *field = node_create(NODE_PARAM, fname.line, fname.column);
                    field->data.param.name = str_ndup(fname.start, fname.length);
                    field->data.param.type = type_create_named(
                        str_ndup(ft.start, ft.length));
                    field->data.param.default_value = NULL;
                    if (match(p, TOK_EQ)) {
                        field->data.param.default_value = parse_expr(p);
                    }
                    node_list_add(&node->data.class_def.fields, field);
                }
            }

            match(p, TOK_NEWLINE);
        }
        match(p, TOK_DEDENT);
    }

    return node;
}

static Node *parse_enum_def(Parser *p) {
    Token tok = advance(p);
    Token name_tok = consume(p, TOK_IDENTIFIER, "Expected enum name");
    if (name_tok.type == TOK_ERROR) return NULL;

    Node *node = node_create(NODE_ENUM_DEF, tok.line, tok.column);
    node->data.enum_def.name = str_ndup(name_tok.start, name_tok.length);
    node->data.enum_def.is_public = 0;
    node_list_init(&node->data.enum_def.variants);
    node_list_init(&node->data.enum_def.type_params);

    consume(p, TOK_COLON, "Expected ':' after enum name");
    consume(p, TOK_NEWLINE, "Expected newline");
    if (match(p, TOK_INDENT)) {
        while (!check(p, TOK_DEDENT) && !check(p, TOK_EOF)) {
            if (match(p, TOK_NEWLINE)) continue;

            Token vname = consume(p, TOK_IDENTIFIER, "Expected variant name");
            Node *variant = node_create(NODE_IDENTIFIER, vname.line, vname.column);
            variant->data.identifier.name = str_ndup(vname.start, vname.length);
            node_list_add(&node->data.enum_def.variants, variant);
            match(p, TOK_NEWLINE);
        }
        match(p, TOK_DEDENT);
    }

    return node;
}

static Node *parse_trait_def(Parser *p) {
    Token tok = advance(p);
    Token name_tok = consume(p, TOK_IDENTIFIER, "Expected trait name");
    if (name_tok.type == TOK_ERROR) return NULL;

    Node *node = node_create(NODE_TRAIT_DEF, tok.line, tok.column);
    node->data.trait_def.name = str_ndup(name_tok.start, name_tok.length);
    node->data.trait_def.is_public = 0;
    node_list_init(&node->data.trait_def.methods);
    node_list_init(&node->data.trait_def.type_params);

    consume(p, TOK_COLON, "Expected ':' after trait name");
    consume(p, TOK_NEWLINE, "Expected newline");
    if (match(p, TOK_INDENT)) {
        while (!check(p, TOK_DEDENT) && !check(p, TOK_EOF)) {
            if (match(p, TOK_NEWLINE)) continue;
            if (check(p, TOK_FN)) {
                Node *method = parse_fn_def(p);
                if (method) node_list_add(&node->data.trait_def.methods, method);
            }
            match(p, TOK_NEWLINE);
        }
        match(p, TOK_DEDENT);
    }

    return node;
}

static Node *parse_import(Parser *p) {
    Token tok = advance(p);
    Node *node = node_create(NODE_IMPORT, tok.line, tok.column);
    node->data.import.from_module = NULL;
    node->data.import.name = NULL;
    node->data.import.alias = NULL;

    if (check(p, TOK_IDENTIFIER)) {
        Token name = advance(p);
        node->data.import.name = str_ndup(name.start, name.length);

        if (match(p, TOK_DOT)) {
            free(node->data.import.from_module);
            node->data.import.from_module = node->data.import.name;
            Token sub = consume(p, TOK_IDENTIFIER, "Expected module name");
            node->data.import.name = str_ndup(sub.start, sub.length);
        }
    }

    return node;
}

static Node *parse_if_stmt(Parser *p) {
    Token tok = advance(p);
    Node *node = node_create(NODE_IF, tok.line, tok.column);
    node->data.if_stmt.condition = parse_expr(p);
    node->data.if_stmt.then_block = NULL;
    node->data.if_stmt.elif_chain = NULL;
    node->data.if_stmt.else_block = NULL;

    consume(p, TOK_COLON, "Expected ':' after if condition");
    node->data.if_stmt.then_block = parse_block(p, 1);

    while (check(p, TOK_ELIF)) {
        advance(p);
        Node *elif_node = node_create(NODE_IF, p->current.line, p->current.column);
        elif_node->data.if_stmt.condition = parse_expr(p);
        consume(p, TOK_COLON, "Expected ':' after elif condition");
        elif_node->data.if_stmt.then_block = parse_block(p, 1);

        if (!node->data.if_stmt.elif_chain) {
            node->data.if_stmt.elif_chain = elif_node;
        } else {
            Node *last = node->data.if_stmt.elif_chain;
            while (last->data.if_stmt.elif_chain) {
                last = last->data.if_stmt.elif_chain;
            }
            last->data.if_stmt.elif_chain = elif_node;
        }
    }

    if (check(p, TOK_ELSE)) {
        advance(p);
        consume(p, TOK_COLON, "Expected ':' after else");
        node->data.if_stmt.else_block = parse_block(p, 1);
    }

    return node;
}

static Node *parse_while_loop(Parser *p) {
    Token tok = advance(p);
    Node *node = node_create(NODE_WHILE, tok.line, tok.column);
    node->data.while_loop.condition = parse_expr(p);
    consume(p, TOK_COLON, "Expected ':' after while condition");
    node->data.while_loop.body = parse_block(p, 1);
    return node;
}

static Node *parse_for_loop(Parser *p) {
    Token tok = advance(p);
    Node *node = node_create(NODE_FOR, tok.line, tok.column);
    Token vname = consume(p, TOK_IDENTIFIER, "Expected loop variable name");
    node->data.for_loop.var_name = str_ndup(vname.start, vname.length);
    consume(p, TOK_IN, "Expected 'in' after for variable");
    node->data.for_loop.iterable = parse_expr(p);
    consume(p, TOK_COLON, "Expected ':' after for expression");
    node->data.for_loop.body = parse_block(p, 1);
    return node;
}

static Node *parse_return(Parser *p) {
    Token tok = advance(p);
    Node *node = node_create(NODE_RETURN, tok.line, tok.column);
    node->data.return_stmt.value = NULL;
    if (!check(p, TOK_NEWLINE) && !check(p, TOK_DEDENT) && !check(p, TOK_EOF)) {
        node->data.return_stmt.value = parse_expr(p);
    }
    return node;
}

static Node *parse_break(Parser *p) {
    Token tok = advance(p);
    return node_create(NODE_BREAK, tok.line, tok.column);
}

static Node *parse_continue(Parser *p) {
    Token tok = advance(p);
    return node_create(NODE_CONTINUE, tok.line, tok.column);
}

static Node *parse_primary(Parser *p);

static Node *parse_match(Parser *p) {
    Token tok = advance(p);
    Node *node = node_create(NODE_MATCH, tok.line, tok.column);
    node->data.match_stmt.expr = parse_expr(p);
    node_list_init(&node->data.match_stmt.arms);

    consume(p, TOK_COLON, "Expected ':' after match expression");
    consume(p, TOK_NEWLINE, "Expected newline");

    if (match(p, TOK_INDENT)) {
        while (!check(p, TOK_DEDENT) && !check(p, TOK_EOF)) {
            if (match(p, TOK_NEWLINE)) continue;

            Node *arm = node_create(NODE_MATCH_ARM, peek(p).line, peek(p).column);
            node_list_init(&arm->data.match_arm.patterns);
            arm->data.match_arm.guard = NULL;
            arm->data.match_arm.value = NULL;

            Node *pat = parse_primary(p);
            if (pat) node_list_add(&arm->data.match_arm.patterns, pat);

            if (match(p, TOK_IF)) {
                arm->data.match_arm.guard = parse_expr(p);
            }

            consume(p, TOK_FAT_ARROW, "Expected '=>' in match arm");

            if (check(p, TOK_NEWLINE)) {
                advance(p);
                arm->data.match_arm.value = parse_block(p, 1);
            } else {
                arm->data.match_arm.value = parse_expr(p);
            }

            node_list_add(&node->data.match_stmt.arms, arm);
            match(p, TOK_NEWLINE);
        }
        match(p, TOK_DEDENT);
    }

    return node;
}

static Node *parse_stmt(Parser *p) {
    if (match(p, TOK_PUB)) {
        Node *n = parse_stmt(p);
        if (n) {
            switch (n->type) {
                case NODE_FN_DEF: n->data.fn_def.is_public = 1; break;
                case NODE_STRUCT_DEF: n->data.struct_def.is_public = 1; break;
                case NODE_CLASS_DEF: n->data.class_def.is_public = 1; break;
                case NODE_ENUM_DEF: n->data.enum_def.is_public = 1; break;
                case NODE_TRAIT_DEF: n->data.trait_def.is_public = 1; break;
                case NODE_CONST_DECL: break;
                default: break;
            }
        }
        return n;
    }

    switch (peek(p).type) {
        case TOK_FN: return parse_fn_def(p);
        case TOK_LET: return parse_let_decl(p);
        case TOK_CONST: return parse_const_decl(p);
        case TOK_STRUCT: return parse_struct_def(p);
        case TOK_CLASS: return parse_class_def(p);
        case TOK_ENUM: return parse_enum_def(p);
        case TOK_TRAIT: return parse_trait_def(p);
        case TOK_IMPORT: return parse_import(p);
        case TOK_IF: return parse_if_stmt(p);
        case TOK_WHILE: return parse_while_loop(p);
        case TOK_FOR: return parse_for_loop(p);
        case TOK_MATCH: return parse_match(p);
        case TOK_RETURN: return parse_return(p);
        case TOK_BREAK: return parse_break(p);
        case TOK_CONTINUE: return parse_continue(p);

        case TOK_NEWLINE:
            advance(p);
            return NULL;

        case TOK_INDENT:
        case TOK_DEDENT:
            return NULL;

        case TOK_EOF:
            return NULL;

        default: {
            Node *expr = parse_expr(p);
            if (expr) {
                Node *stmt = node_create(NODE_EXPR_STMT, expr->line, expr->column);
                stmt->data.expr_stmt.expr = expr;
                return stmt;
            }
            return NULL;
        }
    }
}

static Node *parse_primary(Parser *p) {
    Token t = peek(p);

    switch (t.type) {
        case TOK_INTEGER: {
            advance(p);
            return node_make_int(t.line, t.column, t.int_value);
        }
        case TOK_FLOAT: {
            advance(p);
            return node_make_float(t.line, t.column, t.float_value);
        }
        case TOK_STRING: {
            advance(p);
            char *val = str_ndup(t.start, t.length);
            return node_make_str(t.line, t.column, val);
        }
        case TOK_TRUE: advance(p); return node_make_bool(t.line, t.column, 1);
        case TOK_FALSE: advance(p); return node_make_bool(t.line, t.column, 0);
        case TOK_NIL: advance(p); return node_make_nil(t.line, t.column);

        case TOK_IDENTIFIER: {
            advance(p);
            Node *n = node_make_ident(t.line, t.column,
                                       str_ndup(t.start, t.length));

            if (match(p, TOK_LPAREN)) {
                Node *call = node_create(NODE_CALL, t.line, t.column);
                call->data.call.callee = n;
                node_list_init(&call->data.call.args);

                if (!check(p, TOK_RPAREN)) {
                    do {
                        node_list_add(&call->data.call.args, parse_expr(p));
                    } while (match(p, TOK_COMMA));
                }
                consume(p, TOK_RPAREN, "Expected ')' after arguments");
                return call;
            }

            if (match(p, TOK_LBRACKET)) {
                Node *index = node_create(NODE_INDEX, t.line, t.column);
                index->data.index_expr.target = n;
                index->data.index_expr.index = parse_expr(p);
                consume(p, TOK_RBRACKET, "Expected ']' after index");
                return index;
            }

            if (match(p, TOK_DOT)) {
                Token mem = consume(p, TOK_IDENTIFIER, "Expected member name");
                Node *member = node_create(NODE_MEMBER, t.line, t.column);
                member->data.member_expr.object = n;
                member->data.member_expr.member = str_ndup(mem.start, mem.length);
                return member;
            }

            return n;
        }

        case TOK_LPAREN: {
            advance(p);
            Node *expr = parse_expr(p);
            if (match(p, TOK_COMMA)) {
                Node *tuple = node_create(NODE_TUPLE_LITERAL, t.line, t.column);
                node_list_init(&tuple->data.tuple_literal.elements);
                node_list_add(&tuple->data.tuple_literal.elements, expr);
                while (!check(p, TOK_RPAREN) && !check(p, TOK_EOF)) {
                    node_list_add(&tuple->data.tuple_literal.elements, parse_expr(p));
                    match(p, TOK_COMMA);
                }
                consume(p, TOK_RPAREN, "Expected ')'");
                return tuple;
            }
            consume(p, TOK_RPAREN, "Expected ')'");
            return expr;
        }

        case TOK_LBRACKET: {
            advance(p);
            Node *list = node_create(NODE_LIST_LITERAL, t.line, t.column);
            node_list_init(&list->data.list_literal.elements);
            if (!check(p, TOK_RBRACKET)) {
                do {
                    node_list_add(&list->data.list_literal.elements, parse_expr(p));
                } while (match(p, TOK_COMMA));
            }
            consume(p, TOK_RBRACKET, "Expected ']'");
            return list;
        }

        case TOK_LBRACE: {
            advance(p);
            Node *dict = node_create(NODE_DICT_LITERAL, t.line, t.column);
            node_list_init(&dict->data.dict_literal.keys);
            node_list_init(&dict->data.dict_literal.values);
            if (!check(p, TOK_RBRACE)) {
                do {
                    node_list_add(&dict->data.dict_literal.keys, parse_expr(p));
                    consume(p, TOK_COLON, "Expected ':' in dict literal");
                    node_list_add(&dict->data.dict_literal.values, parse_expr(p));
                } while (match(p, TOK_COMMA));
            }
            consume(p, TOK_RBRACE, "Expected '}'");
            return dict;
        }

        case TOK_MINUS: {
            advance(p);
            Node *n = node_create(NODE_UNARY, t.line, t.column);
            n->data.unary.op = TOK_MINUS;
            n->data.unary.operand = parse_primary(p);
            n->data.unary.is_prefix = 1;
            return n;
        }

        case TOK_BANG: {
            advance(p);
            Node *n = node_create(NODE_UNARY, t.line, t.column);
            n->data.unary.op = TOK_BANG;
            n->data.unary.operand = parse_primary(p);
            n->data.unary.is_prefix = 1;
            return n;
        }

        case TOK_UNDERSCORE: {
            advance(p);
            return node_make_ident(t.line, t.column, "_");
        }

        case TOK_SELF: {
            advance(p);
            return node_make_ident(t.line, t.column, "self");
        }

        case TOK_SUPER: {
            advance(p);
            Node *n = node_make_ident(t.line, t.column, "super");
            if (match(p, TOK_DOT)) {
                Token mem = consume(p, TOK_IDENTIFIER, "Expected member name");
                Node *member = node_create(NODE_MEMBER, t.line, t.column);
                member->data.member_expr.object = n;
                member->data.member_expr.member = str_ndup(mem.start, mem.length);
                return member;
            }
            return n;
        }

        default:
            diag_add(p->diags, DIAG_ERROR, t.line, t.column,
                     "Unexpected token '%s'", token_type_name(t.type));
            p->had_error = 1;
            advance(p);
            return NULL;
    }
}

static int get_precedence(TokenType type) {
    switch (type) {
        case TOK_OR: case TOK_PIPE_PIPE: return 1;
        case TOK_AND: case TOK_AMPERSAND_AMPERSAND: return 2;
        case TOK_PIPE: return 3;
        case TOK_CARET: return 4;
        case TOK_AMPERSAND: return 5;
        case TOK_EQ_EQ: case TOK_BANG_EQ: return 6;
        case TOK_LT: case TOK_GT: case TOK_LT_EQ: case TOK_GT_EQ: return 7;
        case TOK_LSHIFT: case TOK_RSHIFT: return 8;
        case TOK_PLUS: case TOK_MINUS: return 9;
        case TOK_STAR: case TOK_SLASH: case TOK_PERCENT: return 10;
        default: return -1;
    }
}

static Node *parse_binary(Parser *p, int min_prec) {
    Node *left = parse_primary(p);
    if (!left) return NULL;

    while (1) {
        TokenType op = peek(p).type;
        int prec = get_precedence(op);
        if (prec < min_prec) break;

        if (op == TOK_AND) {
            advance(p);
            Node *node = node_create(NODE_BINARY, left->line, left->column);
            node->data.binary.op = TOK_AND;
            node->data.binary.left = left;
            node->data.binary.right = parse_binary(p, prec + 1);
            left = node;
            continue;
        }

        if (op == TOK_OR) {
            advance(p);
            Node *node = node_create(NODE_BINARY, left->line, left->column);
            node->data.binary.op = TOK_OR;
            node->data.binary.left = left;
            node->data.binary.right = parse_binary(p, prec + 1);
            left = node;
            continue;
        }

        switch (op) {
            case TOK_PLUS: case TOK_MINUS:
            case TOK_STAR: case TOK_SLASH: case TOK_PERCENT:
            case TOK_EQ_EQ: case TOK_BANG_EQ:
            case TOK_LT: case TOK_GT: case TOK_LT_EQ: case TOK_GT_EQ:
            case TOK_LSHIFT: case TOK_RSHIFT:
            case TOK_PIPE: case TOK_CARET: case TOK_AMPERSAND:
            case TOK_PIPE_PIPE: case TOK_AMPERSAND_AMPERSAND: {
                advance(p);
                Node *node = node_create(NODE_BINARY, left->line, left->column);
                node->data.binary.op = op;
                node->data.binary.left = left;
                node->data.binary.right = parse_binary(p, prec + 1);
                left = node;
                break;
            }
            case TOK_IS: {
                advance(p);
                Node *node = node_create(NODE_IS_EXPR, left->line, left->column);
                node->data.is_expr.expr = left;
                if (check(p, TOK_IDENTIFIER)) {
                    Token tn = advance(p);
                    node->data.is_expr.target_type = type_create_named(
                        str_ndup(tn.start, tn.length));
                }
                left = node;
                break;
            }
            case TOK_AS: {
                advance(p);
                Node *node = node_create(NODE_AS_EXPR, left->line, left->column);
                node->data.as_expr.expr = left;
                if (check(p, TOK_IDENTIFIER)) {
                    Token tn = advance(p);
                    node->data.as_expr.target_type = type_create_named(
                        str_ndup(tn.start, tn.length));
                }
                left = node;
                break;
            }
            case TOK_DOT: {
                advance(p);
                if (check(p, TOK_IDENTIFIER)) {
                    Token mem = advance(p);
                    Node *member = node_create(NODE_MEMBER, left->line, left->column);
                    member->data.member_expr.object = left;
                    member->data.member_expr.member = str_ndup(mem.start, mem.length);

                    if (match(p, TOK_LPAREN)) {
                        Node *call = node_create(NODE_CALL, mem.line, mem.column);
                        call->data.call.callee = member;
                        node_list_init(&call->data.call.args);
                        if (!check(p, TOK_RPAREN)) {
                            do {
                                node_list_add(&call->data.call.args, parse_expr(p));
                            } while (match(p, TOK_COMMA));
                        }
                        consume(p, TOK_RPAREN, "Expected ')'");
                        left = call;
                    } else {
                        left = member;
                    }
                }
                break;
            }
            case TOK_LBRACKET: {
                advance(p);
                Node *index = node_create(NODE_INDEX, left->line, left->column);
                index->data.index_expr.target = left;
                index->data.index_expr.index = parse_expr(p);
                consume(p, TOK_RBRACKET, "Expected ']'");
                left = index;
                break;
            }
            default:
                goto done;
        }
    }

done:
    return left;
}

static Node *parse_expr(Parser *p) {
    return parse_binary(p, 0);
}

Node *parser_parse(Parser *p) {
    Node *module = node_create(NODE_MODULE, 1, 1);
    module->data.module.name = NULL;
    node_list_init(&module->data.module.imports);
    node_list_init(&module->data.module.statements);

    while (!check(p, TOK_EOF)) {
        if (match(p, TOK_NEWLINE)) continue;

        if (check(p, TOK_IMPORT)) {
            Node *imp = parse_import(p);
            if (imp) {
                node_list_add(&module->data.module.imports, imp);
            }
            match(p, TOK_NEWLINE);
            continue;
        }

        Node *stmt = parse_stmt(p);
        if (stmt) {
            node_list_add(&module->data.module.statements, stmt);
        } else if (!p->had_error && !check(p, TOK_EOF)) {
            advance(p);
        }

        while (match(p, TOK_NEWLINE));
    }

    return module;
}
