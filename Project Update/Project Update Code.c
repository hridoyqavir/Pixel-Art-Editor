#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

// Size of the pixel grid
#define WIDTH 32
#define HEIGHT 32
#define PIXEL_SIZE 20  // each "pixel" is actually a square of this many pixels

// Struct for RGB color
typedef struct {
    Uint8 r, g, b;
} Color;

// Globals (yep, went the easy way for now)
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

// The 2D grid to hold color values
Color grid[WIDTH][HEIGHT];

// Default drawing color: black
Color currentColor = { 0, 0, 0 };

void initGrid() {
    // Set every pixel in the grid to white initially
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            grid[i][j].r = 255;
            grid[i][j].g = 255;
            grid[i][j].b = 255;
        }
    }
}

// Draw the full grid (pixels + outlines)
void drawGrid() {
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            SDL_Rect cell = { i * PIXEL_SIZE, j * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE };

            // Fill the pixel with its stored color
            SDL_SetRenderDrawColor(renderer, grid[i][j].r, grid[i][j].g, grid[i][j].b, 255);
            SDL_RenderFillRect(renderer, &cell);

            // Optional: draw light gray grid lines (for pixel borders)
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderDrawRect(renderer, &cell);
        }
    }
}

// Safely color a single grid cell
void drawPixel(int x, int y, Color color) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    grid[x][y] = color;
}

int main(int argc, char* argv[]) {
    // Fire up SDL (if this fails, we probably can't do anything useful)
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL failed to initialize: %s\n", SDL_GetError());
        return 1;
    }

    // Create the main window and renderer
    window = SDL_CreateWindow("Pixel Art Editor",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              WIDTH * PIXEL_SIZE,
                              HEIGHT * PIXEL_SIZE,
                              SDL_WINDOW_SHOWN);

    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Setup the blank canvas
    initGrid();

    SDL_Event event;
    bool running = true;

    while (running) {
        // Input event handling
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_MOUSEBUTTONDOWN: {
                    int x = event.button.x / PIXEL_SIZE;
                    int y = event.button.y / PIXEL_SIZE;

                    if (event.button.button == SDL_BUTTON_LEFT) {
                        drawPixel(x, y, currentColor); // draw with current color
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        drawPixel(x, y, (Color){ 255, 255, 255 }); // erase (white)
                    }
                    break;
                }

                case SDL_KEYDOWN:
                    // Keyboard shortcuts for color selection
                    switch (event.key.keysym.sym) {
                        case SDLK_r: currentColor = (Color){255, 0, 0}; break;     // Red
                        case SDLK_g: currentColor = (Color){0, 255, 0}; break;     // Green
                        case SDLK_b: currentColor = (Color){0, 0, 255}; break;     // Blue
                        case SDLK_w: currentColor = (Color){255, 255, 255}; break; // White
                        case SDLK_k: currentColor = (Color){0, 0, 0}; break;       // Black
                        // Might add more colors later...
                    }
                    break;
            }
        }

        // Background color before drawing grid (a bit brighter gray)
        SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
        SDL_RenderClear(renderer);

        drawGrid();  // actually draw the grid

        SDL_RenderPresent(renderer);  // push the result to the window
    }

    // Clean up resources
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
