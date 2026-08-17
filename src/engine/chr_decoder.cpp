#include "chr_decoder.hpp"

namespace ff1 {

// Authentic NES NTSC 64-color Hardware RGB Table (0x00..0x3F)
const std::array<uint32_t, 64> lut_NESPalette = {
    0xFF666666, 0xFF002A88, 0xFF1412A7, 0xFF3B00A4, 0xFF5C007E, 0xFF6E0040, 0xFF6C0600, 0xFF561D00,
    0xFF333500, 0xFF0B4800, 0xFF005200, 0xFF004F08, 0xFF00404D, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFADADAD, 0xFF155FD9, 0xFF4240FF, 0xFF7527FE, 0xFFA01ACC, 0xFFB71E7B, 0xFFB53120, 0xFF994E00,
    0xFF6B6D00, 0xFF2F8200, 0xFF058F00, 0xFF008A30, 0xFF007C8D, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFFFFFFF, 0xFF64B0FF, 0xFF9290FF, 0xFFC676FF, 0xFFF36EFF, 0xFFFE6FBF, 0xFFFE816E, 0xFFE59F29,
    0xFFB7C000, 0xFF7CBD00, 0xFF52CA2B, 0xFF45CA75, 0xFF46BDBF, 0xFF4E4E4E, 0xFF000000, 0xFF000000,
    0xFFFFFFFF, 0xFFC0E0FF, 0xFFD3D2FF, 0xFFE8C8FF, 0xFFFBC2FF, 0xFFFEC2EA, 0xFFFEC9C2, 0xFFF5D6A4,
    0xFFE2E4A2, 0xFFCAEA9D, 0xFFB9EEB9, 0xFFB4EED4, 0xFFB5E7F5, 0xFFB8B8B8, 0xFF000000, 0xFF000000
};

PixelBuffer8x8 CHRDecoder::decode_chr_tile(
    const uint8_t* chr_data_16bytes,
    const std::array<uint8_t, 4>& palette_indices,
    bool flip_x,
    bool flip_y,
    bool is_sprite
) {
    PixelBuffer8x8 buffer;
    for (int y = 0; y < 8; ++y) {
        int src_y = flip_y ? (7 - y) : y;
        uint8_t p0 = chr_data_16bytes[src_y];
        uint8_t p1 = chr_data_16bytes[src_y + 8];

        for (int x = 0; x < 8; ++x) {
            int src_x = flip_x ? (7 - x) : x;
            uint8_t bit0 = (p0 >> (7 - src_x)) & 1;
            uint8_t bit1 = (p1 >> (7 - src_x)) & 1;
            uint8_t color_idx = (bit1 << 1) | bit0;

            if (color_idx == 0) {
                if (is_sprite) {
                    buffer[y * 8 + x] = 0x00000000; // Transparent for sprites/OAM
                } else {
                    uint8_t bg_color = palette_indices[0];
                    buffer[y * 8 + x] = lut_NESPalette[bg_color % 64]; // Opaque universal BG color for tiles
                }
            } else {
                uint8_t nes_color = palette_indices[color_idx % 4];
                buffer[y * 8 + x] = lut_NESPalette[nes_color % 64];
            }
        }
    }
    return buffer;
}

std::vector<PixelBuffer8x8> CHRDecoder::decode_chr_bank(
    const std::vector<uint8_t>& bank_data,
    const std::array<uint8_t, 4>& palette_indices,
    bool is_sprite
) {
    std::vector<PixelBuffer8x8> tiles;
    size_t num_tiles = bank_data.size() / 16;
    tiles.reserve(num_tiles);

    for (size_t i = 0; i < num_tiles; ++i) {
        tiles.push_back(decode_chr_tile(&bank_data[i * 16], palette_indices, false, false, is_sprite));
    }
    return tiles;
}

PixelBuffer8x8 CHRDecoder::decode_1bpp_tile(
    const uint8_t* chr_data_8bytes,
    uint32_t fg_color,
    uint32_t bg_color
) {
    PixelBuffer8x8 buffer;
    if (!chr_data_8bytes) {
        buffer.fill(bg_color);
        return buffer;
    }
    for (int y = 0; y < 8; ++y) {
        uint8_t row = chr_data_8bytes[y];
        for (int x = 0; x < 8; ++x) {
            int bit = 7 - x;
            bool is_set = (row & (1 << bit)) != 0;
            buffer[y * 8 + x] = is_set ? fg_color : bg_color;
        }
    }
    return buffer;
}

} // namespace ff1
