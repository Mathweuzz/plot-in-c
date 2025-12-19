#ifndef TP_PLOT_H
#define TP_PLOT_H

#include <SDL2/SDL.h>
#include "tp_view.h"
#include "tp_render.h"
#include "tp_ast.h"

/* y = f(x) */
void tp_draw_function(SDL_Renderer *r,
                      const TP_View *v, TP_Screen s,
                      const TP_Node *expr,
                      unsigned char fr, unsigned char fg, unsigned char fb);

/* curva paramétrica: (x(t), y(t)) */
void tp_draw_parametric(SDL_Renderer *r,
                        const TP_View *v, TP_Screen s,
                        const TP_Node *xexpr, const TP_Node *yexpr,
                        double tmin, double tmax, int steps,
                        unsigned char fr, unsigned char fg, unsigned char fb);

#endif
