#ifndef TP_SCREENSHOT_H
#define TP_SCREENSHOT_H

#include <SDL2/SDL.h>

/* Salva o frame atual do renderer como BMP.
   Retorna 0 se OK; !=0 se erro (msg em errbuf se fornecido). */
int tp_screenshot_save_bmp(SDL_Renderer *renderer,
                           int w, int h,
                           const char *path,
                           char *errbuf, int errbuf_sz);

#endif
