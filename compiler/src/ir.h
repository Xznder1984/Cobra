#ifndef COBRA_IR_H
#define COBRA_IR_H

#include "ast.h"

typedef enum {
    IR_NOP,
    IR_MOV,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_MOD,
    IR_NEG,
    IR_CMP,
    IR_JMP,
    IR_JE,
    IR_JNE,
    IR_JL,
    IR_JLE,
    IR_JG,
    IR_JGE,
    IR_CALL,
    IR_RET,
    IR_PUSH,
    IR_POP,
    IR_LABEL,
    IR_ALLOCA,
    IR_LOAD,
    IR_STORE,
    IR_LEA,
    IR_SYSCALL,
    IR_COMMENT,
} IROpcode;

typedef struct IRValue {
    enum { VAL_INT, VAL_STR, VAL_REG, VAL_LABEL } kind;
    long long int_val;
    char *str_val;
    int reg_index;
} IRValue;

typedef struct IRInsn {
    IROpcode opcode;
    IRValue dst;
    IRValue src1;
    IRValue src2;
    char *comment;
} IRInsn;

typedef struct IRFunction {
    char *name;
    IRInsn *insns;
    int insn_count;
    int insn_capacity;
    int reg_count;
} IRFunction;

typedef struct IRModule {
    IRFunction **functions;
    int func_count;
    int func_capacity;
} IRModule;

IRValue ir_val_int(long long v);
IRValue ir_val_str(const char *s);
IRValue ir_val_reg(int reg);

IRFunction *ir_func_create(const char *name);
void ir_func_free(IRFunction *f);
void ir_emit(IRFunction *f, IROpcode op, IRValue dst, IRValue src1, IRValue src2);
void ir_emit_comment(IRFunction *f, const char *comment);
int ir_new_reg(IRFunction *f);

IRModule *ir_module_create(void);
void ir_module_free(IRModule *m);
IRFunction *ir_add_function(IRModule *m, const char *name);

void ir_print(IRModule *m);

#endif
