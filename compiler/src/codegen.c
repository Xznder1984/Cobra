#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

CodeGenerator *cg_create(DiagnosticList *diags) {
    CodeGenerator *cg = calloc(1, sizeof(CodeGenerator));
    cg->diags = diags;
    cg->output = NULL;
    cg->label_count = 0;
    cg->stack_offset = 0;
    cg->had_error = 0;
    cg->ir = ir_module_create();
    cg->strings = NULL;
    cg->string_count = 0;
    cg->string_capacity = 0;
    cg->str_label_count = 0;
    cg->var_names = NULL;
    cg->var_offsets = NULL;
    cg->var_count = 0;
    cg->var_capacity = 0;
    cg->current_fn_var_base = 16;
    cg->fn_end_label = -1;
    return cg;
}

void cg_free(CodeGenerator *cg) {
    if (cg) {
        if (cg->output) fclose(cg->output);
        ir_module_free(cg->ir);
        for (int i = 0; i < cg->string_count; i++) free(cg->strings[i]);
        free(cg->strings);
        for (int i = 0; i < cg->var_count; i++) free(cg->var_names[i]);
        free(cg->var_names);
        free(cg->var_offsets);
        free(cg);
    }
}

static int cg_gen_node(CodeGenerator *cg, Node *node);
static int cg_gen_expr(CodeGenerator *cg, Node *node);

static int cg_find_var(CodeGenerator *cg, const char *name) {
    for (int i = cg->var_count - 1; i >= 0; i--) {
        if (strcmp(cg->var_names[i], name) == 0) return cg->var_offsets[i];
    }
    return -1;
}

static int cg_add_var(CodeGenerator *cg, const char *name) {
    if (cg->var_count >= cg->var_capacity) {
        cg->var_capacity = cg->var_capacity ? cg->var_capacity * 2 : 8;
        cg->var_names = realloc(cg->var_names, sizeof(char*) * cg->var_capacity);
        cg->var_offsets = realloc(cg->var_offsets, sizeof(int) * cg->var_capacity);
    }
    int offset = cg->current_fn_var_base + cg->var_count * 8;
    cg->var_names[cg->var_count] = strdup(name);
    cg->var_offsets[cg->var_count] = offset;
    return cg->var_count++;
}

static void cg_reset_vars(CodeGenerator *cg) {
    for (int i = 0; i < cg->var_count; i++) free(cg->var_names[i]);
    cg->var_count = 0;
}

static int cg_count_let_decls(Node *node) {
    int count = 0;
    if (!node) return 0;
    if (node->type == NODE_LET_DECL) return 1;
    if (node->type == NODE_FOR) return 1;
    if (node->type == NODE_BLOCK) {
        for (int i = 0; i < node->data.block.statements.count; i++) {
            count += cg_count_let_decls(node->data.block.statements.items[i]);
        }
        return count;
    }
    if (node->type == NODE_FN_DEF) {
        for (int i = 0; i < node->data.fn_def.body.count; i++) {
            count += cg_count_let_decls(node->data.fn_def.body.items[i]);
        }
        return count;
    }
    return 0;
}

static int cg_gen_module(CodeGenerator *cg, Node *node) {
    for (int i = 0; i < node->data.module.statements.count; i++) {
        cg_gen_node(cg, node->data.module.statements.items[i]);
    }
    return 1;
}

