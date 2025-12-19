#include "tp_ast.h"
#include <math.h>
#include <stdlib.h>

static TP_Node *tp_new_node(TP_NodeType t) {
    TP_Node *n = (TP_Node*)calloc(1, sizeof(TP_Node));
    if (!n) return NULL;
    n->type = t;
    return n;
}

TP_Node *tp_node_number(double v) {
    TP_Node *n = tp_new_node(TP_NODE_NUMBER);
    if (!n) return NULL;
    n->as.number = v;
    return n;
}

TP_Node *tp_node_var_x(void) {
    return tp_new_node(TP_NODE_VAR_X);
}

TP_Node *tp_node_unary(TP_NodeType t, TP_Node *a) {
    TP_Node *n = tp_new_node(t);
    if (!n) return NULL;
    n->as.unary.a = a;
    return n;
}

TP_Node *tp_node_bin(TP_NodeType t, TP_Node *a, TP_Node *b) {
    TP_Node *n = tp_new_node(t);
    if (!n) return NULL;
    n->as.bin.a = a;
    n->as.bin.b = b;
    return n;
}

TP_Node *tp_node_func1(TP_Func1 f, TP_Node *arg) {
    TP_Node *n = tp_new_node(TP_NODE_FUNC1);
    if (!n) return NULL;
    n->as.func1.f = f;
    n->as.func1.arg = arg;
    return n;
}

TP_Node *tp_node_frac(TP_Node *num, TP_Node *den) {
    TP_Node *n = tp_new_node(TP_NODE_FRAC);
    if (!n) return NULL;
    n->as.frac.num = num;
    n->as.frac.den = den;
    return n;
}

TP_Node *tp_node_tuple2(TP_Node *a, TP_Node *b) {
    TP_Node *n = tp_new_node(TP_NODE_TUPLE2);
    if (!n) return NULL;
    n->as.tuple2.a = a;
    n->as.tuple2.b = b;
    return n;
}

void tp_ast_free(TP_Node *n) {
    if (!n) return;

    switch (n->type) {
        case TP_NODE_UNARY_NEG:
            tp_ast_free(n->as.unary.a);
            break;

        case TP_NODE_ADD:
        case TP_NODE_SUB:
        case TP_NODE_MUL:
        case TP_NODE_DIV:
        case TP_NODE_POW:
            tp_ast_free(n->as.bin.a);
            tp_ast_free(n->as.bin.b);
            break;

        case TP_NODE_FUNC1:
            tp_ast_free(n->as.func1.arg);
            break;

        case TP_NODE_FRAC:
            tp_ast_free(n->as.frac.num);
            tp_ast_free(n->as.frac.den);
            break;

        case TP_NODE_TUPLE2:
            tp_ast_free(n->as.tuple2.a);
            tp_ast_free(n->as.tuple2.b);
            break;

        default:
            break;
    }

    free(n);
}

double tp_eval(const TP_Node *n, double x) {
    if (!n) return NAN;

    switch (n->type) {
        case TP_NODE_NUMBER: return n->as.number;
        case TP_NODE_VAR_X:  return x;

        case TP_NODE_UNARY_NEG:
            return -tp_eval(n->as.unary.a, x);

        case TP_NODE_ADD:
            return tp_eval(n->as.bin.a, x) + tp_eval(n->as.bin.b, x);
        case TP_NODE_SUB:
            return tp_eval(n->as.bin.a, x) - tp_eval(n->as.bin.b, x);
        case TP_NODE_MUL:
            return tp_eval(n->as.bin.a, x) * tp_eval(n->as.bin.b, x);
        case TP_NODE_DIV:
            return tp_eval(n->as.bin.a, x) / tp_eval(n->as.bin.b, x);
        case TP_NODE_POW:
            return pow(tp_eval(n->as.bin.a, x), tp_eval(n->as.bin.b, x));

        case TP_NODE_FUNC1: {
            double a = tp_eval(n->as.func1.arg, x);
            switch (n->as.func1.f) {
                case TP_F_SIN:  return sin(a);
                case TP_F_COS:  return cos(a);
                case TP_F_TAN:  return tan(a);
                case TP_F_LOG:  return log(a);
                case TP_F_EXP:  return exp(a);
                case TP_F_SQRT: return sqrt(a);
                default: return NAN;
            }
        }

        case TP_NODE_FRAC:
            return tp_eval(n->as.frac.num, x) / tp_eval(n->as.frac.den, x);

        /* Tupla não é "avaliável" como escalar */
        case TP_NODE_TUPLE2:
            return NAN;

        default:
            return NAN;
    }
}
