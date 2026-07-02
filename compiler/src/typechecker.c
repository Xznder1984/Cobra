#include "typechecker.h"
#include <stdlib.h>
#include <string.h>

TypeChecker *tc_create(DiagnosticList *diags) {
    TypeChecker *tc = calloc(1, sizeof(TypeChecker));
    tc->diags = diags;
    tc->had_error = 0;
    return tc;
}

void tc_free(TypeChecker *tc) {
    free(tc);
}

static Type *tc_check_node(TypeChecker *tc, Node *node);

static Type *tc_check_binary(TypeChecker *tc, Node *node) {
    Type *left = tc_check_node(tc, node->data.binary.left);
    Type *right = tc_check_node(tc, node->data.binary.right);

    if (!left || !right) return type_create(TYPE_INT);

    TokenType op = node->data.binary.op;

    switch (op) {
        case TOK_PLUS: case TOK_MINUS: case TOK_STAR:
        case TOK_SLASH: case TOK_PERCENT:
            if (left->kind == TYPE_F64 || right->kind == TYPE_F64)
                return type_create(TYPE_F64);
            return type_create(TYPE_INT);

        case TOK_EQ_EQ: case TOK_BANG_EQ:
        case TOK_LT: case TOK_GT: case TOK_LT_EQ: case TOK_GT_EQ:
            return type_create(TYPE_BOOL);

        case TOK_AND: case TOK_OR:
            return type_create(TYPE_BOOL);

        default:
            return type_create(TYPE_INT);
    }
}

static Type *tc_check_unary(TypeChecker *tc, Node *node) {
    Type *operand = tc_check_node(tc, node->data.unary.operand);
    if (!operand) return type_create(TYPE_INT);

    if (node->data.unary.op == TOK_BANG)
        return type_create(TYPE_BOOL);
    return type_create(TYPE_INT);
}

static Type *tc_check_call(TypeChecker *tc, Node *node) {
    Type *callee = tc_check_node(tc, node->data.call.callee);
    for (int i = 0; i < node->data.call.args.count; i++) {
        tc_check_node(tc, node->data.call.args.items[i]);
    }

    if (callee && callee->kind == TYPE_FN_TYPE) {
        return callee->subtype ? type_clone(callee->subtype) : type_create(TYPE_VOID);
    }

    return type_create(TYPE_INT);
}

static Type *tc_check_node(TypeChecker *tc, Node *node) {
    if (!node) return NULL;

    switch (node->type) {
        case NODE_INT_LITERAL:
            return type_create(TYPE_INT);
        case NODE_FLOAT_LITERAL:
            return type_create(TYPE_F64);
        case NODE_STR_LITERAL:
            return type_create_named("str");
        case NODE_BOOL_LITERAL:
            return type_create(TYPE_BOOL);
        case NODE_NIL_LITERAL:
            return type_create(TYPE_VOID);

        case NODE_IDENTIFIER:
            if (node->expr_type) return type_clone(node->expr_type);
            return type_create(TYPE_INT);

        case NODE_BINARY:
            return tc_check_binary(tc, node);
        case NODE_UNARY:
            return tc_check_unary(tc, node);
        case NODE_CALL:
            return tc_check_call(tc, node);

        case NODE_IF:
            if (node->data.if_stmt.condition)
                tc_check_node(tc, node->data.if_stmt.condition);
            if (node->data.if_stmt.then_block)
                tc_check_node(tc, node->data.if_stmt.then_block);
            if (node->data.if_stmt.elif_chain)
                tc_check_node(tc, node->data.if_stmt.elif_chain);
            if (node->data.if_stmt.else_block)
                tc_check_node(tc, node->data.if_stmt.else_block);
            return NULL;

        case NODE_WHILE:
            if (node->data.while_loop.condition)
                tc_check_node(tc, node->data.while_loop.condition);
            if (node->data.while_loop.body)
                tc_check_node(tc, node->data.while_loop.body);
            return NULL;

        case NODE_RETURN:
            if (node->data.return_stmt.value)
                return tc_check_node(tc, node->data.return_stmt.value);
            return type_create(TYPE_VOID);

        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.statements.count; i++) {
                tc_check_node(tc, node->data.block.statements.items[i]);
            }
            return NULL;

        case NODE_EXPR_STMT:
            if (node->data.expr_stmt.expr)
                return tc_check_node(tc, node->data.expr_stmt.expr);
            return NULL;

        case NODE_ASSIGN:
            if (node->data.assign.target)
                tc_check_node(tc, node->data.assign.target);
            if (node->data.assign.value)
                tc_check_node(tc, node->data.assign.value);
            return NULL;

        case NODE_MEMBER:
            if (node->data.member_expr.object)
                return tc_check_node(tc, node->data.member_expr.object);
            return type_create(TYPE_INT);

        case NODE_INDEX:
            if (node->data.index_expr.target)
                tc_check_node(tc, node->data.index_expr.target);
            if (node->data.index_expr.index)
                tc_check_node(tc, node->data.index_expr.index);
            return type_create(TYPE_INT);

        case NODE_LIST_LITERAL:
            for (int i = 0; i < node->data.list_literal.elements.count; i++) {
                tc_check_node(tc, node->data.list_literal.elements.items[i]);
            }
            return type_create_named("List");

        case NODE_TUPLE_LITERAL:
            for (int i = 0; i < node->data.tuple_literal.elements.count; i++) {
                tc_check_node(tc, node->data.tuple_literal.elements.items[i]);
            }
            return type_create_named("tuple");

        case NODE_IF_EXPR:
            if (node->data.if_expr.condition)
                tc_check_node(tc, node->data.if_expr.condition);
            if (node->data.if_expr.then_expr)
                return tc_check_node(tc, node->data.if_expr.then_expr);
            if (node->data.if_expr.else_expr)
                return tc_check_node(tc, node->data.if_expr.else_expr);
            return NULL;

        case NODE_LAMBDA:
            for (int i = 0; i < node->data.lambda.params.count; i++) {
                Node *p = node->data.lambda.params.items[i];
                if (p->data.param.default_value)
                    tc_check_node(tc, p->data.param.default_value);
            }
            if (node->data.lambda.body)
                tc_check_node(tc, node->data.lambda.body);
            return type_create_named("fn");

        case NODE_FN_DEF:
            for (int i = 0; i < node->data.fn_def.params.count; i++) {
                Node *p = node->data.fn_def.params.items[i];
                if (p->data.param.default_value)
                    tc_check_node(tc, p->data.param.default_value);
            }
            for (int i = 0; i < node->data.fn_def.body.count; i++) {
                tc_check_node(tc, node->data.fn_def.body.items[i]);
            }
            return node->data.fn_def.return_type
                ? type_clone(node->data.fn_def.return_type)
                : type_create(TYPE_VOID);

        case NODE_STRUCT_DEF:
        case NODE_CLASS_DEF:
        case NODE_ENUM_DEF:
        case NODE_TRAIT_DEF:
        case NODE_IMPORT:
        case NODE_LET_DECL:
        case NODE_CONST_DECL:
        case NODE_VAR_DECL:
        case NODE_BREAK:
        case NODE_CONTINUE:
            return NULL;

        default:
            return NULL;
    }
}

int tc_check(TypeChecker *tc, Node *node) {
    tc_check_node(tc, node);
    return !tc->had_error;
}
