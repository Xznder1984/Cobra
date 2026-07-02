#include "ir.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

IRValue ir_val_int(long long v) {
    IRValue val;
    val.kind = VAL_INT;
    val.int_val = v;
    val.str_val = NULL;
    val.reg_index = -1;
    return val;
}

IRValue ir_val_str(const char *s) {
    IRValue val;
    val.kind = VAL_STR;
    val.int_val = 0;
    val.str_val = str_dup(s);
    val.reg_index = -1;
    return val;
}

IRValue ir_val_reg(int reg) {
    IRValue val;
    val.kind = VAL_REG;
    val.int_val = 0;
    val.str_val = NULL;
    val.reg_index = reg;
    return val;
}

IRFunction *ir_func_create(const char *name) {
    IRFunction *f = calloc(1, sizeof(IRFunction));
    f->name = str_dup(name);
    f->insn_capacity = 64;
    f->insns = calloc(f->insn_capacity, sizeof(IRInsn));
    f->insn_count = 0;
    f->reg_count = 0;
    return f;
}

void ir_func_free(IRFunction *f) {
    if (!f) return;
    free(f->name);
    for (int i = 0; i < f->insn_count; i++) {
        free(f->insns[i].comment);
    }
    free(f->insns);
    free(f);
}

void ir_emit(IRFunction *f, IROpcode op, IRValue dst, IRValue src1, IRValue src2) {
    if (f->insn_count >= f->insn_capacity) {
        f->insn_capacity *= 2;
        f->insns = realloc(f->insns, f->insn_capacity * sizeof(IRInsn));
    }
    IRInsn *insn = &f->insns[f->insn_count++];
    insn->opcode = op;
    insn->dst = dst;
    insn->src1 = src1;
    insn->src2 = src2;
    insn->comment = NULL;
}

void ir_emit_comment(IRFunction *f, const char *comment) {
    if (f->insn_count >= f->insn_capacity) {
        f->insn_capacity *= 2;
        f->insns = realloc(f->insns, f->insn_capacity * sizeof(IRInsn));
    }
    IRInsn *insn = &f->insns[f->insn_count++];
    insn->opcode = IR_COMMENT;
    insn->dst = ir_val_int(0);
    insn->src1 = ir_val_int(0);
    insn->src2 = ir_val_int(0);
    insn->comment = str_dup(comment);
}

int ir_new_reg(IRFunction *f) {
    return f->reg_count++;
}

IRModule *ir_module_create(void) {
    IRModule *m = calloc(1, sizeof(IRModule));
    m->func_capacity = 16;
    m->functions = calloc(m->func_capacity, sizeof(IRFunction *));
    m->func_count = 0;
    return m;
}

void ir_module_free(IRModule *m) {
    if (!m) return;
    for (int i = 0; i < m->func_count; i++) {
        ir_func_free(m->functions[i]);
    }
    free(m->functions);
    free(m);
}

IRFunction *ir_add_function(IRModule *m, const char *name) {
    if (m->func_count >= m->func_capacity) {
        m->func_capacity *= 2;
        m->functions = realloc(m->functions, m->func_capacity * sizeof(IRFunction *));
    }
    IRFunction *f = ir_func_create(name);
    m->functions[m->func_count++] = f;
    return f;
}

static const char *ir_opcode_name(IROpcode op) {
    switch (op) {
        case IR_NOP: return "nop";
        case IR_MOV: return "mov";
        case IR_ADD: return "add";
        case IR_SUB: return "sub";
        case IR_MUL: return "mul";
        case IR_DIV: return "div";
        case IR_MOD: return "mod";
        case IR_NEG: return "neg";
        case IR_CMP: return "cmp";
        case IR_JMP: return "jmp";
        case IR_JE: return "je";
        case IR_JNE: return "jne";
        case IR_JL: return "jl";
        case IR_JLE: return "jle";
        case IR_JG: return "jg";
        case IR_JGE: return "jge";
        case IR_CALL: return "call";
        case IR_RET: return "ret";
        case IR_PUSH: return "push";
        case IR_POP: return "pop";
        case IR_LABEL: return "label";
        case IR_ALLOCA: return "alloca";
        case IR_LOAD: return "load";
        case IR_STORE: return "store";
        case IR_LEA: return "lea";
        case IR_SYSCALL: return "syscall";
        case IR_COMMENT: return "#";
        default: return "???";
    }
}

static void ir_print_val(IRValue v) {
    switch (v.kind) {
        case VAL_INT: printf("%lld", v.int_val); break;
        case VAL_STR: printf("\"%s\"", v.str_val); break;
        case VAL_REG: printf("r%d", v.reg_index); break;
        case VAL_LABEL: printf(".L%s", v.str_val); break;
    }
}

void ir_print(IRModule *m) {
    for (int i = 0; i < m->func_count; i++) {
        IRFunction *f = m->functions[i];
        printf("function %s:\n", f->name);
        for (int j = 0; j < f->insn_count; j++) {
            IRInsn *insn = &f->insns[j];
            if (insn->opcode == IR_COMMENT) {
                printf("    # %s\n", insn->comment);
                continue;
            }
            printf("    %s", ir_opcode_name(insn->opcode));
            if (insn->opcode == IR_LABEL) {
                printf(" ");
                ir_print_val(insn->dst);
                printf(":");
            } else if (insn->opcode == IR_RET) {
                if (insn->dst.kind != VAL_INT || insn->dst.int_val != 0) {
                    printf(" ");
                    ir_print_val(insn->dst);
                }
            } else {
                printf(" ");
                ir_print_val(insn->dst);
                printf(", ");
                ir_print_val(insn->src1);
                if (insn->src2.kind != VAL_INT || insn->src2.int_val != 0) {
                    printf(", ");
                    ir_print_val(insn->src2);
                }
            }
            printf("\n");
        }
    }
}
