#ifndef TP_AST_H
#define TP_AST_H

typedef enum TP_NodeType {
    TP_NODE_NUMBER,
    TP_NODE_VAR_X,

    TP_NODE_UNARY_NEG,

    TP_NODE_ADD,
    TP_NODE_SUB,
    TP_NODE_MUL,
    TP_NODE_DIV,
    TP_NODE_POW,

    TP_NODE_FUNC1,
    TP_NODE_FRAC
} TP_NodeType;

typedef enum TP_Func1 {
    TP_F_SIN,
    TP_F_COS,
    TP_F_TAN,
    TP_F_LOG,
    TP_F_EXP,
    TP_F_SQRT
} TP_Func1;

typedef struct TP_Node TP_Node;

struct TP_Node {
    TP_NodeType type;
    union {
        double number;

        struct { TP_Node *a; } unary;
        struct { TP_Node *a, *b; } bin;

        struct { TP_Func1 f; TP_Node *arg; } func1;
        struct { TP_Node *num; TP_Node *den; } frac;
    } as;
};

TP_Node *tp_node_number(double v);
TP_Node *tp_node_var_x(void);
TP_Node *tp_node_unary(TP_NodeType t, TP_Node *a);
TP_Node *tp_node_bin(TP_NodeType t, TP_Node *a, TP_Node *b);
TP_Node *tp_node_func1(TP_Func1 f, TP_Node *arg);
TP_Node *tp_node_frac(TP_Node *num, TP_Node *den);

void tp_ast_free(TP_Node *n);

double tp_eval(const TP_Node *n, double x);

#endif
