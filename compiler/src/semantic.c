#include "semantic.h"
#include <stdlib.h>
#include <string.h>

SemanticAnalyzer *semantic_create(DiagnosticList *diags) {
    SemanticAnalyzer *sa = calloc(1, sizeof(SemanticAnalyzer));
    sa->diags = diags;
    sa->had_error = 0;
    sa->current_scope = calloc(1, sizeof(Scope));
    sa->current_scope->symbols = NULL;
    sa->current_scope->parent = NULL;
    sa->current_scope->depth = 0;
    return sa;
}

void semantic_free(SemanticAnalyzer *sa) {
    if (!sa) return;
    while (sa->current_scope) {
        Scope *parent = sa->current_scope->parent;
        Symbol *s = sa->current_scope->symbols;
        while (s) {
            Symbol *next = s->next;
            free(s->name);
            free(s);
            s = next;
        }
        free(sa->current_scope);
        sa->current_scope = parent;
    }
    free(sa);
}

Scope *scope_push(SemanticAnalyzer *sa) {
    Scope *new_scope = calloc(1, sizeof(Scope));
    new_scope->symbols = NULL;
    new_scope->parent = sa->current_scope;
    new_scope->depth = sa->current_scope->depth + 1;
    sa->current_scope = new_scope;
    return new_scope;
}

void scope_pop(SemanticAnalyzer *sa) {
    if (sa->current_scope && sa->current_scope->parent) {
        Scope *old = sa->current_scope;
        sa->current_scope = old->parent;
        Symbol *s = old->symbols;
        while (s) {
            Symbol *next = s->next;
            free(s->name);
            free(s);
            s = next;
        }
        free(old);
    }
}

void scope_add_symbol(SemanticAnalyzer *sa, const char *name, Type *type, int is_mut, int is_const, Node *node) {
    Symbol *existing = scope_lookup(sa, name);
    if (existing && existing->node->line == node->line) {
        diag_add(sa->diags, DIAG_ERROR, node->line, node->column,
                 "Duplicate symbol '%s'", name);
        sa->had_error = 1;
        return;
    }

    Symbol *sym = calloc(1, sizeof(Symbol));
    sym->name = str_dup(name);
    sym->type = type ? type_clone(type) : NULL;
    sym->is_mut = is_mut;
    sym->is_const = is_const;
    sym->is_function = (node && node->type == NODE_FN_DEF);
    sym->node = node;
    sym->next = sa->current_scope->symbols;
    sa->current_scope->symbols = sym;
}

Symbol *scope_lookup(SemanticAnalyzer *sa, const char *name) {
    Scope *scope = sa->current_scope;
    while (scope) {
        Symbol *s = scope->symbols;
        while (s) {
            if (strcmp(s->name, name) == 0) return s;
            s = s->next;
        }
        scope = scope->parent;
    }
    return NULL;
}

static int semantic_analyze_node(SemanticAnalyzer *sa, Node *node);

static int semantic_analyze_block(SemanticAnalyzer *sa, NodeList *stmts) {
    scope_push(sa);
    for (int i = 0; i < stmts->count; i++) {
        if (!semantic_analyze_node(sa, stmts->items[i])) {
            sa->had_error = 1;
        }
    }
    scope_pop(sa);
    return !sa->had_error;
}

static int semantic_analyze_fn_def(SemanticAnalyzer *sa, Node *node) {
    scope_add_symbol(sa, node->data.fn_def.name, NULL, 0, 1, node);

    scope_push(sa);

    for (int i = 0; i < node->data.fn_def.params.count; i++) {
        Node *param = node->data.fn_def.params.items[i];
        Type *ptype = param->data.param.type;
        scope_add_symbol(sa, param->data.param.name, ptype, 1, 0, param);
    }

    if (node->data.fn_def.body.count > 0) {
        for (int i = 0; i < node->data.fn_def.body.count; i++) {
            semantic_analyze_node(sa, node->data.fn_def.body.items[i]);
        }
    }

    scope_pop(sa);
    return !sa->had_error;
}

static int semantic_analyze_let_decl(SemanticAnalyzer *sa, Node *node) {
    if (node->data.let_decl.value) {
        semantic_analyze_node(sa, node->data.let_decl.value);
    }
    scope_add_symbol(sa, node->data.let_decl.name,
                     node->data.let_decl.type,
                     node->data.let_decl.is_mut, 0, node);
    return !sa->had_error;
}

