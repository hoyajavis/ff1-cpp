#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <SDL.h>
#include <cstdint>

namespace ff1 {

class System {
public:
    System(int scale = 3);
    ~System();

    bool init(const char* title = "Final Fantasy I (NES Port in C++)");
    void shutdown();

    void update_texture(const uint32_t* pixel_buffer);
    void render_present();

    bool poll_events();
    bool is_key_pressed(SDL_Scancode code) const;

private:
    int scale_ = 3;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    bool running_ = true;
};

} // namespace ff1

#endif // SYSTEM_HPP
