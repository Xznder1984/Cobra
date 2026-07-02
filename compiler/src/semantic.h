#ifndef COBRA_SEMANTIC_H
#define COBRA_SEMANTIC_H

#include "ast.h"

typedef struct Symbol {
    char *name;
    Type *type;
    int is_mut;
    int is_const;
    int is_function;
    Node *node;
    struct Symbol *next;
} Symbol;

typedef struct Scope {
    Symbol *symbols;
    struct Scope *parent;
    int depth;
} Scope;

typedef struct {
    Scope *current_scope;
    DiagnosticList *diags;
    int had_error;
} SemanticAnalyzer;

SemanticAnalyzer *semantic_create(DiagnosticList *diags);
void semantic_free(SemanticAnalyzer *sa);
int semantic_analyze(SemanticAnalyzer *sa, Node *node);

Scope *scope_push(SemanticAnalyzer *sa);
void scope_pop(SemanticAnalyzer *sa);
void scope_add_symbol(SemanticAnalyzer *sa, const char *name, Type *type, int is_mut, int is_const, Node *node);
Symbol *scope_lookup(SemanticAnalyzer *sa, const char *name);

#endif
