#include "tp_plot.h"
#include <math.h>

static int tp_isfinite(double x) { return isfinite(x); }

void tp_draw_function(SDL_Renderer *r,
                      const TP_View *v, TP_Screen s,
                      const TP_Node *expr,
                      unsigned char fr, unsigned char fg, unsigned char fb)
{
    SDL_SetRenderDrawColor(r, fr, fg, fb, 255);

    const double y_range = (v->ymax - v->ymin);
    const double jump_break = y_range * 2.0;

    int have_prev = 0;
    int prev_sx = 0, prev_sy = 0;
    double prev_y = 0.0;

    for (int sx = 0; sx < s.w; sx++) {
        double xw = 0.0, dummy = 0.0;
        tp_screen_to_world(v, s, sx, 0, &xw, &dummy);

        double yw = tp_eval(expr, xw);

        if (!tp_isfinite(yw)) { have_prev = 0; continue; }
        if (yw < v->ymin - y_range || yw > v->ymax + y_range) { have_prev = 0; continue; }

        int sy = 0;
        tp_world_to_screen(v, s, xw, yw, NULL, &sy);

        if (have_prev) {
            if (fabs(yw - prev_y) > jump_break) {
                have_prev = 0;
            } else {
                SDL_RenderDrawLine(r, prev_sx, prev_sy, sx, sy);
            }
        }

        have_prev = 1;
        prev_sx = sx;
        prev_sy = sy;
        prev_y = yw;
    }
}

void tp_draw_parametric(SDL_Renderer *r,
                        const TP_View *v, TP_Screen s,
                        const TP_Node *xexpr, const TP_Node *yexpr,
                        double tmin, double tmax, int steps,
                        unsigned char fr, unsigned char fg, unsigned char fb)
{
    if (steps < 100) steps = 100;

    SDL_SetRenderDrawColor(r, fr, fg, fb, 255);

    int have_prev = 0;
    int prev_sx = 0, prev_sy = 0;
    double prev_x = 0.0, prev_y = 0.0;

    for (int i = 0; i < steps; i++) {
        double t = tmin + (tmax - tmin) * ((double)i / (double)(steps - 1));

        double xw = tp_eval(xexpr, t);
        double yw = tp_eval(yexpr, t);

        if (!tp_isfinite(xw) || !tp_isfinite(yw)) {
            have_prev = 0;
            continue;
        }

        int sx, sy;
        tp_world_to_screen(v, s, xw, yw, &sx, &sy);

        if (have_prev) {
            /* quebra se der um salto gigante em coords do mundo (evita "costurar" o desenho) */
            double dx = xw - prev_x;
            double dy = yw - prev_y;
            double dist2 = dx*dx + dy*dy;
            double range = (v->xmax - v->xmin) + (v->ymax - v->ymin);
            double max_jump = range * 0.25;
            if (dist2 > max_jump * max_jump) {
                have_prev = 0;
            } else {
                SDL_RenderDrawLine(r, prev_sx, prev_sy, sx, sy);
            }
        }

        have_prev = 1;
        prev_sx = sx;
        prev_sy = sy;
        prev_x = xw;
        prev_y = yw;
    }
}
