#ifndef TP_CLI_H
#define TP_CLI_H

#include "tp_view.h"

typedef struct TP_Args {
    const char *expr;

    int width;
    int height;

    unsigned char bg_r, bg_g, bg_b;
    unsigned char fg_r, fg_g, fg_b;

    TP_View view;
} TP_Args;

int tp_args_parse(int argc, char **argv, TP_Args *out,
                  char *errbuf, int errbuf_sz);

/* usado por main e também quando user pede --help */
void tp_args_print_help(const char *prog);

#endif