static int cg_gen_fn_def(CodeGenerator *cg, Node *node) {
    if (node->data.fn_def.is_extern) {
        fprintf(cg->output, "\n# Extern function %s (resolved by linker)\n", node->data.fn_def.name);
        return 1;
    }

    cg_reset_vars(cg);
    cg->fn_end_label = cg->label_count++;

    int let_count = cg_count_let_decls(node);
    int stack_size = 16 + 8 * let_count;
    if (stack_size % 16 != 0) stack_size += 8;

    fprintf(cg->output, "\n# Function %s\n", node->data.fn_def.name);
    fprintf(cg->output, ".text\n");
    fprintf(cg->output, ".globl _%s\n", node->data.fn_def.name);
    fprintf(cg->output, "_%s:\n", node->data.fn_def.name);

    fprintf(cg->output, "    push rbp\n");
    fprintf(cg->output, "    mov rbp, rsp\n");
    fprintf(cg->output, "    sub rsp, %d\n", stack_size);

    for (int i = 0; i < node->data.fn_def.params.count && i < 6; i++) {
        static const char *regs64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
        fprintf(cg->output, "    mov [rbp - %d], %s\n", (i + 1) * 8, regs64[i]);
    }

    for (int i = 0; i < node->data.fn_def.body.count; i++) {
        cg_gen_node(cg, node->data.fn_def.body.items[i]);
    }

    fprintf(cg->output, ".Lfunc_end_%d:\n", cg->fn_end_label);
    fprintf(cg->output, "    mov rsp, rbp\n");
    fprintf(cg->output, "    pop rbp\n");
    fprintf(cg->output, "    ret\n");
    return 1;
}

static int cg_gen_return(CodeGenerator *cg, Node *node) {
    if (node->data.return_stmt.value) {
        cg_gen_expr(cg, node->data.return_stmt.value);
    } else {
        fprintf(cg->output, "    xor rax, rax\n");
    }
    if (cg->fn_end_label >= 0) {
        fprintf(cg->output, "    jmp .Lfunc_end_%d\n", cg->fn_end_label);
    }
    return 1;
}

static int cg_gen_if(CodeGenerator *cg, Node *node) {
    int label_else = cg->label_count++;
    int label_end = cg->label_count++;

    if (node->data.if_stmt.condition) {
        cg_gen_expr(cg, node->data.if_stmt.condition);
    }

    fprintf(cg->output, "    cmp rax, 0\n");
    fprintf(cg->output, "    je .L%d\n", label_else);

    if (node->data.if_stmt.then_block) {
        cg_gen_node(cg, node->data.if_stmt.then_block);
    }

    fprintf(cg->output, "    jmp .L%d\n", label_end);
    fprintf(cg->output, ".L%d:\n", label_else);

    if (node->data.if_stmt.elif_chain) {
        cg_gen_node(cg, node->data.if_stmt.elif_chain);
    } else if (node->data.if_stmt.else_block) {
        cg_gen_node(cg, node->data.if_stmt.else_block);
    }

    fprintf(cg->output, ".L%d:\n", label_end);
    return 1;
}

static int cg_gen_while(CodeGenerator *cg, Node *node) {
    int label_start = cg->label_count++;
    int label_end = cg->label_count++;

    fprintf(cg->output, ".L%d:\n", label_start);
    if (node->data.while_loop.condition) {
        cg_gen_expr(cg, node->data.while_loop.condition);
    }
    fprintf(cg->output, "    cmp rax, 0\n");
    fprintf(cg->output, "    je .L%d\n", label_end);

    if (node->data.while_loop.body) {
        cg_gen_node(cg, node->data.while_loop.body);
    }

    fprintf(cg->output, "    jmp .L%d\n", label_start);
    fprintf(cg->output, ".L%d:\n", label_end);
    return 1;
}

