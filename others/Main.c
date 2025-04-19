#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 32
#define HEIGHT 32
#define PIXEL_SIZE 20
#define MAX_LAYERS 3
#define MAX_UNDO 10
#define COLOR_BUTTON_SIZE 30
#define NUM_COLORS 16
#define TOOLBAR_HEIGHT 120

typedef struct {
    Uint8 r, g, b;
} Color;

typedef struct {
    Color pixels[HEIGHT][WIDTH];
} Layer;

typedef struct {
    SDL_Rect rect;
    Color color;
} ColorButton;

typedef struct {
    Layer layers[MAX_LAYERS];
    int currentLayer;
    int currentBrush;
    Color currentColor;
    Layer undoStack[MAX_UNDO];
    Layer redoStack[MAX_UNDO];
    int undoTop;
    int redoTop;
} EditorState;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

ColorButton colorButtons[NUM_COLORS];
SDL_Rect brushButtons[3];
SDL_Rect saveButton, loadButton, exportButton, undoButton, redoButton;
SDL_Rect gridToggleButton, eraserButton;
bool showGrid = true;
bool isEraserOn = false;

EditorState editor;

Color palette[NUM_COLORS] = {
    {255, 0, 0}, {0, 0, 255}, {255, 255, 0}, {255, 0, 255},
    {0, 255, 255}, {0, 255, 0}, {0, 0, 0}, {255, 255, 255},
    {64, 64, 64}, {128, 64, 64}, {192, 64, 64}, {224, 224, 224},
    {128, 0, 255}, {255, 128, 0}, {0, 128, 255}, {128, 128, 0}
};

void renderText(const char* text, int x, int y) {
    SDL_Color white = { 255, 255, 255 };
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, white);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dst = { x, y, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void pushUndo() {
    if (editor.undoTop < MAX_UNDO - 1) {
        editor.undoStack[++editor.undoTop] = editor.layers[editor.currentLayer];
        editor.redoTop = -1;
    }
    else {
        for (int i = 0; i < MAX_UNDO - 1; i++) {
            editor.undoStack[i] = editor.undoStack[i + 1];
        }
        editor.undoStack[MAX_UNDO - 1] = editor.layers[editor.currentLayer];
    }
}

void undo() {
    if (editor.undoTop >= 0) {
        editor.redoStack[++editor.redoTop] = editor.layers[editor.currentLayer];
        editor.layers[editor.currentLayer] = editor.undoStack[editor.undoTop--];
    }
}

void redo() {
    if (editor.redoTop >= 0) {
        editor.undoStack[++editor.undoTop] = editor.layers[editor.currentLayer];
        editor.layers[editor.currentLayer] = editor.redoStack[editor.redoTop--];
    }
}

void drawPixel(int x, int y) {
    Color drawColor = isEraserOn ? (Color) { 0, 0, 0 } : editor.currentColor;
    for (int dy = -editor.currentBrush; dy <= editor.currentBrush; dy++) {
        for (int dx = -editor.currentBrush; dx <= editor.currentBrush; dx++) {
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT)
                editor.layers[editor.currentLayer].pixels[ny][nx] = drawColor;
        }
    }
}

void drawCanvas() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            Color c = editor.layers[editor.currentLayer].pixels[y][x];
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
            SDL_Rect rect = { x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE };
            SDL_RenderFillRect(renderer, &rect);
            if (showGrid) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &rect);
            }
        }
    }
}

void drawUI() {
    for (int i = 0; i < NUM_COLORS; i++) {
        SDL_SetRenderDrawColor(renderer,
            colorButtons[i].color.r,
            colorButtons[i].color.g,
            colorButtons[i].color.b,
            255);
        SDL_RenderFillRect(renderer, &colorButtons[i].rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &colorButtons[i].rect);
    }

    for (int i = 0; i < 3; i++) {
        SDL_SetRenderDrawColor(renderer, 100 + i * 50, 100, 100, 255);
        SDL_RenderFillRect(renderer, &brushButtons[i]);
        renderText(i == 0 ? "S" : i == 1 ? "M" : "L",
            brushButtons[i].x + 10, brushButtons[i].y + 5);
    }

    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderFillRect(renderer, &saveButton);
    SDL_RenderFillRect(renderer, &loadButton);
    SDL_RenderFillRect(renderer, &exportButton);
    SDL_RenderFillRect(renderer, &undoButton);
    SDL_RenderFillRect(renderer, &redoButton);
    SDL_RenderFillRect(renderer, &gridToggleButton);
    SDL_RenderFillRect(renderer, &eraserButton);

    renderText("Save", saveButton.x + 5, saveButton.y + 5);
    renderText("Load", loadButton.x + 5, loadButton.y + 5);
    renderText("Export", exportButton.x + 2, exportButton.y + 5);
    renderText("U", undoButton.x + 5, undoButton.y + 2);
    renderText("R", redoButton.x + 5, redoButton.y + 2);
    renderText("Grid", gridToggleButton.x + 2, gridToggleButton.y + 5);
    renderText("Eraser", eraserButton.x + 2, eraserButton.y + 5);
}

