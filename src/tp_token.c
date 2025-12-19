#include "tp_token.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static char peek(const TP_Lexer *lx) { return lx->src[lx->pos]; }
static char peek_next(const TP_Lexer *lx) { return lx->src[lx->pos + 1]; }

static char advance(TP_Lexer *lx) {
    char c = lx->src[lx->pos++];
    if (c == '\n') lx->col = 1;
    else lx->col++;
    return c;
}

static void lex_error(TP_Lexer *lx, const char *msg) {
    lx->error = msg;
    lx->error_pos = lx->pos;
    lx->error_col = lx->col;
}

static void skip_ws(TP_Lexer *lx) {
    while (isspace((unsigned char)peek(lx))) advance(lx);
}

static TP_Token make_tok(TP_TokType t, const char *lex, size_t len, size_t pos, size_t col) {
    TP_Token tok;
    tok.type = t;
    tok.lexeme = lex;
    tok.len = len;
    tok.pos = pos;
    tok.col = col;
    tok.number = 0.0;
    return tok;
}

static int is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}
static int is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

int tp_tok_is_primary_start(TP_TokType t) {
    return (t == TP_TOK_NUMBER ||
            t == TP_TOK_IDENT ||
            t == TP_TOK_COMMAND ||
            t == TP_TOK_LPAREN ||
            t == TP_TOK_LBRACE);
}

void tp_lex_init(TP_Lexer *lx, const char *src) {
    lx->src = src ? src : "";
    lx->pos = 0;
    lx->col = 1;

    lx->error = NULL;
    lx->error_pos = 0;
    lx->error_col = 1;

    lx->current = make_tok(TP_TOK_EOF, lx->src, 0, 0, 1);
    tp_lex_next(lx);
}

void tp_lex_next(TP_Lexer *lx) {
    skip_ws(lx);

    const size_t tok_pos = lx->pos;
    const size_t tok_col = lx->col;
    const char *start = lx->src + lx->pos;
    char c = peek(lx);

    if (c == '\0') {
        lx->current = make_tok(TP_TOK_EOF, start, 0, tok_pos, tok_col);
        return;
    }

    switch (c) {
        case '+': advance(lx); lx->current = make_tok(TP_TOK_PLUS,  start, 1, tok_pos, tok_col); return;
        case '-': advance(lx); lx->current = make_tok(TP_TOK_MINUS, start, 1, tok_pos, tok_col); return;
        case '*': advance(lx); lx->current = make_tok(TP_TOK_STAR,  start, 1, tok_pos, tok_col); return;
        case '/': advance(lx); lx->current = make_tok(TP_TOK_SLASH, start, 1, tok_pos, tok_col); return;
        case '^': advance(lx); lx->current = make_tok(TP_TOK_CARET, start, 1, tok_pos, tok_col); return;
        case '(': advance(lx); lx->current = make_tok(TP_TOK_LPAREN,start, 1, tok_pos, tok_col); return;
        case ')': advance(lx); lx->current = make_tok(TP_TOK_RPAREN,start, 1, tok_pos, tok_col); return;
        case '{': advance(lx); lx->current = make_tok(TP_TOK_LBRACE,start, 1, tok_pos, tok_col); return;
        case '}': advance(lx); lx->current = make_tok(TP_TOK_RBRACE,start, 1, tok_pos, tok_col); return;
        default: break;
    }

    if (c == '\\') {
        advance(lx);
        const char *cmd_start = lx->src + lx->pos;
        const size_t cmd_pos = lx->pos;
        const size_t cmd_col = lx->col;

        if (!is_ident_start(peek(lx))) {
            lex_error(lx, "comando '\\' deve ser seguido de letras (ex: \\sin)");
            lx->current = make_tok(TP_TOK_INVALID, start, 1, tok_pos, tok_col);
            return;
        }
        while (is_ident_char(peek(lx))) advance(lx);
        size_t len = (size_t)((lx->src + lx->pos) - cmd_start);
        lx->current = make_tok(TP_TOK_COMMAND, cmd_start, len, cmd_pos, cmd_col);
        return;
    }

    if (isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)peek_next(lx)))) {
        while (isdigit((unsigned char)peek(lx))) advance(lx);
        if (peek(lx) == '.') {
            advance(lx);
            while (isdigit((unsigned char)peek(lx))) advance(lx);
        }
        size_t len = (size_t)((lx->src + lx->pos) - start);

        char buf[128];
        if (len >= sizeof(buf)) {
            lex_error(lx, "numero muito grande");
            lx->current = make_tok(TP_TOK_INVALID, start, len, tok_pos, tok_col);
            return;
        }
        memcpy(buf, start, len);
        buf[len] = '\0';

        TP_Token tok = make_tok(TP_TOK_NUMBER, start, len, tok_pos, tok_col);
        tok.number = strtod(buf, NULL);
        lx->current = tok;
        return;
    }

    if (is_ident_start(c)) {
        while (is_ident_char(peek(lx))) advance(lx);
        size_t len = (size_t)((lx->src + lx->pos) - start);
        lx->current = make_tok(TP_TOK_IDENT, start, len, tok_pos, tok_col);
        return;
    }

    lex_error(lx, "caractere invalido na expressao");
    lx->current = make_tok(TP_TOK_INVALID, start, 1, tok_pos, tok_col);
}
