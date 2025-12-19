#include "tp_parser.h"
#include <string.h>

typedef enum {
    PREC_NONE = 0,
    PREC_ADD  = 10,
    PREC_MUL  = 20,
    PREC_POW  = 30,
    PREC_PREFIX = 40
} Prec;

static void set_err(TP_Parser *p, const char *msg) {
    if (!p->error) {
        p->error = msg;
        p->error_pos = p->lx.current.pos;
        p->error_col = p->lx.current.col;
    }
}

static TP_Token cur(TP_Parser *p) { return p->lx.current; }

static void next(TP_Parser *p) {
    tp_lex_next(&p->lx);
    if (p->lx.error) {
        if (!p->error) {
            p->error = p->lx.error;
            p->error_pos = p->lx.error_pos;
            p->error_col = p->lx.error_col;
        }
    }
}

static int tok_is(TP_Parser *p, TP_TokType t) { return cur(p).type == t; }

static int is_command(TP_Token t, const char *name) {
    size_t n = strlen(name);
    return t.type == TP_TOK_COMMAND && t.len == n && strncmp(t.lexeme, name, n) == 0;
}

static Prec infix_prec(TP_Parser *p) {
    TP_TokType t = cur(p).type;

    if (t == TP_TOK_PLUS || t == TP_TOK_MINUS) return PREC_ADD;
    if (t == TP_TOK_STAR || t == TP_TOK_SLASH) return PREC_MUL;
    if (t == TP_TOK_CARET) return PREC_POW;

    /* multiplicação implícita */
    if (tp_tok_is_primary_start(t)) return PREC_MUL;

    return PREC_NONE;
}

static TP_Node *parse_expr_prec(TP_Parser *p, Prec prec);

static int consume(TP_Parser *p, TP_TokType t, const char *msg) {
    if (tok_is(p, t)) { next(p); return 1; }
    set_err(p, msg);
    return 0;
}

static TP_Node *parse_primary(TP_Parser *p) {
    TP_Token t = cur(p);

    if (t.type == TP_TOK_NUMBER) {
        next(p);
        return tp_node_number(t.number);
    }

    if (t.type == TP_TOK_IDENT) {
        /* v1: x, pi, e */
        if (t.len == 1 && t.lexeme[0] == 'x') {
            next(p);
            return tp_node_var_x();
        }
        if (t.len == 2 && strncmp(t.lexeme, "pi", 2) == 0) {
            next(p);
            return tp_node_number(3.14159265358979323846);
        }
        if (t.len == 1 && t.lexeme[0] == 'e') {
            next(p);
            return tp_node_number(2.71828182845904523536);
        }

        set_err(p, "identificador desconhecido (v1 suporta: x, pi, e)");
        return NULL;
    }

    if (t.type == TP_TOK_LPAREN) {
        next(p);
        TP_Node *inside = parse_expr_prec(p, PREC_NONE);
        if (!inside) return NULL;
        if (!consume(p, TP_TOK_RPAREN, "faltou ')'")) { tp_ast_free(inside); return NULL; }
        return inside;
    }

    if (t.type == TP_TOK_LBRACE) {
        next(p);
        TP_Node *inside = parse_expr_prec(p, PREC_NONE);
        if (!inside) return NULL;
        if (!consume(p, TP_TOK_RBRACE, "faltou '}'")) { tp_ast_free(inside); return NULL; }
        return inside;
    }

    if (t.type == TP_TOK_COMMAND) {
        if (is_command(t, "frac")) {
            next(p);

            if (!consume(p, TP_TOK_LBRACE, "faltou '{' apos \\frac")) return NULL;
            TP_Node *num = parse_expr_prec(p, PREC_NONE);
            if (!num) return NULL;
            if (!consume(p, TP_TOK_RBRACE, "faltou '}' no numerador de \\frac")) {
                tp_ast_free(num); return NULL;
            }

            if (!consume(p, TP_TOK_LBRACE, "faltou '{' no denominador de \\frac")) {
                tp_ast_free(num); return NULL;
            }
            TP_Node *den = parse_expr_prec(p, PREC_NONE);
            if (!den) { tp_ast_free(num); return NULL; }
            if (!consume(p, TP_TOK_RBRACE, "faltou '}' no denominador de \\frac")) {
                tp_ast_free(num); tp_ast_free(den); return NULL;
            }

            return tp_node_frac(num, den);
        }

        TP_Func1 f;
        if      (is_command(t, "sin"))  f = TP_F_SIN;
        else if (is_command(t, "cos"))  f = TP_F_COS;
        else if (is_command(t, "tan"))  f = TP_F_TAN;
        else if (is_command(t, "log"))  f = TP_F_LOG;
        else if (is_command(t, "exp"))  f = TP_F_EXP;
        else if (is_command(t, "sqrt")) f = TP_F_SQRT;
        else {
            set_err(p, "comando \\... desconhecido (v1)");
            return NULL;
        }

        next(p);

        /* argumento pode ser (...) ou {...} ou um primary direto */
        TP_Node *arg = parse_primary(p);
        if (!arg) { set_err(p, "faltou argumento para funcao"); return NULL; }

        return tp_node_func1(f, arg);
    }

    set_err(p, "token inesperado no inicio de termo");
    return NULL;
}

