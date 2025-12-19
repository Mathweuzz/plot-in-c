#ifndef TP_TOKEN_H
#define TP_TOKEN_H

#include <stddef.h>

typedef enum TP_TokType {
    TP_TOK_EOF = 0,
    TP_TOK_NUMBER,
    TP_TOK_IDENT,
    TP_TOK_COMMAND,

    TP_TOK_PLUS,
    TP_TOK_MINUS,
    TP_TOK_STAR,
    TP_TOK_SLASH,
    TP_TOK_CARET,

    TP_TOK_COMMA,   /* , */

    TP_TOK_LPAREN,
    TP_TOK_RPAREN,
    TP_TOK_LBRACE,
    TP_TOK_RBRACE,

    TP_TOK_INVALID
} TP_TokType;

typedef struct TP_Token {
    TP_TokType type;
    const char *lexeme;
    size_t len;

    size_t pos;   /* 0-based */
    size_t col;   /* 1-based */

    double number;
} TP_Token;

typedef struct TP_Lexer {
    const char *src;
    size_t pos;
    size_t col;

    TP_Token current;

    const char *error;
    size_t error_pos;
    size_t error_col;
} TP_Lexer;

void tp_lex_init(TP_Lexer *lx, const char *src);
void tp_lex_next(TP_Lexer *lx);

int tp_tok_is_primary_start(TP_TokType t);

#endif
