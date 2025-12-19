#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "tp_cli.h"
#include "tp_view.h"
#include "tp_render.h"
#include "tp_plot.h"
#include "tp_parser.h"
#include "tp_ast.h"

static void update_title(SDL_Window *w, const TP_View *v, const char *expr) {
    char buf[300];

    char expr_short[80];
    if (expr) {
        size_t n = strlen(expr);
        if (n < sizeof(expr_short)) {
            strcpy(expr_short, expr);
        } else {
            memcpy(expr_short, expr, sizeof(expr_short) - 4);
            expr_short[sizeof(expr_short) - 4] = '.';
            expr_short[sizeof(expr_short) - 3] = '.';
            expr_short[sizeof(expr_short) - 2] = '.';
            expr_short[sizeof(expr_short) - 1] = '\0';
        }
    } else {
        strcpy(expr_short, "<null>");
    }

    snprintf(buf, sizeof(buf),
             "TatuPlot | %s | x:[%.3g,%.3g] y:[%.3g,%.3g] | drag: pan | wheel: zoom | R reset | ESC sair",
             expr_short, v->xmin, v->xmax, v->ymin, v->ymax);

    SDL_SetWindowTitle(w, buf);
}

static int isfinite_d(double x) { return isfinite(x); }

static void autofit_param_view(TP_View *view,
                               const TP_Node *xexpr, const TP_Node *yexpr,
                               double tmin, double tmax,
                               int fit_x, int fit_y)
{
    double minx = 0, maxx = 0, miny = 0, maxy = 0;
    int have = 0;

    const int N = 2500;
    for (int i = 0; i < N; i++) {
        double t = tmin + (tmax - tmin) * ((double)i / (double)(N - 1));
        double xw = tp_eval(xexpr, t);
        double yw = tp_eval(yexpr, t);

        if (!isfinite_d(xw) || !isfinite_d(yw)) continue;

        if (!have) {
            minx = maxx = xw;
            miny = maxy = yw;
            have = 1;
        } else {
            if (xw < minx) minx = xw;
            if (xw > maxx) maxx = xw;
            if (yw < miny) miny = yw;
            if (yw > maxy) maxy = yw;
        }
    }

    if (!have) return;

    double padx = (maxx - minx) * 0.05; if (padx <= 0) padx = 1.0;
    double pady = (maxy - miny) * 0.05; if (pady <= 0) pady = 1.0;

    if (fit_x) { view->xmin = minx - padx; view->xmax = maxx + padx; }
    if (fit_y) { view->ymin = miny - pady; view->ymax = maxy + pady; }
}

