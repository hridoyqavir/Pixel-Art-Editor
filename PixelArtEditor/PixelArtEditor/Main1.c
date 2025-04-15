#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

#define WIDTH 32
#define HEIGHT 32
#define PIXEL_SIZE 20

typedef struct {
    Uint8 r, g, b;
} Color;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
Color grid[WIDTH][HEIGHT];
Color currentColor = { 0, 0, 0 }; // Default: Black

void initGrid() {
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            grid[i][j].r = 255; // White background
            grid[i][j].g = 255;
            grid[i][j].b = 255;
        }
    }
}

void drawGrid() {
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            SDL_Rect rect = { i * PIXEL_SIZE, j * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE };
            SDL_SetRenderDrawColor(renderer, grid[i][j].r, grid[i][j].g, grid[i][j].b, 255);
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderDrawRect(renderer, &rect); // Grid lines
        }
    }
}

void drawPixel(int x, int y, Color color) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        grid[x][y] = color;
    }
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Pixel Art Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * PIXEL_SIZE, HEIGHT * PIXEL_SIZE, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    bool running = true;
    SDL_Event event;
    initGrid();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x / PIXEL_SIZE;
                int y = event.button.y / PIXEL_SIZE;
                if (event.button.button == SDL_BUTTON_LEFT) {
                    drawPixel(x, y, currentColor);
                }
                else if (event.button.button == SDL_BUTTON_RIGHT) {
                    drawPixel(x, y, (Color) { 255, 255, 255 });
                }
            }
            else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_r: currentColor = (Color){ 255, 0, 0 }; break; // Red
                case SDLK_g: currentColor = (Color){ 0, 255, 0 }; break; // Green
                case SDLK_b: currentColor = (Color){ 0, 0, 255 }; break; // Blue
                case SDLK_w: currentColor = (Color){ 255, 255, 255 }; break; // White
                case SDLK_k: currentColor = (Color){ 0, 0, 0 }; break; // Black
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
        SDL_RenderClear(renderer);
        drawGrid();
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
