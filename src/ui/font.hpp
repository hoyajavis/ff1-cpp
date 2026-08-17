#ifndef FONT_HPP
#define FONT_HPP

#include <string>
#include <cstdint>

namespace ff1 {

class Font {
public:
    static void draw_string(uint32_t* pixel_buffer, int pitch, int tile_x, int tile_y, const std::string& text, uint32_t color = 0xFFFFFFFF);
};

} // namespace ff1

#endif // FONT_HPP
