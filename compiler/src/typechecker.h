#ifndef COBRA_TYPECHECKER_H
#define COBRA_TYPECHECKER_H

#include "ast.h"

typedef struct {
    DiagnosticList *diags;
    int had_error;
} TypeChecker;

TypeChecker *tc_create(DiagnosticList *diags);
void tc_free(TypeChecker *tc);
int tc_check(TypeChecker *tc, Node *node);

#endif
