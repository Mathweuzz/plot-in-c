#include "tp_plot.h"
#include <math.h>

static int tp_isfinite(double x) {
    return isfinite(x);
}

void tp_draw_function(SDL_Renderer *r,
                      const TP_View *v, TP_Screen s,
                      const TP_Node *expr,
                      unsigned char fr, unsigned char fg, unsigned char fb)
{
    SDL_SetRenderDrawColor(r, fr, fg, fb, 255);

    const double y_range = (v->ymax - v->ymin);
    const double jump_break = y_range * 2.0; /* heurística */

    int have_prev = 0;
    int prev_sx = 0, prev_sy = 0;
    double prev_y = 0.0;

    for (int sx = 0; sx < s.w; sx++) {
        double xw = 0.0, dummy = 0.0;
        tp_screen_to_world(v, s, sx, 0, &xw, &dummy);

        double yw = tp_eval(expr, xw);

        if (!tp_isfinite(yw)) {
            have_prev = 0;
            continue;
        }

        if (yw < v->ymin - y_range || yw > v->ymax + y_range) {
            have_prev = 0;
            continue;
        }

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
