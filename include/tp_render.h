#ifndef TP_RENDER_H
#define TP_RENDER_H

#include <SDL2/SDL.h>
#include "tp_view.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TP_Screen {
    int w;
    int h;
} TP_Screen;

void tp_world_to_screen(const TP_View *v, TP_Screen s,
                        double x, double y, int *sx, int *sy);

void tp_screen_to_world(const TP_View *v, TP_Screen s,
                        int sx, int sy, double *x, double *y);

void tp_draw_grid(SDL_Renderer *r, const TP_View *v, TP_Screen s);
void tp_draw_axes(SDL_Renderer *r, const TP_View *v, TP_Screen s);

#ifdef __cplusplus
}
#endif

#endif
