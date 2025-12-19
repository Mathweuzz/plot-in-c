#ifndef TP_PARSER_H
#define TP_PARSER_H

#include <stddef.h>
#include "tp_token.h"
#include "tp_ast.h"

typedef struct TP_Parser {
    TP_Lexer lx;
    const char *error;
    size_t error_pos;
    size_t error_col;
} TP_Parser;

void tp_parse_init(TP_Parser *p, const char *src);
TP_Node *tp_parse_expr(TP_Parser *p);

#endif
