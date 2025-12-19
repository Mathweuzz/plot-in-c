#include "tp_cli.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static int parse_int(const char *s, int *out) {
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') return 0;
    if (v < 100 || v > 10000) return 0;
    *out = (int)v;
    return 1;
}

static int parse_double(const char *s, double *out) {
    errno = 0;
    char *end = NULL;
    double v = strtod(s, &end);
    if (errno != 0 || end == s || *end != '\0') return 0;
    *out = v;
    return 1;
}

static int parse_rgb(const char *s, unsigned char *r, unsigned char *g, unsigned char *b) {
    int ri, gi, bi;
    if (sscanf(s, "%d,%d,%d", &ri, &gi, &bi) != 3) return 0;
    if (ri < 0 || ri > 255 || gi < 0 || gi > 255 || bi < 0 || bi > 255) return 0;
    *r = (unsigned char)ri;
    *g = (unsigned char)gi;
    *b = (unsigned char)bi;
    return 1;
}

static int streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

void tp_args_print_help(const char *prog) {
    printf("Uso:\n");
    printf("  %s --expr \"<expressao>\" [opcoes]\n\n", prog);
    printf("Opcoes:\n");
    printf("  --expr   \"...\"        (obrigatorio) expressao estilo TeX (subset v1)\n");
    printf("  --xmin A  --xmax B     range do eixo X (default -10..10)\n");
    printf("  --ymin C  --ymax D     range do eixo Y (default -10..10)\n");
    printf("  --width W --height H   tamanho da janela (default 900x600)\n");
    printf("  --bg R,G,B             cor do fundo (default 0,0,0)\n");
    printf("  --fg R,G,B             cor do grafico (default 0,220,0)\n");
    printf("  -h, --help             mostra ajuda\n\n");
    printf("Exemplos (lembre de escapar barra no shell):\n");
    printf("  %s --expr \"\\\\sin(x)\"\n", prog);
    printf("  %s --expr \"\\\\frac{\\\\sin(x)}{x}\" --xmin -20 --xmax 20 --ymin -2 --ymax 2\n", prog);
    printf("  %s --expr \"2pi + e\" --ymin -1 --ymax 10\n", prog);
}

int tp_args_parse(int argc, char **argv, TP_Args *out,
                  char *errbuf, int errbuf_sz)
{
    if (!out) return 1;
    if (errbuf && errbuf_sz > 0) errbuf[0] = '\0';

    out->expr = NULL;
    out->width = 900;
    out->height = 600;

    out->bg_r = 0; out->bg_g = 0; out->bg_b = 0;
    out->fg_r = 0; out->fg_g = 220; out->fg_b = 0;

    out->view.xmin = -10.0; out->view.xmax = 10.0;
    out->view.ymin = -10.0; out->view.ymax = 10.0;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (streq(a, "-h") || streq(a, "--help")) {
            tp_args_print_help(argv[0]);
            return 2; /* help */
        }

        if (streq(a, "--expr")) {
            if (i + 1 >= argc) {
                snprintf(errbuf, errbuf_sz, "faltou valor para --expr");
                return 1;
            }
            out->expr = argv[++i];
            continue;
        }

        if (streq(a, "--width")) {
            if (i + 1 >= argc || !parse_int(argv[i+1], &out->width)) {
                snprintf(errbuf, errbuf_sz, "valor invalido para --width");
                return 1;
            }
            i++;
            continue;
        }

        if (streq(a, "--height")) {
            if (i + 1 >= argc || !parse_int(argv[i+1], &out->height)) {
                snprintf(errbuf, errbuf_sz, "valor invalido para --height");
                return 1;
            }
            i++;
            continue;
        }

        if (streq(a, "--xmin")) {
            if (i + 1 >= argc || !parse_double(argv[i+1], &out->view.xmin)) {
                snprintf(errbuf, errbuf_sz, "valor invalido para --xmin");
                return 1;
            }
            i++;
            continue;
        }
        if (streq(a, "--xmax")) {
            if (i + 1 >= argc || !parse_double(argv[i+1], &out->view.xmax)) {
                snprintf(errbuf, errbuf_sz, "valor invalido para --xmax");
                return 1;
            }
            i++;
            continue;
        }
        if (streq(a, "--ymin")) {
            if (i + 1 >= argc || !parse_double(argv[i+1], &out->view.ymin)) {
                snprintf(errbuf, errbuf_sz, "valor invalido para --ymin");
                return 1;
            }
            i++;
            continue;
        }
        if (streq(a, "--ymax")) {
            if (i + 1 >= argc || !parse_double(argv[i+1], &out->view.ymax)) {
                snprintf(errbuf, errbuf_sz, "valor invalido para --ymax");
                return 1;
            }
            i++;
            continue;
        }

        if (streq(a, "--bg")) {
            if (i + 1 >= argc || !parse_rgb(argv[i+1], &out->bg_r, &out->bg_g, &out->bg_b)) {
                snprintf(errbuf, errbuf_sz, "valor invalido para --bg (use R,G,B)");
                return 1;
            }
            i++;
            continue;
        }

        if (streq(a, "--fg")) {
            if (i + 1 >= argc || !parse_rgb(argv[i+1], &out->fg_r, &out->fg_g, &out->fg_b)) {
                snprintf(errbuf, errbuf_sz, "valor invalido para --fg (use R,G,B)");
                return 1;
            }
            i++;
            continue;
        }

        snprintf(errbuf, errbuf_sz, "argumento desconhecido: %s", a);
        return 1;
    }

    if (!out->expr) {
        snprintf(errbuf, errbuf_sz, "faltou --expr (obrigatorio)");
        return 1;
    }

    if (!(out->view.xmin < out->view.xmax)) {
        snprintf(errbuf, errbuf_sz, "range X invalido: xmin precisa ser < xmax");
        return 1;
    }
    if (!(out->view.ymin < out->view.ymax)) {
        snprintf(errbuf, errbuf_sz, "range Y invalido: ymin precisa ser < ymax");
        return 1;
    }

    return 0;
}
