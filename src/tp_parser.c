#include "tp_parser.h"
#include <string.h>

/* Precedências:
   - prefixo (-)
   - potência (^) direita-associativa
   - multiplicação/divisão
   - soma/sub
   - multiplicação implícita tem mesma precedência de '*' (nesta v1) */

typedef enum {
    PREC_NONE = 0,
    PREC_ADD  = 10,
    PREC_MUL  = 20,
    PREC_POW  = 30,
    PREC_PREFIX = 40
} Prec;

static void set_err(TP_Parser *p, const char *msg) {
    if (!p->error) p->error = msg;
}

static TP_Token cur(TP_Parser *p) { return p->lx.current; }
static void next(TP_Parser *p) { tp_lex_next(&p->lx); if (p->lx.error) set_err(p, p->lx.error); }

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

    /* multiplicação implícita: se o token atual pode iniciar um "primary",
       e o lado esquerdo já existe, tratamos como '*' */
    if (tp_tok_is_primary_start(t)) return PREC_MUL;

    return PREC_NONE;
}

/* Forward */
static TP_Node *parse_expr_prec(TP_Parser *p, Prec prec);

static int consume(TP_Parser *p, TP_TokType t, const char *msg) {
    if (tok_is(p, t)) { next(p); return 1; }
    set_err(p, msg);
    return 0;
}

/* Parse de primary:
   - number
   - x
   - (expr)
   - {expr}
   - \sin(expr) ou \sin{expr} (aceita ambos)
   - \frac{a}{b}
*/
static TP_Node *parse_primary(TP_Parser *p) {
    TP_Token t = cur(p);

    if (t.type == TP_TOK_NUMBER) {
        next(p);
        return tp_node_number(t.number);
    }

    if (t.type == TP_TOK_IDENT) {
        /* v1: apenas 'x' */
        if (t.len == 1 && t.lexeme[0] == 'x') {
            next(p);
            return tp_node_var_x();
        }
        set_err(p, "identificador desconhecido (v1 suporta apenas 'x')");
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
        /* \frac{a}{b} */
        if (is_command(t, "frac")) {
            next(p);
            if (!consume(p, TP_TOK_LBRACE, "faltou '{' apos \\frac")) return NULL;
            TP_Node *num = parse_expr_prec(p, PREC_NONE);
            if (!num) return NULL;
            if (!consume(p, TP_TOK_RBRACE, "faltou '}' no numerador de \\frac")) { tp_ast_free(num); return NULL; }

            if (!consume(p, TP_TOK_LBRACE, "faltou '{' no denominador de \\frac")) { tp_ast_free(num); return NULL; }
            TP_Node *den = parse_expr_prec(p, PREC_NONE);
            if (!den) { tp_ast_free(num); return NULL; }
            if (!consume(p, TP_TOK_RBRACE, "faltou '}' no denominador de \\frac")) { tp_ast_free(num); tp_ast_free(den); return NULL; }

            return tp_node_frac(num, den);
        }

        /* Funções unárias */
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

        /* Aceita argumento em (...) ou {...}.
           Também aceita \sin x (um primary) como açúcar. */
        TP_Node *arg = NULL;
        if (tok_is(p, TP_TOK_LPAREN) || tok_is(p, TP_TOK_LBRACE)) {
            arg = parse_primary(p);
        } else {
            /* \sin x -> arg é primary */
            arg = parse_primary(p);
        }

        if (!arg) { set_err(p, "faltou argumento para funcao"); return NULL; }
        return tp_node_func1(f, arg);
    }

    set_err(p, "token inesperado no inicio de termo");
    return NULL;
}

/* Parse prefixo: -primary */
static TP_Node *parse_prefix(TP_Parser *p) {
    if (tok_is(p, TP_TOK_MINUS)) {
        next(p);
        TP_Node *rhs = parse_prefix(p); /* prefixo pode encadear: --x */
        if (!rhs) return NULL;
        return tp_node_unary(TP_NODE_UNARY_NEG, rhs);
    }
    return parse_primary(p);
}

/* Pratt: expr com precedência */
static TP_Node *parse_expr_prec(TP_Parser *p, Prec prec) {
    TP_Node *left = parse_prefix(p);
    if (!left) return NULL;

    while (!p->error) {
        Prec pcur = infix_prec(p);
        if (pcur == PREC_NONE || pcur <= prec) break;

        TP_Token op = cur(p);

        /* Potência é direita-associativa:
           a ^ b ^ c => a ^ (b ^ c)
           então o RHS usa (pcur - 1) */
        if (op.type == TP_TOK_CARET) {
            next(p);
            TP_Node *rhs = parse_expr_prec(p, (Prec)(pcur - 1));
            if (!rhs) { tp_ast_free(left); return NULL; }
            left = tp_node_bin(TP_NODE_POW, left, rhs);
            continue;
        }

        /* Multiplicação implícita: se estamos aqui e token atual é início de primary,
           tratamos como '*' sem consumir operador explícito. */
        if (tp_tok_is_primary_start(op.type)) {
            TP_Node *rhs = parse_expr_prec(p, (Prec)(PREC_MUL)); /* mesma prec do '*' */
            if (!rhs) { tp_ast_free(left); return NULL; }
            left = tp_node_bin(TP_NODE_MUL, left, rhs);
            continue;
        }

        /* Operadores normais */
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
    tp_lex_init(&p->lx, src);
    if (p->lx.error) set_err(p, p->lx.error);
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
