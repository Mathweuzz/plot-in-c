#include "tp_render.h"
#include <math.h>

/* step "bonito" (1-2-5 * 10^k) */
static double tp_nice_step(double range, int target_lines) {
    if (range <= 0.0) return 1.0;
    if (target_lines < 2) target_lines = 2;

    const double raw = range / (double)target_lines;
    const double exp10 = floor(log10(raw));
    const double base = pow(10.0, exp10);
    const double frac = raw / base;

    double nice_frac = 1.0;
    if (frac < 1.5) nice_frac = 1.0;
    else if (frac < 3.5) nice_frac = 2.0;
    else if (frac < 7.5) nice_frac = 5.0;
    else nice_frac = 10.0;

    return nice_frac * base;
}

static double tp_floor_to_step(double x, double step) {
    return floor(x / step) * step;
}

void tp_world_to_screen(const TP_View *v, TP_Screen s,
                        double x, double y, int *sx, int *sy) {
    const double nx = (x - v->xmin) / (v->xmax - v->xmin);
    const double ny = (y - v->ymin) / (v->ymax - v->ymin);

    int px = (int)lround(nx * (double)(s.w - 1));
    int py = (int)lround((1.0 - ny) * (double)(s.h - 1)); /* Y invertido */

    if (sx) *sx = px;
    if (sy) *sy = py;
}

void tp_screen_to_world(const TP_View *v, TP_Screen s,
                        int sx, int sy, double *x, double *y) {
    const double nx = (double)sx / (double)(s.w - 1);
    const double ny = 1.0 - ((double)sy / (double)(s.h - 1));

    if (x) *x = v->xmin + nx * (v->xmax - v->xmin);
    if (y) *y = v->ymin + ny * (v->ymax - v->ymin);
}

void tp_draw_grid(SDL_Renderer *r, const TP_View *v, TP_Screen s) {
    const double x_range = v->xmax - v->xmin;
    const double y_range = v->ymax - v->ymin;

    const double x_step = tp_nice_step(x_range, 10);
    const double y_step = tp_nice_step(y_range, 10);

    SDL_SetRenderDrawColor(r, 40, 40, 40, 255);

    double x0 = tp_floor_to_step(v->xmin, x_step);
    for (double x = x0; x <= v->xmax; x += x_step) {
        int sx, sy1, sy2;
        tp_world_to_screen(v, s, x, v->ymin, &sx, &sy1);
        tp_world_to_screen(v, s, x, v->ymax, &sx, &sy2);
        SDL_RenderDrawLine(r, sx, sy1, sx, sy2);
    }

    double y0 = tp_floor_to_step(v->ymin, y_step);
    for (double y = y0; y <= v->ymax; y += y_step) {
        int sx1, sx2, sy;
        tp_world_to_screen(v, s, v->xmin, y, &sx1, &sy);
        tp_world_to_screen(v, s, v->xmax, y, &sx2, &sy);
        SDL_RenderDrawLine(r, sx1, sy, sx2, sy);
    }
}

void tp_draw_axes(SDL_Renderer *r, const TP_View *v, TP_Screen s) {
    SDL_SetRenderDrawColor(r, 160, 160, 160, 255);

    if (v->xmin <= 0.0 && v->xmax >= 0.0) {
        int sx, sy1, sy2;
        tp_world_to_screen(v, s, 0.0, v->ymin, &sx, &sy1);
        tp_world_to_screen(v, s, 0.0, v->ymax, &sx, &sy2);
        SDL_RenderDrawLine(r, sx, sy1, sx, sy2);
    }

    if (v->ymin <= 0.0 && v->ymax >= 0.0) {
        int sx1, sx2, sy;
        tp_world_to_screen(v, s, v->xmin, 0.0, &sx1, &sy);
        tp_world_to_screen(v, s, v->xmax, 0.0, &sx2, &sy);
        SDL_RenderDrawLine(r, sx1, sy, sx2, sy);
    }

    const double x_range = v->xmax - v->xmin;
    const double y_range = v->ymax - v->ymin;
    const double x_step = tp_nice_step(x_range, 10);
    const double y_step = tp_nice_step(y_range, 10);
    const int tick = 6;

    if (v->ymin <= 0.0 && v->ymax >= 0.0) {
        double x0 = tp_floor_to_step(v->xmin, x_step);
        for (double x = x0; x <= v->xmax; x += x_step) {
            int sx, sy;
            tp_world_to_screen(v, s, x, 0.0, &sx, &sy);
            SDL_RenderDrawLine(r, sx, sy - tick, sx, sy + tick);
        }
    }

    if (v->xmin <= 0.0 && v->xmax >= 0.0) {
        double y0 = tp_floor_to_step(v->ymin, y_step);
        for (double y = y0; y <= v->ymax; y += y_step) {
            int sx, sy;
            tp_world_to_screen(v, s, 0.0, y, &sx, &sy);
            SDL_RenderDrawLine(r, sx - tick, sy, sx + tick, sy);
        }
    }
}
