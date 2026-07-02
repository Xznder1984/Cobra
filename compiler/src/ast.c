#include "ast.h"
#include <stdlib.h>
#include <string.h>

Node *node_create(NodeType type, int line, int column) {
    Node *n = calloc(1, sizeof(Node));
    n->type = type;
    n->line = line;
    n->column = column;
    n->expr_type = NULL;
    n->is_constant = 0;
    return n;
}

static void node_list_free_internal(NodeList *list) {
    for (int i = 0; i < list->count; i++) {
        node_free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void node_free(Node *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_MODULE:
            free(node->data.module.name);
            node_list_free_internal(&node->data.module.imports);
            node_list_free_internal(&node->data.module.statements);
            break;
        case NODE_FN_DEF:
            free(node->data.fn_def.name);
            node_list_free_internal(&node->data.fn_def.params);
            node_list_free_internal(&node->data.fn_def.body);
            node_list_free_internal(&node->data.fn_def.type_params);
            if (node->data.fn_def.return_type) type_free(node->data.fn_def.return_type);
            break;
        case NODE_LET_DECL:
            free(node->data.let_decl.name);
            if (node->data.let_decl.type) type_free(node->data.let_decl.type);
            if (node->data.let_decl.value) node_free(node->data.let_decl.value);
            break;
        case NODE_CONST_DECL:
            free(node->data.const_decl.name);
            if (node->data.const_decl.type) type_free(node->data.const_decl.type);
            if (node->data.const_decl.value) node_free(node->data.const_decl.value);
            break;
        case NODE_VAR_DECL:
            free(node->data.let_decl.name);
            if (node->data.let_decl.type) type_free(node->data.let_decl.type);
            if (node->data.let_decl.value) node_free(node->data.let_decl.value);
            break;
        case NODE_RETURN:
            if (node->data.return_stmt.value) node_free(node->data.return_stmt.value);
            break;
        case NODE_BLOCK:
            node_list_free_internal(&node->data.block.statements);
            break;
        case NODE_EXPR_STMT:
            if (node->data.expr_stmt.expr) node_free(node->data.expr_stmt.expr);
            break;
        case NODE_BINARY:
            if (node->data.binary.left) node_free(node->data.binary.left);
            if (node->data.binary.right) node_free(node->data.binary.right);
            break;
        case NODE_UNARY:
            if (node->data.unary.operand) node_free(node->data.unary.operand);
            break;
        case NODE_CALL:
            if (node->data.call.callee) node_free(node->data.call.callee);
            node_list_free_internal(&node->data.call.args);
            break;
        case NODE_IDENTIFIER:
            free(node->data.identifier.name);
            break;
        case NODE_STR_LITERAL:
            free(node->data.string_value);
            break;
        case NODE_IF:
            if (node->data.if_stmt.condition) node_free(node->data.if_stmt.condition);
            if (node->data.if_stmt.then_block) node_free(node->data.if_stmt.then_block);
            if (node->data.if_stmt.elif_chain) node_free(node->data.if_stmt.elif_chain);
            if (node->data.if_stmt.else_block) node_free(node->data.if_stmt.else_block);
            break;
        case NODE_WHILE:
            if (node->data.while_loop.condition) node_free(node->data.while_loop.condition);
            if (node->data.while_loop.body) node_free(node->data.while_loop.body);
            break;
        case NODE_FOR:
            free(node->data.for_loop.var_name);
            if (node->data.for_loop.iterable) node_free(node->data.for_loop.iterable);
            if (node->data.for_loop.body) node_free(node->data.for_loop.body);
            break;
        case NODE_IMPORT:
            free(node->data.import.from_module);
            free(node->data.import.name);
            free(node->data.import.alias);
            break;
        case NODE_STRUCT_DEF:
            free(node->data.struct_def.name);
            node_list_free_internal(&node->data.struct_def.fields);
            break;
        case NODE_ASSIGN:
            if (node->data.assign.target) node_free(node->data.assign.target);
            if (node->data.assign.value) node_free(node->data.assign.value);
            break;
        case NODE_STRUCT_INIT:
            free(node->data.struct_init.type_name);
            node_list_free_internal(&node->data.struct_init.fields);
            break;
        case NODE_MEMBER:
            if (node->data.member_expr.object) node_free(node->data.member_expr.object);
            free(node->data.member_expr.member);
            break;
        case NODE_INDEX:
            if (node->data.index_expr.target) node_free(node->data.index_expr.target);
            if (node->data.index_expr.index) node_free(node->data.index_expr.index);
            break;
        case NODE_MATCH:
            if (node->data.match_stmt.expr) node_free(node->data.match_stmt.expr);
            node_list_free_internal(&node->data.match_stmt.arms);
            break;
        case NODE_LIST_LITERAL:
            node_list_free_internal(&node->data.list_literal.elements);
            break;
        case NODE_TUPLE_LITERAL:
            node_list_free_internal(&node->data.tuple_literal.elements);
            break;
        case NODE_LAMBDA:
            node_list_free_internal(&node->data.lambda.params);
            if (node->data.lambda.body) node_free(node->data.lambda.body);
            if (node->data.lambda.return_type) type_free(node->data.lambda.return_type);
            break;
        case NODE_IF_EXPR:
            if (node->data.if_expr.condition) node_free(node->data.if_expr.condition);
            if (node->data.if_expr.then_expr) node_free(node->data.if_expr.then_expr);
            if (node->data.if_expr.else_expr) node_free(node->data.if_expr.else_expr);
            break;
        default:
            break;
    }

    if (node->expr_type) type_free(node->expr_type);
    free(node);
}

void node_list_init(NodeList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void node_list_add(NodeList *list, Node *node) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity ? list->capacity * 2 : 16;
        list->items = realloc(list->items, list->capacity * sizeof(Node *));
    }
    list->items[list->count++] = node;
}