static int semantic_analyze_const_decl(SemanticAnalyzer *sa, Node *node) {
    if (node->data.const_decl.value) {
        semantic_analyze_node(sa, node->data.const_decl.value);
    }
    scope_add_symbol(sa, node->data.const_decl.name,
                     node->data.const_decl.type, 0, 1, node);
    return !sa->had_error;
}

static int semantic_analyze_if(SemanticAnalyzer *sa, Node *node) {
    if (node->data.if_stmt.condition)
        semantic_analyze_node(sa, node->data.if_stmt.condition);
    if (node->data.if_stmt.then_block)
        semantic_analyze_node(sa, node->data.if_stmt.then_block);
    if (node->data.if_stmt.elif_chain)
        semantic_analyze_node(sa, node->data.if_stmt.elif_chain);
    if (node->data.if_stmt.else_block)
        semantic_analyze_node(sa, node->data.if_stmt.else_block);
    return !sa->had_error;
}

static int semantic_analyze_while(SemanticAnalyzer *sa, Node *node) {
    if (node->data.while_loop.condition)
        semantic_analyze_node(sa, node->data.while_loop.condition);
    if (node->data.while_loop.body)
        semantic_analyze_node(sa, node->data.while_loop.body);
    return !sa->had_error;
}

static int semantic_analyze_for(SemanticAnalyzer *sa, Node *node) {
    if (node->data.for_loop.iterable)
        semantic_analyze_node(sa, node->data.for_loop.iterable);
    scope_push(sa);
    scope_add_symbol(sa, node->data.for_loop.var_name, type_create(TYPE_INT), 0, 0, node);
    if (node->data.for_loop.body)
        semantic_analyze_node(sa, node->data.for_loop.body);
    scope_pop(sa);
    return !sa->had_error;
}

static int semantic_analyze_return(SemanticAnalyzer *sa, Node *node) {
    if (node->data.return_stmt.value)
        semantic_analyze_node(sa, node->data.return_stmt.value);
    return !sa->had_error;
}

static int semantic_analyze_identifier(SemanticAnalyzer *sa, Node *node) {
    const char *name = node->data.identifier.name;
    if (strcmp(name, "true") == 0 || strcmp(name, "false") == 0 ||
        strcmp(name, "nil") == 0 || strcmp(name, "_") == 0) {
        return !sa->had_error;
    }

    Symbol *sym = scope_lookup(sa, name);
    if (!sym) {
        diag_add(sa->diags, DIAG_ERROR, node->line, node->column,
                 "Undefined symbol '%s'", name);
        sa->had_error = 1;
        return 0;
    }
    if (sym->type) {
        node->expr_type = type_clone(sym->type);
    }
    return !sa->had_error;
}

static int semantic_analyze_binary(SemanticAnalyzer *sa, Node *node) {
    if (node->data.binary.left)
        semantic_analyze_node(sa, node->data.binary.left);
    if (node->data.binary.right)
        semantic_analyze_node(sa, node->data.binary.right);
    return !sa->had_error;
}

static int semantic_analyze_call(SemanticAnalyzer *sa, Node *node) {
    if (node->data.call.callee && node->data.call.callee->type == NODE_IDENTIFIER) {
        const char *name = node->data.call.callee->data.identifier.name;
        if (strcmp(name, "print") == 0 || strcmp(name, "println") == 0 ||
            strcmp(name, "print_int") == 0 || strcmp(name, "print_float") == 0) {
            for (int i = 0; i < node->data.call.args.count; i++) {
                semantic_analyze_node(sa, node->data.call.args.items[i]);
            }
            return !sa->had_error;
        }
    }
    if (node->data.call.callee)
        semantic_analyze_node(sa, node->data.call.callee);
    for (int i = 0; i < node->data.call.args.count; i++) {
        semantic_analyze_node(sa, node->data.call.args.items[i]);
    }
    return !sa->had_error;
}

