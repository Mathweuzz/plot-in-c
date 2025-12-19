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

static int is_cmd_1char(TP_Token t, char c) {
    return t.type == TP_TOK_COMMAND && t.len == 1 && t.lexeme[0] == c;
}

/* comandos que ignoramos (layout TeX / wrappers) */
static int is_noop_command(TP_Token t) {
    if (t.type != TP_TOK_COMMAND) return 0;
    if (is_command(t, "left") || is_command(t, "right")) return 1;
    if (is_command(t, "quad") || is_command(t, "qquad")) return 1;
    if (is_cmd_1char(t, ';') || is_cmd_1char(t, ',') || is_cmd_1char(t, ':') || is_cmd_1char(t, '!')) return 1;
    return 0;
}

static void skip_noops(TP_Parser *p) {
    while (!p->error && cur(p).type == TP_TOK_COMMAND && is_noop_command(cur(p))) {
        next(p);
    }
}

static Prec infix_prec(TP_Parser *p) {
    TP_TokType t = cur(p).type;

    if (t == TP_TOK_PLUS || t == TP_TOK_MINUS) return PREC_ADD;
    if (t == TP_TOK_STAR || t == TP_TOK_SLASH) return PREC_MUL;
    if (t == TP_TOK_CARET) return PREC_POW;

    /* COMMA encerra expressão dentro de (a,b) */
    if (t == TP_TOK_COMMA) return PREC_NONE;

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

/* Forward */
static TP_Node *parse_primary(TP_Parser *p);

static TP_Node *parse_prefix(TP_Parser *p) {
    skip_noops(p);

    if (tok_is(p, TP_TOK_MINUS)) {
        next(p);
        TP_Node *rhs = parse_prefix(p);
        if (!rhs) return NULL;
        return tp_node_unary(TP_NODE_UNARY_NEG, rhs);
    }
    return parse_primary(p);
}

/* Parse primary:
   - number
   - x | pi | e
   - (expr) ou (a,b)
   - {expr} ou {a,b}
   - \frac{a}{b}
   - \sin^{k}(x) (TeX) -> (sin(x))^k
*/
static TP_Node *parse_group_paren(TP_Parser *p) {
    /* já consumiu '(' */
    skip_noops(p);
    TP_Node *first = parse_expr_prec(p, PREC_NONE);
    if (!first) return NULL;

    skip_noops(p);

    if (tok_is(p, TP_TOK_COMMA)) {
        /* tuple */
        next(p);
        skip_noops(p);
        TP_Node *second = parse_expr_prec(p, PREC_NONE);
        if (!second) { tp_ast_free(first); return NULL; }
        skip_noops(p);
        if (!consume(p, TP_TOK_RPAREN, "faltou ')' apos tupla (a,b)")) {
            tp_ast_free(first); tp_ast_free(second); return NULL;
        }
        return tp_node_tuple2(first, second);
    }

    if (!consume(p, TP_TOK_RPAREN, "faltou ')'")) { tp_ast_free(first); return NULL; }
    return first;
}

static TP_Node *parse_group_brace(TP_Parser *p) {
    /* já consumiu '{' */
    skip_noops(p);
    TP_Node *first = parse_expr_prec(p, PREC_NONE);
    if (!first) return NULL;

    skip_noops(p);

    if (tok_is(p, TP_TOK_COMMA)) {
        next(p);
        skip_noops(p);
        TP_Node *second = parse_expr_prec(p, PREC_NONE);
        if (!second) { tp_ast_free(first); return NULL; }
        skip_noops(p);
        if (!consume(p, TP_TOK_RBRACE, "faltou '}' apos tupla {a,b}")) {
            tp_ast_free(first); tp_ast_free(second); return NULL;
        }
        return tp_node_tuple2(first, second);
    }

    if (!consume(p, TP_TOK_RBRACE, "faltou '}'")) { tp_ast_free(first); return NULL; }
    return first;
}

static TP_Node *parse_primary(TP_Parser *p) {
    skip_noops(p);
    TP_Token t = cur(p);

    if (t.type == TP_TOK_NUMBER) {
        next(p);
        return tp_node_number(t.number);
    }

    if (t.type == TP_TOK_IDENT) {
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
        return parse_group_paren(p);
    }

    if (t.type == TP_TOK_LBRACE) {
        next(p);
        return parse_group_brace(p);
    }

    if (t.type == TP_TOK_COMMAND) {
        /* no-op commands já foram pulados, então aqui é comando real */
        if (is_command(t, "frac")) {
            next(p);

            skip_noops(p);
            if (!consume(p, TP_TOK_LBRACE, "faltou '{' apos \\frac")) return NULL;
            TP_Node *num = parse_expr_prec(p, PREC_NONE);
            if (!num) return NULL;
            skip_noops(p);
            if (!consume(p, TP_TOK_RBRACE, "faltou '}' no numerador de \\frac")) {
                tp_ast_free(num); return NULL;
            }

            skip_noops(p);
            if (!consume(p, TP_TOK_LBRACE, "faltou '{' no denominador de \\frac")) {
                tp_ast_free(num); return NULL;
            }
            TP_Node *den = parse_expr_prec(p, PREC_NONE);
            if (!den) { tp_ast_free(num); return NULL; }
            skip_noops(p);
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
        skip_noops(p);

        /* Suporte TeX: \sin^{3}(x) => (sin(x))^3 */
        TP_Node *exponent = NULL;
        if (tok_is(p, TP_TOK_CARET)) {
            next(p);
            skip_noops(p);
            /* expoente pode ser {expr} ou primary simples */
            exponent = parse_prefix(p);
            if (!exponent) { set_err(p, "faltou expoente apos '^'"); return NULL; }
            skip_noops(p);
        }

        /* argumento pode ser (...) ou {...} ou primary direto */
        TP_Node *arg = parse_primary(p);
        if (!arg) { set_err(p, "faltou argumento para funcao"); tp_ast_free(exponent); return NULL; }

        TP_Node *fn = tp_node_func1(f, arg);
        if (!fn) { tp_ast_free(arg); tp_ast_free(exponent); return NULL; }

        if (exponent) {
            TP_Node *pow_node = tp_node_bin(TP_NODE_POW, fn, exponent);
            if (!pow_node) { tp_ast_free(fn); tp_ast_free(exponent); return NULL; }
            return pow_node;
        }

        return fn;
    }

    set_err(p, "token inesperado no inicio de termo");
    return NULL;
}

static TP_Node *parse_expr_prec(TP_Parser *p, Prec prec) {
    TP_Node *left = parse_prefix(p);
    if (!left) return NULL;

    while (!p->error) {
        skip_noops(p);

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

    skip_noops(p);

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
