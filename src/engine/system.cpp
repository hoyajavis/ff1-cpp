#include "system.hpp"
#include <iostream>

namespace ff1 {

System::System(int scale) : scale_(scale) {}

System::~System() {
    shutdown();
}

bool System::init(const char* title) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL_Init error: " << SDL_GetError() << std::endl;
        return false;
    }

    int win_w = 256 * scale_;
    int win_h = 240 * scale_;

    window_ = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        win_w,
        win_h,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window_) {
        std::cerr << "SDL_CreateWindow error: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }

    texture_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        256,
        240
    );

    return true;
}

void System::shutdown() {
    if (texture_) { SDL_DestroyTexture(texture_); texture_ = nullptr; }
    if (renderer_) { SDL_DestroyRenderer(renderer_); renderer_ = nullptr; }
    if (window_) { SDL_DestroyWindow(window_); window_ = nullptr; }
    SDL_Quit();
}

void System::update_texture(const uint32_t* pixel_buffer) {
    if (texture_ && pixel_buffer) {
        SDL_UpdateTexture(texture_, nullptr, pixel_buffer, 256 * sizeof(uint32_t));
    }
}

void System::render_present() {
    if (renderer_ && texture_) {
        SDL_RenderClear(renderer_);
        SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
        SDL_RenderPresent(renderer_);
    }
}

bool System::poll_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running_ = false;
        }
    }
    return running_;
}

bool System::is_key_pressed(SDL_Scancode code) const {
    const uint8_t* state = SDL_GetKeyboardState(nullptr);
    return state[code] != 0;
}

} // namespace ff1