void saveToFile(const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) return;
    fwrite(&editor.layers[editor.currentLayer], sizeof(Layer), 1, file);
    fclose(file);
}

void loadFromFile(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return;
    fread(&editor.layers[editor.currentLayer], sizeof(Layer), 1, file);
    fclose(file);
}

void exportToBMP(const char* filename) {
    SDL_Surface* surface = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++) {
            Color c = editor.layers[editor.currentLayer].pixels[y][x];
            Uint32* pixel = (Uint32*)((Uint8*)surface->pixels + y * surface->pitch + x * 4);
            *pixel = SDL_MapRGB(surface->format, c.r, c.g, c.b);
        }
    SDL_SaveBMP(surface, filename);
    SDL_FreeSurface(surface);
}

bool inside(SDL_Rect r, int x, int y) {
    return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h;
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    window = SDL_CreateWindow("Advanced Pixel Editor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH * PIXEL_SIZE, HEIGHT * PIXEL_SIZE + TOOLBAR_HEIGHT,
        SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 14);

    memset(&editor, 0, sizeof(editor));
    editor.currentColor = palette[0];
    editor.undoTop = -1;
    editor.redoTop = -1;

    for (int i = 0; i < NUM_COLORS; i++) {
        colorButtons[i] = (ColorButton){
            {10 + i * 35, HEIGHT * PIXEL_SIZE + 10, COLOR_BUTTON_SIZE, COLOR_BUTTON_SIZE},
            palette[i] };
    }

    for (int i = 0; i < 3; i++) {
        brushButtons[i] = (SDL_Rect){ 10 + i * 35, HEIGHT * PIXEL_SIZE + 50, 30, 30 };
    }

    saveButton = (SDL_Rect){ 140, HEIGHT * PIXEL_SIZE + 50, 60, 30 };
    loadButton = (SDL_Rect){ 210, HEIGHT * PIXEL_SIZE + 50, 60, 30 };
    exportButton = (SDL_Rect){ 280, HEIGHT * PIXEL_SIZE + 50, 70, 30 };
    undoButton = (SDL_Rect){ 360, HEIGHT * PIXEL_SIZE + 50, 30, 30 };
    redoButton = (SDL_Rect){ 400, HEIGHT * PIXEL_SIZE + 50, 30, 30 };
    gridToggleButton = (SDL_Rect){ 440, HEIGHT * PIXEL_SIZE + 50, 60, 30 };
    eraserButton = (SDL_Rect){ 510, HEIGHT * PIXEL_SIZE + 50, 70, 30 };

    SDL_Rect canvasRect = { 0, 0, WIDTH * PIXEL_SIZE, HEIGHT * PIXEL_SIZE };
    bool running = true;
    bool isDrawing = false;
    bool undoPushedThisStroke = false;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x;
                int y = event.button.y;

                if (inside(canvasRect, x, y)) {
                    int gx = x / PIXEL_SIZE;
                    int gy = y / PIXEL_SIZE;
                    isDrawing = true;
                    undoPushedThisStroke = false;
                    drawPixel(gx, gy);
                }
                else {
                    for (int i = 0; i < NUM_COLORS; i++)
                        if (inside(colorButtons[i].rect, x, y)) {
                            editor.currentColor = colorButtons[i].color;
                            isEraserOn = false;
                        }

                    for (int i = 0; i < 3; i++)
                        if (inside(brushButtons[i], x, y))
                            editor.currentBrush = i;

                    if (inside(saveButton, x, y)) saveToFile("save.dat");
                    if (inside(loadButton, x, y)) loadFromFile("save.dat");
                    if (inside(exportButton, x, y)) exportToBMP("export.bmp");
                    if (inside(undoButton, x, y)) undo();
                    if (inside(redoButton, x, y)) redo();
                    if (inside(gridToggleButton, x, y)) showGrid = !showGrid;
                    if (inside(eraserButton, x, y)) isEraserOn = !isEraserOn;
                }
            }

            else if (event.type == SDL_MOUSEBUTTONUP) {
                isDrawing = false;
                undoPushedThisStroke = false;
            }

            else if (event.type == SDL_MOUSEMOTION && isDrawing) {
                int x = event.motion.x;
                int y = event.motion.y;
                if (inside(canvasRect, x, y)) {
                    int gx = x / PIXEL_SIZE;
                    int gy = y / PIXEL_SIZE;
                    if (!undoPushedThisStroke) {
                        pushUndo();
                        undoPushedThisStroke = true;
                    }
                    drawPixel(gx, gy);
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderClear(renderer);
        drawCanvas();
        drawUI();
        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