void node_list_free(NodeList *list) {
    for (int i = 0; i < list->count; i++) {
        node_free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

Type *type_create(TypeKind kind) {
    Type *t = calloc(1, sizeof(Type));
    t->kind = kind;
    t->is_mut = 0;
    t->size = 0;
    return t;
}

Type *type_create_named(const char *name) {
    Type *t = type_create(TYPE_NAMED);
    t->name = str_dup(name);
    return t;
}

Type *type_create_pointer(Type *subtype) {
    Type *t = type_create(TYPE_POINTER);
    t->subtype = subtype;
    t->size = 8;
    return t;
}

Type *type_create_array(Type *subtype, size_t sz) {
    Type *t = type_create(TYPE_ARRAY);
    t->subtype = subtype;
    t->size = sz;
    return t;
}

Type *type_create_fn(Type **params, int count, Type *ret) {
    Type *t = type_create(TYPE_FN_TYPE);
    t->params = params;
    t->param_count = count;
    t->subtype = ret;
    t->size = 8;
    return t;
}

void type_free(Type *type) {
    if (!type) return;
    if (type->subtype) type_free(type->subtype);
    if (type->subtype2) type_free(type->subtype2);
    if (type->params) {
        for (int i = 0; i < type->param_count; i++) {
            type_free(type->params[i]);
        }
        free(type->params);
    }
    free(type->name);
    free(type);
}

Type *type_clone(Type *type) {
    if (!type) return NULL;
    Type *t = calloc(1, sizeof(Type));
    t->kind = type->kind;
    t->is_mut = type->is_mut;
    t->size = type->size;
    if (type->name) t->name = str_dup(type->name);
    if (type->subtype) t->subtype = type_clone(type->subtype);
    if (type->subtype2) t->subtype2 = type_clone(type->subtype2);
    if (type->params) {
        t->params = calloc(type->param_count, sizeof(Type *));
        for (int i = 0; i < type->param_count; i++) {
            t->params[i] = type_clone(type->params[i]);
        }
        t->param_count = type->param_count;
    }
    return t;
}

int type_equal(Type *a, Type *b) {
    if (!a || !b) return a == b;
    if (a->kind != b->kind) return 0;
    if (a->kind == TYPE_NAMED) return strcmp(a->name, b->name) == 0;
    if (a->kind == TYPE_POINTER) return type_equal(a->subtype, b->subtype);
    if (a->kind == TYPE_ARRAY) return a->size == b->size && type_equal(a->subtype, b->subtype);
    if (a->kind == TYPE_FN_TYPE) {
        if (a->param_count != b->param_count) return 0;
        for (int i = 0; i < a->param_count; i++) {
            if (!type_equal(a->params[i], b->params[i])) return 0;
        }
        return type_equal(a->subtype, b->subtype);
    }
    return 1;
}

size_t type_size(Type *type) {
    if (!type) return 0;
    if (type->size > 0) return type->size;
    switch (type->kind) {
        case TYPE_VOID: return 0;
        case TYPE_BOOL: return 1;
        case TYPE_I8: case TYPE_U8: return 1;
        case TYPE_I16: case TYPE_U16: return 2;
        case TYPE_INT: case TYPE_I32: case TYPE_U32: case TYPE_F32: return 4;
        case TYPE_I64: case TYPE_U64: case TYPE_F64: return 8;
        case TYPE_STR: case TYPE_CHAR: return 8;
        case TYPE_POINTER: return 8;
        case TYPE_NAMED: return 8;
        default: return 8;
    }
}

Node *node_make_int(int line, int col, long long val) {
    Node *n = node_create(NODE_INT_LITERAL, line, col);
    n->data.int_value = val;
    n->expr_type = type_create(TYPE_INT);
    n->is_constant = 1;
    return n;
}

Node *node_make_float(int line, int col, double val) {
    Node *n = node_create(NODE_FLOAT_LITERAL, line, col);
    n->data.float_value = val;
    n->expr_type = type_create(TYPE_F64);
    n->is_constant = 1;
    return n;
}

Node *node_make_str(int line, int col, const char *val) {
    Node *n = node_create(NODE_STR_LITERAL, line, col);
    n->data.string_value = str_dup(val);
    n->expr_type = type_create_named("str");
    n->is_constant = 1;
    return n;
}

Node *node_make_bool(int line, int col, int val) {
    Node *n = node_create(NODE_BOOL_LITERAL, line, col);
    n->data.bool_value = val;
    n->expr_type = type_create(TYPE_BOOL);
    n->is_constant = 1;
    return n;
}

Node *node_make_nil(int line, int col) {
    Node *n = node_create(NODE_NIL_LITERAL, line, col);
    return n;
}

Node *node_make_ident(int line, int col, const char *name) {
    Node *n = node_create(NODE_IDENTIFIER, line, col);
    n->data.identifier.name = str_dup(name);
    return n;
}