static int semantic_analyze_node(SemanticAnalyzer *sa, Node *node) {
    if (!node) return 1;

    switch (node->type) {
        case NODE_MODULE:
            return semantic_analyze_block(sa, &node->data.module.statements);
        case NODE_FN_DEF:
            return semantic_analyze_fn_def(sa, node);
        case NODE_LET_DECL:
        case NODE_VAR_DECL:
            return semantic_analyze_let_decl(sa, node);
        case NODE_ASSIGN:
            if (node->data.assign.target &&
                node->data.assign.target->type == NODE_IDENTIFIER) {
                const char *name = node->data.assign.target->data.identifier.name;
                Symbol *sym = scope_lookup(sa, name);
                if (!sym) {
                    scope_add_symbol(sa, name, NULL, 1, 0, node);
                }
            }
            if (node->data.assign.target)
                semantic_analyze_node(sa, node->data.assign.target);
            if (node->data.assign.value)
                semantic_analyze_node(sa, node->data.assign.value);
            return !sa->had_error;
        case NODE_CONST_DECL:
            return semantic_analyze_const_decl(sa, node);
        case NODE_IDENTIFIER:
            return semantic_analyze_identifier(sa, node);
        case NODE_BINARY:
            return semantic_analyze_binary(sa, node);
        case NODE_UNARY:
            if (node->data.unary.operand)
                return semantic_analyze_node(sa, node->data.unary.operand);
            return !sa->had_error;
        case NODE_CALL:
            return semantic_analyze_call(sa, node);
        case NODE_IF:
            return semantic_analyze_if(sa, node);
        case NODE_WHILE:
            return semantic_analyze_while(sa, node);
        case NODE_FOR:
            return semantic_analyze_for(sa, node);
        case NODE_RETURN:
            return semantic_analyze_return(sa, node);
        case NODE_BREAK:
        case NODE_CONTINUE:
            return !sa->had_error;
        case NODE_BLOCK:
            return semantic_analyze_block(sa, &node->data.block.statements);
        case NODE_EXPR_STMT:
            if (node->data.expr_stmt.expr)
                return semantic_analyze_node(sa, node->data.expr_stmt.expr);
            return !sa->had_error;
        case NODE_INT_LITERAL:
        case NODE_FLOAT_LITERAL:
        case NODE_STR_LITERAL:
        case NODE_BOOL_LITERAL:
        case NODE_NIL_LITERAL:
            return !sa->had_error;
        case NODE_STRUCT_DEF:
        case NODE_CLASS_DEF:
        case NODE_ENUM_DEF:
        case NODE_TRAIT_DEF:
            return !sa->had_error;
        case NODE_IMPORT:
        case NODE_USE_DECL:
            return !sa->had_error;

        case NODE_STRUCT_INIT:
            for (int i = 0; i < node->data.struct_init.fields.count; i++) {
                semantic_analyze_node(sa, node->data.struct_init.fields.items[i]);
            }
            return !sa->had_error;
        case NODE_MEMBER:
            if (node->data.member_expr.object)
                semantic_analyze_node(sa, node->data.member_expr.object);
            return !sa->had_error;
        case NODE_INDEX:
            if (node->data.index_expr.target)
                semantic_analyze_node(sa, node->data.index_expr.target);
            if (node->data.index_expr.index)
                semantic_analyze_node(sa, node->data.index_expr.index);
            return !sa->had_error;
        case NODE_MATCH:
            if (node->data.match_stmt.expr)
                semantic_analyze_node(sa, node->data.match_stmt.expr);
            for (int i = 0; i < node->data.match_stmt.arms.count; i++) {
                Node *arm = node->data.match_stmt.arms.items[i];
                for (int j = 0; j < arm->data.match_arm.patterns.count; j++) {
                    semantic_analyze_node(sa, arm->data.match_arm.patterns.items[j]);
                }
                if (arm->data.match_arm.value)
                    semantic_analyze_node(sa, arm->data.match_arm.value);
            }
            return !sa->had_error;
        case NODE_LIST_LITERAL:
        case NODE_TUPLE_LITERAL:
            for (int i = 0; i < node->data.list_literal.elements.count; i++) {
                semantic_analyze_node(sa, node->data.list_literal.elements.items[i]);
            }
            return !sa->had_error;
        case NODE_LAMBDA:
            scope_push(sa);
            for (int i = 0; i < node->data.lambda.params.count; i++) {
                Node *p = node->data.lambda.params.items[i];
                scope_add_symbol(sa, p->data.param.name, p->data.param.type, 1, 0, p);
            }
            if (node->data.lambda.body)
                semantic_analyze_node(sa, node->data.lambda.body);
            scope_pop(sa);
            return !sa->had_error;
        default:
            return !sa->had_error;
    }
}

int semantic_analyze(SemanticAnalyzer *sa, Node *node) {
    return semantic_analyze_node(sa, node);
}
