#include <SDL2/SDL.h>
#include <stdio.h>

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    // Inicializa o subsistema de vídeo do SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init falhou: %s\n", SDL_GetError());
        return 1;
    }

    // Cria uma janela simples
    SDL_Window *window = SDL_CreateWindow(
        "TatuPlot - Hello SDL2",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        900, 600,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow falhou: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Cria um renderer (onde desenharemos depois)
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer falhou: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Loop principal: fecha quando o usuário clicar no X da janela
    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            }
        }

        // Limpa a tela (preto)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Apresenta o frame
        SDL_RenderPresent(renderer);
    }

    // Libera recursos
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}