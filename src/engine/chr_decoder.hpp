#ifndef CHR_DECODER_HPP
#define CHR_DECODER_HPP

#include <vector>
#include <array>
#include <cstdint>

namespace ff1 {

// 64 Standard NES Hardware RGB Palette Colors
extern const std::array<uint32_t, 64> lut_NESPalette;

// 8x8 RGBA Pixel Tile Buffer (64 uint32_t pixels)
using PixelBuffer8x8 = std::array<uint32_t, 64>;

class CHRDecoder {
public:
    // Decodes 16-byte 2-bit planar NES CHR tile with optional horizontal/vertical flipping and sprite transparency
    static PixelBuffer8x8 decode_chr_tile(
        const uint8_t* chr_data_16bytes,
        const std::array<uint8_t, 4>& palette_indices,
        bool flip_x = false,
        bool flip_y = false,
        bool is_sprite = false
    );

    // Decodes a complete bank of CHR tiles (16 KB = 1024 tiles of 16 bytes each)
    static std::vector<PixelBuffer8x8> decode_chr_bank(
        const std::vector<uint8_t>& bank_data,
        const std::array<uint8_t, 4>& palette_indices,
        bool is_sprite = false
    );

    // Decodes 8-byte 1-bit planar NES CHR tile (used in 15-Puzzle numbers)
    static PixelBuffer8x8 decode_1bpp_tile(
        const uint8_t* chr_data_8bytes,
        uint32_t fg_color = 0xFFFFFFFF,
        uint32_t bg_color = 0xFF000000
    );
};

} // namespace ff1

#endif // CHR_DECODER_HPP
