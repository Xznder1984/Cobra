#ifndef COBRA_AST_H
#define COBRA_AST_H

#include "utils.h"
#include "lexer.h"

typedef enum {
    NODE_MODULE,
    NODE_FN_DEF,
    NODE_PARAM,
    NODE_LET_DECL,
    NODE_CONST_DECL,
    NODE_VAR_DECL,
    NODE_STRUCT_DEF,
    NODE_CLASS_DEF,
    NODE_ENUM_DEF,
    NODE_TRAIT_DEF,
    NODE_IMPL_BLOCK,
    NODE_TYPE_ALIAS,
    NODE_IMPORT,
    NODE_USE_DECL,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_MATCH,
    NODE_MATCH_ARM,
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_DEFER,
    NODE_TRY,
    NODE_THROW,
    NODE_BLOCK,
    NODE_EXPR_STMT,

    NODE_BINARY,
    NODE_UNARY,
    NODE_CALL,
    NODE_INDEX,
    NODE_MEMBER,
    NODE_IDENTIFIER,
    NODE_INT_LITERAL,
    NODE_FLOAT_LITERAL,
    NODE_STR_LITERAL,
    NODE_CHAR_LITERAL,
    NODE_BOOL_LITERAL,
    NODE_NIL_LITERAL,
    NODE_LIST_LITERAL,
    NODE_DICT_LITERAL,
    NODE_TUPLE_LITERAL,
    NODE_LAMBDA,
    NODE_IF_EXPR,
    NODE_CAST,
    NODE_AS_EXPR,
    NODE_IS_EXPR,
    NODE_STRUCT_INIT,
    NODE_FIELD_ACCESS,
    NODE_DOT_DOT,
    NODE_RANGE,
    NODE_ASSIGN,
    NODE_ASSIGN_OP,
    NODE_ANNOTATION,
    NODE_TUPLE,
    NODE_SLICE,
    NODE_AWAIT,
} NodeType;

typedef enum {
    TYPE_INFER,
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_INT,
    TYPE_I8, TYPE_I16, TYPE_I32, TYPE_I64,
    TYPE_U8, TYPE_U16, TYPE_U32, TYPE_U64,
    TYPE_F32, TYPE_F64,
    TYPE_STR,
    TYPE_CHAR,
    TYPE_NAMED,
    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_SLICE,
    TYPE_TUPLE_TYPE,
    TYPE_FN_TYPE,
    TYPE_OPTIONAL,
    TYPE_RESULT,
} TypeKind;

typedef struct Type {
    TypeKind kind;
    struct Type *subtype;
    struct Type *subtype2;
    struct Type **params;
    int param_count;
    char *name;
    int is_mut;
    size_t size;
} Type;

typedef struct Node Node;
typedef struct NodeList {
    Node **items;
    int count;
    int capacity;
} NodeList;

struct Node {
    NodeType type;
    int line;
    int column;
    Type *expr_type;
    int is_constant;

    union {
        struct {
            char *name;
            NodeList imports;
            NodeList statements;
            Node *body;
        } module;

        struct {
            char *name;
            NodeList params;
            Type *return_type;
            NodeList body;
            NodeList type_params;
            int is_public;
            int is_async;
            int is_extern;
            char *link_name;
        } fn_def;

        struct {
            char *name;
            Type *type;
            Node *default_value;
        } param;

        struct {
            char *name;
            Type *type;
            Node *value;
            int is_mut;
        } let_decl;

        struct {
            char *name;
            Type *type;
            Node *value;
        } const_decl;

        struct {
            char *name;
            NodeList fields;
            NodeList type_params;
            int is_public;
        } struct_def;

        struct {
            char *name;
            NodeList fields;
            NodeList methods;
            NodeList type_params;
            int is_public;
        } class_def;

        struct {
            char *name;
            NodeList variants;
            NodeList type_params;
            int is_public;
        } enum_def;

        struct {
            char *name;
            NodeList methods;
            NodeList type_params;
            int is_public;
        } trait_def;

        struct {
            char *type_name;
            char *trait_name;
            NodeList body;
        } impl_block;

        struct {
            char *from_module;
            char *name;
            char *alias;
        } import;

        struct {
            char *name;
            char *package;
        } use_decl;

        struct {
            Node *condition;
            Node *then_block;
            Node *elif_chain;
            Node *else_block;
        } if_stmt;

        struct {
            Node *condition;
            Node *body;
        } while_loop;

        struct {
            char *var_name;
            Node *iterable;
            Node *body;
        } for_loop;

        struct {
            Node *expr;
            NodeList arms;
        } match_stmt;

        struct {
            NodeList patterns;
            Node *guard;
            Node *value;
        } match_arm;

        struct {
            Node *value;
        } return_stmt;

        struct {
            Node *value;
        } throw_stmt;

        struct {
            Node *value;
        } try_expr;

        struct {
            Node *body;
        } defer_stmt;

        struct {
            NodeList statements;
        } block;

        struct {
            Node *expr;
        } expr_stmt;

        struct {
            TokenType op;
            Node *left;
            Node *right;
        } binary;

        struct {
            TokenType op;
            Node *operand;
            int is_prefix;
        } unary;

        struct {
            Node *callee;
            NodeList args;
        } call;

        struct {
            Node *target;
            Node *index;
        } index_expr;

        struct {
            Node *object;
            char *member;
        } member_expr;

        struct {
            char *name;
        } identifier;

        long long int_value;
        double float_value;
        char *string_value;
        char char_value;
        int bool_value;

        struct {
            NodeList elements;
        } list_literal;

        struct {
            NodeList keys;
            NodeList values;
        } dict_literal;

        struct {
            NodeList elements;
        } tuple_literal;

        struct {
            NodeList params;
            Type *return_type;
            Node *body;
        } lambda;

        struct {
            Node *condition;
            Node *then_expr;
            Node *else_expr;
        } if_expr;

        struct {
            Type *target_type;
            Node *expr;
        } cast;

        struct {
            Node *expr;
            Type *target_type;
        } as_expr;

        struct {
            Node *expr;
            Type *target_type;
        } is_expr;

        struct {
            char *type_name;
            NodeList fields;
        } struct_init;

        struct {
            char *name;
            Node *value;
        } struct_field;

        struct {
            Node *start;
            Node *end;
            int inclusive;
        } range;

        struct {
            TokenType op;
            Node *target;
            Node *value;
        } assign;

        struct {
            char *name;
            Node *value;
        } annotation;

        struct {
            Node *expr;
        } await_expr;
    } data;
};

Node *node_create(NodeType type, int line, int column);
void node_free(Node *node);
void node_list_init(NodeList *list);
void node_list_add(NodeList *list, Node *node);
void node_list_free(NodeList *list);

Type *type_create(TypeKind kind);
Type *type_create_named(const char *name);
Type *type_create_pointer(Type *subtype);
Type *type_create_array(Type *subtype, size_t size);
Type *type_create_fn(Type **params, int count, Type *ret);
void type_free(Type *type);
Type *type_clone(Type *type);
int type_equal(Type *a, Type *b);
size_t type_size(Type *type);

Node *node_make_int(int line, int col, long long val);
Node *node_make_float(int line, int col, double val);
Node *node_make_str(int line, int col, const char *val);
Node *node_make_bool(int line, int col, int val);
Node *node_make_nil(int line, int col);
Node *node_make_ident(int line, int col, const char *name);

#endif