static int cg_gen_for(CodeGenerator *cg, Node *node) {
    const char *var_name = node->data.for_loop.var_name;
    Node *iterable = node->data.for_loop.iterable;

    int label_cond = cg->label_count++;
    int label_end = cg->label_count++;

    if (iterable && iterable->type == NODE_RANGE) {
        cg_add_var(cg, var_name);
        int var_offset = cg->var_offsets[cg->var_count - 1];

        cg_gen_expr(cg, iterable->data.range.start);
        fprintf(cg->output, "    mov [rbp - %d], rax\n", var_offset);

        fprintf(cg->output, ".L%d:\n", label_cond);

        cg_gen_expr(cg, iterable->data.range.end);
        fprintf(cg->output, "    mov rcx, rax\n");
        fprintf(cg->output, "    mov rax, [rbp - %d]\n", var_offset);
        fprintf(cg->output, "    cmp rax, rcx\n");
        if (iterable->data.range.inclusive) {
            fprintf(cg->output, "    jg .L%d\n", label_end);
        } else {
            fprintf(cg->output, "    jge .L%d\n", label_end);
        }

        if (node->data.for_loop.body) {
            cg_gen_node(cg, node->data.for_loop.body);
        }

        fprintf(cg->output, "    mov rax, [rbp - %d]\n", var_offset);
        fprintf(cg->output, "    add rax, 1\n");
        fprintf(cg->output, "    mov [rbp - %d], rax\n", var_offset);
        fprintf(cg->output, "    jmp .L%d\n", label_cond);
        fprintf(cg->output, ".L%d:\n", label_end);
    } else {
        cg_gen_expr(cg, iterable);

        fprintf(cg->output, ".L%d:\n", label_cond);
        if (node->data.for_loop.body) {
            cg_gen_node(cg, node->data.for_loop.body);
        }
        fprintf(cg->output, "    jmp .L%d\n", label_cond);
        fprintf(cg->output, ".L%d:\n", label_end);
    }
    return 1;
}

static int cg_gen_block(CodeGenerator *cg, Node *node) {
    for (int i = 0; i < node->data.block.statements.count; i++) {
        cg_gen_node(cg, node->data.block.statements.items[i]);
    }
    return 1;
}

static int cg_gen_expr_stmt(CodeGenerator *cg, Node *node) {
    if (node->data.expr_stmt.expr) {
        return cg_gen_expr(cg, node->data.expr_stmt.expr);
    }
    return 1;
}

static int cg_gen_call(CodeGenerator *cg, Node *node);

static int cg_gen_binary(CodeGenerator *cg, Node *node) {
    cg_gen_expr(cg, node->data.binary.left);
    fprintf(cg->output, "    mov [rbp - 8], rax\n");

    cg_gen_expr(cg, node->data.binary.right);
    fprintf(cg->output, "    mov rcx, rax\n");
    fprintf(cg->output, "    mov rax, [rbp - 8]\n");

    switch (node->data.binary.op) {
        case TOK_PLUS:
            fprintf(cg->output, "    add rax, rcx\n");
            break;
        case TOK_MINUS:
            fprintf(cg->output, "    sub rax, rcx\n");
            break;
        case TOK_STAR:
            fprintf(cg->output, "    imul rax, rcx\n");
            break;
        case TOK_SLASH:
            fprintf(cg->output, "    xor rdx, rdx\n");
            fprintf(cg->output, "    div rcx\n");
            break;
        case TOK_EQ_EQ:
            fprintf(cg->output, "    cmp rax, rcx\n");
            fprintf(cg->output, "    sete al\n");
            fprintf(cg->output, "    movzx rax, al\n");
            break;
        case TOK_BANG_EQ:
            fprintf(cg->output, "    cmp rax, rcx\n");
            fprintf(cg->output, "    setne al\n");
            fprintf(cg->output, "    movzx rax, al\n");
            break;
        case TOK_LT:
            fprintf(cg->output, "    cmp rax, rcx\n");
            fprintf(cg->output, "    setl al\n");
            fprintf(cg->output, "    movzx rax, al\n");
            break;
        case TOK_GT:
            fprintf(cg->output, "    cmp rax, rcx\n");
            fprintf(cg->output, "    setg al\n");
            fprintf(cg->output, "    movzx rax, al\n");
            break;
        case TOK_LT_EQ:
            fprintf(cg->output, "    cmp rax, rcx\n");
            fprintf(cg->output, "    setle al\n");
            fprintf(cg->output, "    movzx rax, al\n");
            break;
        case TOK_GT_EQ:
            fprintf(cg->output, "    cmp rax, rcx\n");
            fprintf(cg->output, "    setge al\n");
            fprintf(cg->output, "    movzx rax, al\n");
            break;
        default:
            fprintf(cg->output, "    # unknown binary op\n");
            break;
    }
    return 1;
}

