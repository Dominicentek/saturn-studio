#include <SDL2/SDL.h>

#include <string>

#include "font.h"

struct GUIHandle {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* font;
    const char *from, *to;
};

void gui_init_font(struct GUIHandle* handle) {
    int numPixels = 112 * 84;
    int* pixels = (int*)malloc(numPixels * 4);
    for (int i = 0; i < numPixels; i++) {
        pixels[i] = (font_data[i / 8] & (1 << (7 - i % 8))) ? 0xFFFFFFFF : 0x00000000;
    }
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(pixels, 112, 84, 32, 112 * 4, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    handle->font = SDL_CreateTextureFromSurface(handle->renderer, surface);
    SDL_FreeSurface(surface);
    free(pixels);
}

void gui_text(struct GUIHandle* handle, std::string text, int x, int y) {
    for (int i = 0; i < text.length(); i++) {
        unsigned char character = text[i];
        int charX = character % 16;
        int charY = character / 16 - 2;
        SDL_Rect src = (SDL_Rect){ .x = charX * 7, .y = charY * 14, .w = 7, .h = 14 };
        SDL_Rect dst = (SDL_Rect){ .x = x + i * 7, .y = y, .w = 7, .h = 14 };
        SDL_RenderCopy(handle->renderer, handle->font, &src, &dst);
    }
}

void gui_text_centered(struct GUIHandle* handle, std::string text, int x, int y, int w, int h) {
    int tw = text.length() * 7;
    int th = 14;
    if (w == -1) w = tw;
    if (h == -1) h = th;
    x += w / 2 - tw / 2;
    y += h / 2 - th / 2;
    gui_text(handle, text, x, y);
}

void gui_rect(struct GUIHandle* handle, int x, int y, int w, int h, int color) {
    SDL_SetRenderDrawColor(handle->renderer, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SDL_RenderFillRect(handle->renderer, &rect);
}

void gui_update(struct GUIHandle* handle, float progress) {
    if (progress < 0 || progress > 1) progress = 0;
    SDL_SetRenderDrawColor(handle->renderer, 32, 32, 32, 255);
    SDL_RenderClear(handle->renderer);
    gui_text_centered(handle, "Updating Saturn Studio", 0, 4, 264, 14);
    gui_text_centered(handle, std::string(handle->from) + " -> " + handle->to, 0, 21, 264, 14);
    gui_rect(handle, 4, 39, 256, 24, 0x2F2F2FFF);
    gui_rect(handle, 4, 39, 256 * progress, 24, 0x1F9F1FFF);
    gui_text_centered(handle, std::to_string((int)(progress * 100)) + "%", 4, 39, 256, 24);
    SDL_RenderPresent(handle->renderer);
}

struct GUIHandle* gui_init(const char* from, const char* to) {
    struct GUIHandle* handle = new struct GUIHandle;
    gui_init_font(handle);
    SDL_Init(SDL_INIT_VIDEO);
    handle->window = SDL_CreateWindow("Saturn Studio Update Tool", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 264, 68, 0);
    handle->renderer = SDL_CreateRenderer(handle->window, -1, SDL_RENDERER_ACCELERATED);
    handle->from = from;
    handle->to = to;
    gui_init_font(handle);
    gui_update(handle, 0);
    return handle;
}

void gui_quit(struct GUIHandle* handle) {
    SDL_DestroyTexture(handle->font);
    SDL_DestroyRenderer(handle->renderer);
    SDL_DestroyWindow(handle->window);
    delete handle;
}