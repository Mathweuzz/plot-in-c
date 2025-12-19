#include <SDL2/SDL.h>
#include <stdio.h>

#include "tp_view.h"
#include "tp_render.h"

static void update_window_title(SDL_Window *w, const TP_View *v) {
    char buf[256];
    /* Sem depender de locale/text rendering: só título */
    snprintf(buf, sizeof(buf),
             "TatuPlot | x:[%.3g, %.3g] y:[%.3g, %.3g] | (WASD pan, +/- zoom, ESC sair)",
             v->xmin, v->xmax, v->ymin, v->ymax);
    SDL_SetWindowTitle(w, buf);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init falhou: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "TatuPlot",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        900, 600,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow falhou: %s\n", SDL_GetError());
        SDL_Quit();
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
        return 1;
    }

    /* Viewport inicial (mundo matemático) */
    TP_View view = {
        .xmin = -10.0, .xmax = 10.0,
        .ymin = -10.0, .ymax = 10.0
    };

    update_window_title(window, &view);

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
                    update_window_title(window, &view);
                }
                break;

            case SDL_KEYDOWN: {
                const SDL_Keycode key = e.key.keysym.sym;

                if (key == SDLK_ESCAPE) {
                    running = 0;
                    break;
                }

                /* Pan: desloca 5% do tamanho atual do viewport */
                const double dx = (view.xmax - view.xmin) * 0.05;
                const double dy = (view.ymax - view.ymin) * 0.05;

                if (key == SDLK_a) tp_view_pan(&view, -dx, 0.0);
                if (key == SDLK_d) tp_view_pan(&view, +dx, 0.0);
                if (key == SDLK_w) tp_view_pan(&view, 0.0, +dy);
                if (key == SDLK_s) tp_view_pan(&view, 0.0, -dy);

                /* Zoom: + / - (também aceita keypad) */
                if (key == SDLK_EQUALS || key == SDLK_KP_PLUS) {
                    tp_view_zoom(&view, 0.85); /* zoom in */
                }
                if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
                    tp_view_zoom(&view, 1.15); /* zoom out */
                }

                update_window_title(window, &view);
            } break;

            default:
                break;
            }
        }

        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        TP_Screen screen = { .w = w, .h = h };

        /* Fundo */
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        /* Grid e eixos */
        tp_draw_grid(renderer, &view, screen);
        tp_draw_axes(renderer, &view, screen);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}