static int cg_gen_unary(CodeGenerator *cg, Node *node) {
    cg_gen_expr(cg, node->data.unary.operand);

    switch (node->data.unary.op) {
        case TOK_MINUS:
            fprintf(cg->output, "    neg rax\n");
            break;
        case TOK_BANG:
            fprintf(cg->output, "    cmp rax, 0\n");
            fprintf(cg->output, "    sete al\n");
            fprintf(cg->output, "    movzx rax, al\n");
            break;
        default:
            break;
    }
    return 1;
}

static int cg_gen_call(CodeGenerator *cg, Node *node) {
    if (node->data.call.callee->type == NODE_IDENTIFIER) {
        const char *name = node->data.call.callee->data.identifier.name;

        if (strcmp(name, "print") == 0 || strcmp(name, "println") == 0) {
            if (node->data.call.args.count > 0) {
                cg_gen_expr(cg, node->data.call.args.items[0]);
                fprintf(cg->output, "    mov rdi, rax\n");
            } else {
                fprintf(cg->output, "    xor rdi, rdi\n");
            }
            if (strcmp(name, "println") == 0) {
                fprintf(cg->output, "    call _cobra_println\n");
            } else {
                fprintf(cg->output, "    call _cobra_print\n");
            }
            return 1;
        }
        if (strcmp(name, "print_int") == 0) {
            if (node->data.call.args.count > 0) {
                cg_gen_expr(cg, node->data.call.args.items[0]);
                fprintf(cg->output, "    mov rdi, rax\n");
            }
            fprintf(cg->output, "    call _cobra_print_int\n");
            return 1;
        }
        if (strcmp(name, "print_float") == 0) {
            if (node->data.call.args.count > 0) {
                cg_gen_expr(cg, node->data.call.args.items[0]);
            }
            fprintf(cg->output, "    call _cobra_print_float\n");
            return 1;
        }

        for (int i = 0; i < node->data.call.args.count && i < 6; i++) {
            cg_gen_expr(cg, node->data.call.args.items[i]);
            static const char *regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
            fprintf(cg->output, "    mov %s, rax\n", regs[i]);
        }

        fprintf(cg->output, "    call _%s\n", name);
    }
    return 1;
}

static int cg_gen_identifier(CodeGenerator *cg, Node *node) {
    const char *name = node->data.identifier.name;
    if (strcmp(name, "true") == 0) {
        fprintf(cg->output, "    mov rax, 1\n");
        return 1;
    }
    if (strcmp(name, "false") == 0) {
        fprintf(cg->output, "    mov rax, 0\n");
        return 1;
    }
    if (strcmp(name, "nil") == 0) {
        fprintf(cg->output, "    xor rax, rax\n");
        return 1;
    }
    int offset = cg_find_var(cg, name);
    if (offset >= 0) {
        fprintf(cg->output, "    mov rax, [rbp - %d]\n", offset);
        return 1;
    }
    return 1;
}

static int cg_gen_let_decl(CodeGenerator *cg, Node *node) {
    const char *name = node->data.let_decl.name;

    if (node->data.let_decl.value) {
        cg_gen_expr(cg, node->data.let_decl.value);
    } else {
        fprintf(cg->output, "    xor rax, rax\n");
    }

    int idx = cg_add_var(cg, name);
    fprintf(cg->output, "    mov [rbp - %d], rax\n", cg->var_offsets[idx]);
    return 1;
}

static int cg_gen_int_literal(CodeGenerator *cg, Node *node) {
    fprintf(cg->output, "    mov rax, %lld\n", node->data.int_value);
    return 1;
}