static TP_Node *parse_prefix(TP_Parser *p) {
    if (tok_is(p, TP_TOK_MINUS)) {
        next(p);
        TP_Node *rhs = parse_prefix(p);
        if (!rhs) return NULL;
        return tp_node_unary(TP_NODE_UNARY_NEG, rhs);
    }
    return parse_primary(p);
}

static TP_Node *parse_expr_prec(TP_Parser *p, Prec prec) {
    TP_Node *left = parse_prefix(p);
    if (!left) return NULL;

    while (!p->error) {
        Prec pcur = infix_prec(p);
        if (pcur == PREC_NONE || pcur <= prec) break;

        TP_Token op = cur(p);

        if (op.type == TP_TOK_CARET) {
            next(p);
            TP_Node *rhs = parse_expr_prec(p, (Prec)(pcur - 1)); /* direita-assoc */
            if (!rhs) { tp_ast_free(left); return NULL; }
            left = tp_node_bin(TP_NODE_POW, left, rhs);
            continue;
        }

        if (tp_tok_is_primary_start(op.type)) {
            TP_Node *rhs = parse_expr_prec(p, PREC_MUL);
            if (!rhs) { tp_ast_free(left); return NULL; }
            left = tp_node_bin(TP_NODE_MUL, left, rhs);
            continue;
        }

        if (op.type == TP_TOK_PLUS || op.type == TP_TOK_MINUS ||
            op.type == TP_TOK_STAR || op.type == TP_TOK_SLASH) {

            next(p);
            TP_Node *rhs = parse_expr_prec(p, pcur);
            if (!rhs) { tp_ast_free(left); return NULL; }

            TP_NodeType nt = TP_NODE_ADD;
            if (op.type == TP_TOK_PLUS)  nt = TP_NODE_ADD;
            if (op.type == TP_TOK_MINUS) nt = TP_NODE_SUB;
            if (op.type == TP_TOK_STAR)  nt = TP_NODE_MUL;
            if (op.type == TP_TOK_SLASH) nt = TP_NODE_DIV;

            left = tp_node_bin(nt, left, rhs);
            continue;
        }

        break;
    }

    return left;
}

void tp_parse_init(TP_Parser *p, const char *src) {
    p->error = NULL;
    p->error_pos = 0;
    p->error_col = 1;

    tp_lex_init(&p->lx, src);
    if (p->lx.error) {
        p->error = p->lx.error;
        p->error_pos = p->lx.error_pos;
        p->error_col = p->lx.error_col;
    }
}

TP_Node *tp_parse_expr(TP_Parser *p) {
    TP_Node *root = parse_expr_prec(p, PREC_NONE);
    if (!root) return NULL;

    if (!p->error && cur(p).type != TP_TOK_EOF) {
        set_err(p, "sobrou texto apos o fim da expressao");
        tp_ast_free(root);
        return NULL;
    }
    if (p->error) {
        tp_ast_free(root);
        return NULL;
    }
    return root;
}
