#ifndef WINDOW_BOX_HPP
#define WINDOW_BOX_HPP

#include <vector>
#include <cstdint>

namespace ff1 {

class WindowBox {
public:
    // Draw classic NES FF1 dialog box border into tile/pixel buffer
    static void draw_box(uint32_t* pixel_buffer, int pitch, int tile_x, int tile_y, int tile_w, int tile_h);
};

} // namespace ff1

#endif // WINDOW_BOX_HPP