int main(int argc, char **argv) {
    TP_Args args;
    char err[256];

    int rc = tp_args_parse(argc, argv, &args, err, (int)sizeof(err));
    if (rc == 2) return 0;
    if (rc != 0) {
        fprintf(stderr, "ERRO: %s\n\n", err[0] ? err : "argumentos invalidos");
        tp_args_print_help(argv[0]);
        return 1;
    }

    TP_Parser p;
    tp_parse_init(&p, args.expr);
    TP_Node *expr_ast = tp_parse_expr(&p);
    if (!expr_ast) {
        fprintf(stderr, "ERRO parse (col %zu): %s\n", p.error_col, p.error ? p.error : "desconhecido");
        fprintf(stderr, "Expr: %s\n", args.expr);
        return 1;
    }

    const int is_tuple = (expr_ast->type == TP_NODE_TUPLE2);

    TP_View view = args.view;
    TP_View view0 = args.view;

    double tmin = 0.0, tmax = 1.0;
    int fit_x = 0;
    int fit_y = 0;

    if (is_tuple) {
        if (args.has_t) {
            tmin = args.tmin;
            tmax = args.tmax;
        } else if (args.has_xrange) {
            /* compat com teu comando: xmin/xmax viram t-range */
            tmin = args.view.xmin;
            tmax = args.view.xmax;
            fit_x = 1;
            fit_y = args.has_yrange ? 0 : 1;
            view.xmin = -10.0; view.xmax = 10.0;
        } else {
            tmin = 0.0;
            tmax = 6.283185307179586;
            fit_x = 1;
            fit_y = args.has_yrange ? 0 : 1;
        }

        autofit_param_view(&view,
                           expr_ast->as.tuple2.a, expr_ast->as.tuple2.b,
                           tmin, tmax,
                           fit_x, fit_y);

        view0 = view;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init falhou: %s\n", SDL_GetError());
        tp_ast_free(expr_ast);
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "TatuPlot",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        args.width, args.height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow falhou: %s\n", SDL_GetError());
        SDL_Quit();
        tp_ast_free(expr_ast);
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer falhou: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        tp_ast_free(expr_ast);
        return 1;
    }

    update_title(window, &view, args.expr);

    /* Drag state */
    int dragging = 0;
    int last_mx = 0, last_my = 0;

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    running = 0;
                    break;

                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        update_title(window, &view, args.expr);
                    }
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        dragging = 1;
                        last_mx = e.button.x;
                        last_my = e.button.y;
                    }
                    break;

                case SDL_MOUSEBUTTONUP:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        dragging = 0;
                    }
                    break;

                case SDL_MOUSEMOTION:
                    if (dragging) {
                        int w, h;
                        SDL_GetWindowSize(window, &w, &h);
                        TP_Screen screen = { .w = w, .h = h };

                        /* Converte last e current para mundo e pan pelo delta (com sinal "map-drag") */
                        double x0, y0, x1, y1;
                        tp_screen_to_world(&view, screen, last_mx, last_my, &x0, &y0);
                        tp_screen_to_world(&view, screen, e.motion.x, e.motion.y, &x1, &y1);

                        /* arrastar -> move o mundo junto com o mouse */
                        tp_view_pan(&view, -(x1 - x0), -(y1 - y0));

                        last_mx = e.motion.x;
                        last_my = e.motion.y;

                        update_title(window, &view, args.expr);
                    }
                    break;

                case SDL_MOUSEWHEEL: {
                    /* Zoom com âncora no cursor */
                    int w, h;
                    SDL_GetWindowSize(window, &w, &h);
                    TP_Screen screen = { .w = w, .h = h };

                    int mx, my;
                    SDL_GetMouseState(&mx, &my);

                    double ax, ay;
                    tp_screen_to_world(&view, screen, mx, my, &ax, &ay);

                    /* scroll up -> zoom in | scroll down -> zoom out */
                    double factor = 1.0;
                    if (e.wheel.y > 0) factor = 0.85;
                    else if (e.wheel.y < 0) factor = 1.15;

                    if (factor != 1.0) {
                        tp_view_zoom_at(&view, factor, ax, ay);
                        update_title(window, &view, args.expr);
                    }
                } break;

                case SDL_KEYDOWN: {
                    const SDL_Keycode key = e.key.keysym.sym;

                    if (key == SDLK_ESCAPE) {
                        running = 0;
                        break;
                    }

                    /* Mantém WASD e +/- também (útil no notebook) */
                    const double dx = (view.xmax - view.xmin) * 0.05;
                    const double dy = (view.ymax - view.ymin) * 0.05;

                    if (key == SDLK_a) tp_view_pan(&view, -dx, 0.0);
                    if (key == SDLK_d) tp_view_pan(&view, +dx, 0.0);
                    if (key == SDLK_w) tp_view_pan(&view, 0.0, +dy);
                    if (key == SDLK_s) tp_view_pan(&view, 0.0, -dy);

                    if (key == SDLK_EQUALS || key == SDLK_KP_PLUS) tp_view_zoom(&view, 0.85);
                    if (key == SDLK_MINUS  || key == SDLK_KP_MINUS) tp_view_zoom(&view, 1.15);

                    if (key == SDLK_r) view = view0;

                    update_title(window, &view, args.expr);
                } break;

                default:
                    break;
            }
        }

        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        TP_Screen screen = { .w = w, .h = h };

        SDL_SetRenderDrawColor(renderer, args.bg_r, args.bg_g, args.bg_b, 255);
        SDL_RenderClear(renderer);

        tp_draw_grid(renderer, &view, screen);
        tp_draw_axes(renderer, &view, screen);

        if (!is_tuple) {
            tp_draw_function(renderer, &view, screen, expr_ast, args.fg_r, args.fg_g, args.fg_b);
        } else {
            tp_draw_parametric(renderer, &view, screen,
                               expr_ast->as.tuple2.a, expr_ast->as.tuple2.b,
                               tmin, tmax, 3000,
                               args.fg_r, args.fg_g, args.fg_b);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    tp_ast_free(expr_ast);
    return 0;
}
