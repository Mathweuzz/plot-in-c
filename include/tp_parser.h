#ifndef TP_PARSER_H
#define TP_PARSER_H

#include "tp_token.h"
#include "tp_ast.h"

typedef struct TP_Parser {
    TP_Lexer lx;
    const char *error;
} TP_Parser;

void tp_parse_init(TP_Parser *p, const char *src);

/* Parse da expressão inteira.
   Retorna AST ou NULL (p->error setado). */
TP_Node *tp_parse_expr(TP_Parser *p);

#endif
