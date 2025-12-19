#include "tp_screenshot.h"
#include <stdio.h>

int tp_screenshot_save_bmp(SDL_Renderer *renderer,
                           int w, int h,
                           const char *path,
                           char *errbuf, int errbuf_sz)
{
    if (errbuf && errbuf_sz > 0) errbuf[0] = '\0';
    if (!renderer || !path || w <= 0 || h <= 0) {
        if (errbuf && errbuf_sz > 0) {
            snprintf(errbuf, errbuf_sz, "parametros invalidos para screenshot");
        }
        return 1;
    }

    /* Surface compatível com o formato mais comum do renderer */
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surf) {
        if (errbuf && errbuf_sz > 0) {
            snprintf(errbuf, errbuf_sz, "SDL_CreateRGBSurfaceWithFormat falhou: %s", SDL_GetError());
        }
        return 2;
    }

    /* Lê pixels do backbuffer atual (depois de desenhar) */
    if (SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, surf->pixels, surf->pitch) != 0) {
        if (errbuf && errbuf_sz > 0) {
            snprintf(errbuf, errbuf_sz, "SDL_RenderReadPixels falhou: %s", SDL_GetError());
        }
        SDL_FreeSurface(surf);
        return 3;
    }

    if (SDL_SaveBMP(surf, path) != 0) {
        if (errbuf && errbuf_sz > 0) {
            snprintf(errbuf, errbuf_sz, "SDL_SaveBMP falhou: %s", SDL_GetError());
        }
        SDL_FreeSurface(surf);
        return 4;
    }

    SDL_FreeSurface(surf);
    return 0;
}
