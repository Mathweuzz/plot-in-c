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

    /* flags */
    int has_xrange;
    int has_yrange;
    int has_t;

    /* parametric range */
    double tmin, tmax;
} TP_Args;

/* retorna:
   0 OK
   1 erro (errbuf)
   2 help impresso
*/
int tp_args_parse(int argc, char **argv, TP_Args *out,
                  char *errbuf, int errbuf_sz);

void tp_args_print_help(const char *prog);

#endif
