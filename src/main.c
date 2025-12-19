#include <stdio.h>
#include <math.h>

#include "tp_parser.h"
#include "tp_ast.h"

static void sample(const char *expr) {
    TP_Parser p;
    tp_parse_init(&p, expr);

    TP_Node *root = tp_parse_expr(&p);
    if (!root) {
        fprintf(stderr, "ERRO parse: %s\n", p.error ? p.error : "desconhecido");
        fprintf(stderr, "Expr: %s\n", expr);
        return;
    }

    printf("Expr OK: %s\n", expr);
    for (int i = -5; i <= 5; i++) {
        double x = (double)i;
        double y = tp_eval(root, x);
        printf("  x=% .2f -> y=% .6f\n", x, y);
    }

    tp_ast_free(root);
}

int main(void) {
    /* Testes que cobrem boa parte do v1 */
    sample("\\sin(x)");
    sample("x^2 + 3x + 2");
    sample("\\frac{\\sin(x)}{x}");
    sample("\\sqrt{x^2 + 1}");
    sample("\\exp(-\\frac{x^2}{2})");

    return 0;
}