#ifndef TP_TOKEN_H
#define TP_TOKEN_H

#include <stddef.h>

typedef enum TP_TokType {
    TP_TOK_EOF = 0,
    TP_TOK_NUMBER,
    TP_TOK_IDENT,     /* para "x" (e futuro: constantes) */
    TP_TOK_COMMAND,   /* \sin, \sqrt, \frac ... (sem a barra) */

    TP_TOK_PLUS,      /* + */
    TP_TOK_MINUS,     /* - */
    TP_TOK_STAR,      /* * */
    TP_TOK_SLASH,     /* / */
    TP_TOK_CARET,     /* ^ */

    TP_TOK_LPAREN,    /* ( */
    TP_TOK_RPAREN,    /* ) */
    TP_TOK_LBRACE,    /* { */
    TP_TOK_RBRACE,    /* } */

    TP_TOK_INVALID
} TP_TokType;

typedef struct TP_Token {
    TP_TokType type;
    const char *lexeme; /* aponta para dentro da string original */
    size_t len;

    double number;      /* válido quando type==NUMBER */
} TP_Token;

typedef struct TP_Lexer {
    const char *src;
    size_t pos;
    TP_Token current;

    const char *error;
} TP_Lexer;

void tp_lex_init(TP_Lexer *lx, const char *src);
void tp_lex_next(TP_Lexer *lx);

int tp_tok_is_primary_start(TP_TokType t); /* útil p/ multiplicação implícita */

#endif