static int cg_gen_str_literal(CodeGenerator *cg, Node *node) {
    int label = cg->str_label_count++;
    fprintf(cg->output, "    lea rax, [rip + .Lstr%d]\n", label);
    if (cg->string_count >= cg->string_capacity) {
        cg->string_capacity = cg->string_capacity ? cg->string_capacity * 2 : 8;
        cg->strings = realloc(cg->strings, sizeof(char*) * cg->string_capacity);
    }
    cg->strings[cg->string_count++] = strdup(node->data.string_value);
    return 1;
}

static int cg_gen_expr(CodeGenerator *cg, Node *node) {
    if (!node) return 0;
    switch (node->type) {
        case NODE_INT_LITERAL: return cg_gen_int_literal(cg, node);
        case NODE_STR_LITERAL: return cg_gen_str_literal(cg, node);
        case NODE_BOOL_LITERAL:
            fprintf(cg->output, "    mov rax, %d\n", node->data.bool_value);
            return 1;
        case NODE_NIL_LITERAL:
            fprintf(cg->output, "    xor rax, rax\n");
            return 1;
        case NODE_IDENTIFIER: return cg_gen_identifier(cg, node);
        case NODE_ASSIGN: {
            Node *target = node->data.assign.target;
            if (target && target->type == NODE_IDENTIFIER) {
                int offset = cg_find_var(cg, target->data.identifier.name);
                if (offset >= 0) {
                    cg_gen_expr(cg, node->data.assign.value);
                    fprintf(cg->output, "    mov [rbp - %d], rax\n", offset);
                }
            }
            return 1;
        }
        case NODE_BINARY: return cg_gen_binary(cg, node);
        case NODE_UNARY: return cg_gen_unary(cg, node);
        case NODE_CALL: return cg_gen_call(cg, node);
        default: return 0;
    }
}

static int cg_gen_node(CodeGenerator *cg, Node *node) {
    if (!node) return 1;
    switch (node->type) {
        case NODE_MODULE: return cg_gen_module(cg, node);
        case NODE_FN_DEF: return cg_gen_fn_def(cg, node);
        case NODE_RETURN: return cg_gen_return(cg, node);
        case NODE_IF: return cg_gen_if(cg, node);
        case NODE_WHILE: return cg_gen_while(cg, node);
        case NODE_FOR: return cg_gen_for(cg, node);
        case NODE_BLOCK: return cg_gen_block(cg, node);
        case NODE_EXPR_STMT: return cg_gen_expr_stmt(cg, node);
        case NODE_LET_DECL: return cg_gen_let_decl(cg, node);
        case NODE_BREAK:
        case NODE_CONTINUE:
            return 1;
        default:
            return cg_gen_expr(cg, node) || 1;
    }
}

int cg_generate(CodeGenerator *cg, Node *node, const char *output_path) {
    cg->output = fopen(output_path, "w");
    if (!cg->output) {
        diag_add(cg->diags, DIAG_ERROR, 0, 0,
                 "Could not open output file '%s'", output_path);
        cg->had_error = 1;
        return 0;
    }

    time_t now;
    time(&now);
    fprintf(cg->output, "# Generated by Cobra Compiler v%s\n", COBRA_VERSION);
    fprintf(cg->output, "# Date: %s", ctime(&now));
    fprintf(cg->output, ".intel_syntax noprefix\n");

    int result = cg_gen_node(cg, node);

    if (cg->string_count > 0) {
        fprintf(cg->output, "\n.cstring\n");
        for (int i = 0; i < cg->string_count; i++) {
            fprintf(cg->output, ".Lstr%d:\n", i);
            if (cg->strings[i][0] == '\0') {
                fprintf(cg->output, "    .byte 0\n");
            } else {
                fprintf(cg->output, "    .asciz \"%s\"\n", cg->strings[i]);
            }
        }
    }

    fprintf(cg->output, "\n");
    fclose(cg->output);
    cg->output = NULL;

    return result;
}
