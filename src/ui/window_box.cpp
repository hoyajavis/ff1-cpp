#include "window_box.hpp"

namespace ff1 {

void WindowBox::draw_box(uint32_t* pixel_buffer, int pitch, int tile_x, int tile_y, int tile_w, int tile_h) {
    if (!pixel_buffer || tile_w < 2 || tile_h < 2) return;

    int px = tile_x * 8;
    int py = tile_y * 8;
    int pw = tile_w * 8;
    int ph = tile_h * 8;

    uint32_t border_color = 0xFFFFFFFF; // White border
    uint32_t bg_color     = 0xFF000000; // Black background

    // Fill background
    for (int y = py; y < py + ph; ++y) {
        if (y < 0 || y >= 240) continue;
        for (int x = px; x < px + pw; ++x) {
            if (x < 0 || x >= pitch) continue;
            pixel_buffer[y * pitch + x] = bg_color;
        }
    }

    // Outer border (with rounded corners)
    for (int x = px + 1; x < px + pw - 1; ++x) {
        if (x < 0 || x >= pitch) continue;
        if (py >= 0 && py < 240) pixel_buffer[py * pitch + x] = border_color;
        if (py + ph - 1 >= 0 && py + ph - 1 < 240) pixel_buffer[(py + ph - 1) * pitch + x] = border_color;
    }
    for (int y = py + 1; y < py + ph - 1; ++y) {
        if (y < 0 || y >= 240) continue;
        if (px >= 0 && px < pitch) pixel_buffer[y * pitch + px] = border_color;
        if (px + pw - 1 >= 0 && px + pw - 1 < pitch) pixel_buffer[y * pitch + (px + pw - 1)] = border_color;
    }

    // Inner double-line border (classic NES FF1 style, 2 pixels inside)
    if (pw >= 16 && ph >= 16) {
        int ipx = px + 2;
        int ipy = py + 2;
        int ipw = pw - 4;
        int iph = ph - 4;
        for (int x = ipx; x < ipx + ipw; ++x) {
            if (x < 0 || x >= pitch) continue;
            if (ipy >= 0 && ipy < 240) pixel_buffer[ipy * pitch + x] = border_color;
            if (ipy + iph - 1 >= 0 && ipy + iph - 1 < 240) pixel_buffer[(ipy + iph - 1) * pitch + x] = border_color;
        }
        for (int y = ipy; y < ipy + iph; ++y) {
            if (y < 0 || y >= 240) continue;
            if (ipx >= 0 && ipx < pitch) pixel_buffer[y * pitch + ipx] = border_color;
            if (ipx + ipw - 1 >= 0 && ipx + ipw - 1 < pitch) pixel_buffer[y * pitch + (ipx + ipw - 1)] = border_color;
        }
    }
}

} // namespace ff1
