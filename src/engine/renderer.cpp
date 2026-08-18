#include "renderer.hpp"
#include "chr_decoder.hpp"
#include "ui/font.hpp"
#include "ui/window_box.hpp"
#include "core/menu_engine.hpp"
#include <algorithm>
#include <iostream>

namespace ff1 {

// Exact NES Disassembly Table: lut_PlayerMapmanSprTbl (bank_0F.asm line 8751)
// 8 entries (4 directions x 2 animation frames)
// Each entry has 8 bytes: {UL_tile, UL_attr, DL_tile, DL_attr, UR_tile, UR_attr, DR_tile, DR_attr}
static const uint8_t lut_PlayerMapmanSprTbl[8][8] = {
    {0x09, 0x40, 0x0B, 0x41, 0x08, 0x40, 0x0A, 0x41}, // Right, frame 0
    {0x0D, 0x40, 0x0F, 0x41, 0x0C, 0x40, 0x0E, 0x41}, // Right, frame 1
    {0x08, 0x00, 0x0A, 0x01, 0x09, 0x00, 0x0B, 0x01}, // Left,  frame 0
    {0x0C, 0x00, 0x0E, 0x01, 0x0D, 0x00, 0x0F, 0x01}, // Left,  frame 1
    {0x04, 0x00, 0x06, 0x01, 0x05, 0x00, 0x07, 0x01}, // Up,    frame 0
    {0x04, 0x00, 0x07, 0x41, 0x05, 0x00, 0x06, 0x41}, // Up,    frame 1
    {0x00, 0x00, 0x02, 0x01, 0x01, 0x00, 0x03, 0x01}, // Down,  frame 0
    {0x00, 0x00, 0x03, 0x41, 0x01, 0x00, 0x02, 0x41}  // Down,  frame 1
};

// NES PPU Overworld BG Palette Mapping
static std::array<uint8_t, 4> get_ow_pal(uint8_t pal_attr) {
    switch (pal_attr & 0x03) {
        case 0: return {0x0F, 0x1A, 0x2A, 0x0A};
        case 1: return {0x0F, 0x12, 0x22, 0x16};
        case 2: return {0x0F, 0x28, 0x16, 0x30};
        case 3: return {0x0F, 0x00, 0x10, 0x20};
        default: return {0x0F, 0x1A, 0x2A, 0x30};
    }
}

// NES PPU Town/Standard Map BG Palette Mapping
static std::array<uint8_t, 4> get_sm_pal(uint8_t pal_attr) {
    switch (pal_attr & 0x03) {
        case 0: return {0x0F, 0x30, 0x10, 0x00};
        case 1: return {0x0F, 0x30, 0x16, 0x27};
        case 2: return {0x0F, 0x30, 0x12, 0x22};
        case 3: return {0x0F, 0x30, 0x17, 0x07};
        default: return {0x0F, 0x30, 0x10, 0x00};
    }
}

Renderer::Renderer(int width, int height)
    : width_(width), height_(height), frame_counter_(0), buffer_(width * height, 0xFF000000) {}

void Renderer::clear(uint32_t color) {
    std::fill(buffer_.begin(), buffer_.end(), color);
    frame_counter_++;
}

void Renderer::draw_rect(int px, int py, int pw, int ph, uint32_t color) {
    for (int y = py; y < py + ph; ++y) {
        if (y < 0 || y >= height_) continue;
        for (int x = px; x < px + pw; ++x) {
            if (x < 0 || x >= width_) continue;
            buffer_[y * width_ + x] = color;
        }
    }
}

void Renderer::draw_chr_tile(int px, int py, const PixelBuffer8x8& tile) {
    for (int y = 0; y < 8; ++y) {
        int screen_y = py + y;
        if (screen_y < 0 || screen_y >= height_) continue;
        for (int x = 0; x < 8; ++x) {
            int screen_x = px + x;
            if (screen_x < 0 || screen_x >= width_) continue;
            uint32_t pixel = tile[y * 8 + x];
            if ((pixel & 0xFF000000) != 0) {
                buffer_[screen_y * width_ + screen_x] = pixel;
            }
        }
    }
}

void Renderer::draw_map(const MapEngine& map, const DataLoader& loader, int center_x, int center_y) {
    int start_tile_x = center_x - 8;
    int start_tile_y = center_y - 7;

    bool is_overworld = (map.get_map_type() == MapType::OVERWORLD);
    uint8_t map_id = map.get_current_map_id();

    const auto& bank_00 = loader.get_chr_bank_00();
    const auto& bank_02 = loader.get_chr_bank_02();
    const auto& bank_03 = loader.get_chr_bank_03();
    const auto& bank_09 = loader.get_chr_bank_09();

    if (bank_00.size() < 0x3000) {
        clear(is_overworld ? 0xFF145214 : 0xFF303030);
        return;
    }

    uint8_t tileset_id = 0;
    if (!is_overworld && loader.get_tileset_assignments().size() > map_id) {
        tileset_id = loader.get_tileset_assignments()[map_id] & 0x07;
    }

    auto decode_subtile = [&](uint8_t sub_idx, const std::array<uint8_t, 4>& pal) -> PixelBuffer8x8 {
        if (is_overworld) {
            size_t o = static_cast<size_t>(sub_idx) * 16;
            if (o + 16 <= bank_02.size()) {
                return CHRDecoder::decode_chr_tile(&bank_02[o], pal, false, false, false);
            }
        } else {
            if (sub_idx < 128) {
                size_t o = static_cast<size_t>(tileset_id) * 2048 + static_cast<size_t>(sub_idx) * 16;
                if (o + 16 <= bank_03.size()) {
                    return CHRDecoder::decode_chr_tile(&bank_03[o], pal, false, false, false);
                }
            } else {
                size_t o = 0x0800 + static_cast<size_t>(sub_idx - 128) * 16;
                if (o + 16 <= bank_09.size()) {
                    return CHRDecoder::decode_chr_tile(&bank_09[o], pal, false, false, false);
                }
            }
        }
        PixelBuffer8x8 empty{};
        empty.fill(lut_NESPalette[pal[0] % 64]);
        return empty;
    };

    for (int ty = 0; ty < 15; ++ty) {
        for (int tx = 0; tx < 16; ++tx) {
            int world_x = start_tile_x + tx;
            int world_y = start_tile_y + ty;

            uint8_t tile = map.get_tile_at(world_x, world_y);
            size_t tile_idx = static_cast<size_t>(tile & 0x7F);

            uint8_t tl_idx = 0;
            uint8_t tr_idx = 0;
            uint8_t bl_idx = 0;
            uint8_t br_idx = 0;
            uint8_t pal_attr = 0;

            if (is_overworld) {
                // Exact Disassembly Offsets in bank_00.dat ($8000 + offset)
                tl_idx = bank_00[0x0100 + tile_idx];
                tr_idx = bank_00[0x0180 + tile_idx];
                bl_idx = bank_00[0x0200 + tile_idx];
                br_idx = bank_00[0x0280 + tile_idx];
                pal_attr = bank_00[0x0300 + tile_idx] & 0x03;
            } else {
                // Standard map TSA in bank_00.dat ($9000 + tileset * 512)
                size_t tsa_base = 0x1000 + (tileset_id * 512);
                tl_idx = bank_00[tsa_base + tile_idx];
                tr_idx = bank_00[tsa_base + 0x80 + tile_idx];
                bl_idx = bank_00[tsa_base + 0x100 + tile_idx];
                br_idx = bank_00[tsa_base + 0x180 + tile_idx];
                pal_attr = bank_00[0x0400 + (tileset_id * 128) + tile_idx] & 0x03;
            }

            auto palette = is_overworld ? loader.get_overworld_palette(pal_attr)
                                        : loader.get_standard_map_palette(map_id, pal_attr);

            // Smooth palette shimmer animation EXCLUSIVELY for authentic ocean, rivers, and coastline transitions
            if (is_overworld && pal_attr == 2) {
                // Ocean (0x17, 0x07, 0x16, 0x18, 0x27), Coastline diagonals (0x06, 0x08, 0x26, 0x28), Rivers (0x40, 0x41, 0x44, 0x46, 0x50, 0x51)
                bool is_water_or_coast = (tile_idx >= 0x06 && tile_idx <= 0x08) ||
                                         (tile_idx >= 0x16 && tile_idx <= 0x18) ||
                                         (tile_idx >= 0x26 && tile_idx <= 0x28) ||
                                         (tile_idx >= 0x40 && tile_idx <= 0x41) ||
                                         (tile_idx == 0x44 || tile_idx == 0x46) ||
                                         (tile_idx >= 0x50 && tile_idx <= 0x51);
                if (is_water_or_coast) {
                    static const uint8_t wave_shimmer[4] = {0x21, 0x31, 0x22, 0x32};
                    uint8_t step = static_cast<uint8_t>((frame_counter_ / 16) % 4);
                    palette[2] = wave_shimmer[step];
                    palette[3] = wave_shimmer[(step + 1) % 4];
                }
            }

            PixelBuffer8x8 subtile_tl = decode_subtile(tl_idx, palette);
            PixelBuffer8x8 subtile_tr = decode_subtile(tr_idx, palette);
            PixelBuffer8x8 subtile_bl = decode_subtile(bl_idx, palette);
            PixelBuffer8x8 subtile_br = decode_subtile(br_idx, palette);

            draw_chr_tile(tx * 16, ty * 16, subtile_tl);
            draw_chr_tile(tx * 16 + 8, ty * 16, subtile_tr);
            draw_chr_tile(tx * 16, ty * 16 + 8, subtile_bl);
            draw_chr_tile(tx * 16 + 8, ty * 16 + 8, subtile_br);
        }
    }
}

void Renderer::draw_player(const PartyCharacter& lead_char, const DataLoader& loader, Direction facing, VehicleType vehicle) {
    int px = 8 * 16;
    int py = 7 * 16;

    std::array<uint8_t, 4> pal0 = loader.get_player_palette(lead_char.char_class, 0);
    std::array<uint8_t, 4> pal1 = loader.get_player_palette(lead_char.char_class, 1);

    // Map Direction + 2-Frame Walk Animation Cycle
    int dir_offset = 6; // Default Down
    if (facing == Direction::RIGHT) dir_offset = 0;
    else if (facing == Direction::LEFT) dir_offset = 2;
    else if (facing == Direction::UP) dir_offset = 4;
    else if (facing == Direction::DOWN) dir_offset = 6;

    int anim_frame = static_cast<int>((frame_counter_ / 16) % 2);
    int tbl_idx = dir_offset + anim_frame;

    const uint8_t* spr_tbl = lut_PlayerMapmanSprTbl[tbl_idx];
    const auto& bank_02 = loader.get_chr_bank_02();

    // Vehicle overrides: Ship = 0x1C00, Airship = 0x1D00, Player = 0x1000 + class * 0x100
    size_t hero_class_offset = 0x1000 + (static_cast<size_t>(lead_char.char_class) % 12) * 0x100;
    if (vehicle == VehicleType::SHIP) {
        hero_class_offset = 0x1C00;
        pal0 = {0x0F, 0x16, 0x30, 0x27};
        pal1 = {0x0F, 0x16, 0x30, 0x27};
    } else if (vehicle == VehicleType::AIRSHIP) {
        hero_class_offset = 0x1D00;
        pal0 = {0x0F, 0x28, 0x30, 0x12};
        pal1 = {0x0F, 0x28, 0x30, 0x12};
        // Draw airship ground shadow
        for (int sy = 0; sy < 4; ++sy) {
            for (int sx = 2; sx < 14; ++sx) {
                int spx = px + sx;
                int spy = py + 12 + sy;
                if (spx < width_ && spy < height_) {
                    buffer_[spy * width_ + spx] = 0xFF080810;
                }
            }
        }
        py -= 4; // Elevate in the air
    }

    size_t o_ul = hero_class_offset + (static_cast<size_t>(spr_tbl[0]) * 16);
    size_t o_dl = hero_class_offset + (static_cast<size_t>(spr_tbl[2]) * 16);
    size_t o_ur = hero_class_offset + (static_cast<size_t>(spr_tbl[4]) * 16);
    size_t o_dr = hero_class_offset + (static_cast<size_t>(spr_tbl[6]) * 16);

    // NES PPU OAM bit 6 = H-flip, bit 7 = V-flip
    bool ul_fx = (spr_tbl[1] & 0x40) != 0; bool ul_fy = (spr_tbl[1] & 0x80) != 0;
    bool dl_fx = (spr_tbl[3] & 0x40) != 0; bool dl_fy = (spr_tbl[3] & 0x80) != 0;
    bool ur_fx = (spr_tbl[5] & 0x40) != 0; bool ur_fy = (spr_tbl[5] & 0x80) != 0;
    bool dr_fx = (spr_tbl[7] & 0x40) != 0; bool dr_fy = (spr_tbl[7] & 0x80) != 0;

    // Palette attribute bit 0 selects pal0 vs pal1
    const auto& pal_ul = (spr_tbl[1] & 1) ? pal1 : pal0;
    const auto& pal_dl = (spr_tbl[3] & 1) ? pal1 : pal0;
    const auto& pal_ur = (spr_tbl[5] & 1) ? pal1 : pal0;
    const auto& pal_dr = (spr_tbl[7] & 1) ? pal1 : pal0;

    if (o_ul + 16 <= bank_02.size() && o_dl + 16 <= bank_02.size() &&
        o_ur + 16 <= bank_02.size() && o_dr + 16 <= bank_02.size()) {

        PixelBuffer8x8 subtile_ul = CHRDecoder::decode_chr_tile(&bank_02[o_ul], pal_ul, ul_fx, ul_fy, true);
        PixelBuffer8x8 subtile_dl = CHRDecoder::decode_chr_tile(&bank_02[o_dl], pal_dl, dl_fx, dl_fy, true);
        PixelBuffer8x8 subtile_ur = CHRDecoder::decode_chr_tile(&bank_02[o_ur], pal_ur, ur_fx, ur_fy, true);
        PixelBuffer8x8 subtile_dr = CHRDecoder::decode_chr_tile(&bank_02[o_dr], pal_dr, dr_fx, dr_fy, true);

        draw_chr_tile(px, py, subtile_ul);
        draw_chr_tile(px, py + 8, subtile_dl);
        draw_chr_tile(px + 8, py, subtile_ur);
        draw_chr_tile(px + 8, py + 8, subtile_dr);
    } else {
        uint32_t char_color = lut_NESPalette[pal0[2] % 64];
        draw_rect(px + 2, py + 1, 12, 14, char_color);
    }
}

void Renderer::draw_npc(int tile_x, int tile_y, int camera_x, int camera_y, const DataLoader& loader, uint32_t color) {
    (void)color;
    int screen_tx = tile_x - (camera_x - 8);
    int screen_ty = tile_y - (camera_y - 7);

    if (screen_tx >= 0 && screen_tx < 16 && screen_ty >= 0 && screen_ty < 15) {
        int px = screen_tx * 16;
        int py = screen_ty * 16;

        std::array<uint8_t, 4> pal = {0x0F, 0x28, 0x30, 0x11};
        const auto& bank_02 = loader.get_chr_bank_02();

        const uint8_t* spr_tbl = lut_PlayerMapmanSprTbl[6]; // Facing down

        // OW objects CHR at bank_02 offset 0x1C00 (PPU $1100)
        size_t npc_offset = 0x1C00;

        size_t o_ul = npc_offset + (static_cast<size_t>(spr_tbl[0]) * 16);
        size_t o_dl = npc_offset + (static_cast<size_t>(spr_tbl[2]) * 16);
        size_t o_ur = npc_offset + (static_cast<size_t>(spr_tbl[4]) * 16);
        size_t o_dr = npc_offset + (static_cast<size_t>(spr_tbl[6]) * 16);

        bool ul_fx = (spr_tbl[1] & 0x40) != 0; bool ul_fy = (spr_tbl[1] & 0x80) != 0;
        bool dl_fx = (spr_tbl[3] & 0x40) != 0; bool dl_fy = (spr_tbl[3] & 0x80) != 0;
        bool ur_fx = (spr_tbl[5] & 0x40) != 0; bool ur_fy = (spr_tbl[5] & 0x80) != 0;
        bool dr_fx = (spr_tbl[7] & 0x40) != 0; bool dr_fy = (spr_tbl[7] & 0x80) != 0;

        if (o_ul + 16 <= bank_02.size() && o_dl + 16 <= bank_02.size() &&
            o_ur + 16 <= bank_02.size() && o_dr + 16 <= bank_02.size()) {

            PixelBuffer8x8 subtile_ul = CHRDecoder::decode_chr_tile(&bank_02[o_ul], pal, ul_fx, ul_fy, true);
            PixelBuffer8x8 subtile_dl = CHRDecoder::decode_chr_tile(&bank_02[o_dl], pal, dl_fx, dl_fy, true);
            PixelBuffer8x8 subtile_ur = CHRDecoder::decode_chr_tile(&bank_02[o_ur], pal, ur_fx, ur_fy, true);
            PixelBuffer8x8 subtile_dr = CHRDecoder::decode_chr_tile(&bank_02[o_dr], pal, dr_fx, dr_fy, true);

            draw_chr_tile(px, py, subtile_ul);
            draw_chr_tile(px, py + 8, subtile_dl);
            draw_chr_tile(px + 8, py, subtile_ur);
            draw_chr_tile(px + 8, py + 8, subtile_dr);
        } else {
            draw_rect(px + 3, py + 2, 10, 12, 0xFF40E0D0);
        }
    }
}

void Renderer::draw_battle(const BattleEngine& battle, const GameSaveData& save_data, const DataLoader& loader) {
    // 1. Draw Upper Battle Arena Backdrop (Y=0..128)
    for (int y = 0; y < 128; ++y) {
        uint32_t bg = (y < 76) ? 0xFF0A1428 : 0xFF142438;
        for (int x = 0; x < width_; ++x) {
            buffer_[y * width_ + x] = bg;
        }
    }
    // Arena horizon divider line
    for (int x = 0; x < width_; ++x) {
        buffer_[76 * width_ + x] = 0xFF384C64;
    }

    // 2. Draw Active Monsters
    const auto& monsters = battle.get_monsters();
    const auto& formation = battle.get_formation();
    const auto& bank_07 = loader.get_chr_bank_07();
    const auto& bank_08 = loader.get_chr_bank_08();

    static const int monster_slots_x[4] = {36, 92, 36, 92};
    static const int monster_slots_y[4] = {32, 32, 72, 72};

    for (size_t m = 0; m < monsters.size(); ++m) {
        if (!monsters[m].alive) continue;

        int mx = (m < 4) ? monster_slots_x[m] : (24 + (static_cast<int>(m) % 3) * 40);
        int my = (m < 4) ? monster_slots_y[m] : (32 + (static_cast<int>(m) / 3) * 32);

        uint8_t pal_id = (m < 2) ? formation.palette_id[0] : formation.palette_id[1];
        std::array<uint8_t, 4> mpal = loader.get_monster_palette(pal_id);

        size_t m_tile_idx = (static_cast<size_t>(monsters[m].enemy_id) % 64) * 4;
        const auto& m_bank = (monsters[m].enemy_id < 64 && !bank_07.empty()) ? bank_07 : bank_08;

        if (m_tile_idx * 16 + 64 <= m_bank.size()) {
            PixelBuffer8x8 t0 = CHRDecoder::decode_chr_tile(&m_bank[(m_tile_idx + 0) * 16], mpal, false, false, true);
            PixelBuffer8x8 t1 = CHRDecoder::decode_chr_tile(&m_bank[(m_tile_idx + 1) * 16], mpal, false, false, true);
            PixelBuffer8x8 t2 = CHRDecoder::decode_chr_tile(&m_bank[(m_tile_idx + 2) * 16], mpal, false, false, true);
            PixelBuffer8x8 t3 = CHRDecoder::decode_chr_tile(&m_bank[(m_tile_idx + 3) * 16], mpal, false, false, true);

            draw_chr_tile(mx, my, t0);
            draw_chr_tile(mx + 8, my, t1);
            draw_chr_tile(mx, my + 8, t2);
            draw_chr_tile(mx + 8, my + 8, t3);
        } else {
            draw_rect(mx, my, 16, 16, lut_NESPalette[mpal[1] % 64]);
        }

        // Monster abbreviation tag
        Font::draw_string(buffer_.data(), width_, mx / 8, (my + 18) / 8, monsters[m].name.substr(0, 4));

        // Target Cursor indicator
        if (battle.get_state() == BattleState::TARGET_SELECT && battle.get_target_cursor() == m) {
            Font::draw_string(buffer_.data(), width_, (mx - 10) / 8, (my + 4) / 8, ">");
        }
    }

    // 3. Draw 4 Party Heroes on Right Flank with authentic poses & step-forward
    const auto& bank_02 = loader.get_chr_bank_02();
    for (size_t i = 0; i < 4; ++i) {
        const auto& hero = save_data.party[i];
        HeroPose pose = battle.get_hero_pose(i, save_data);

        int hx = 184;
        if (pose == HeroPose::STEP_FORWARD || pose == HeroPose::ATTACK_SWING) {
            hx = 168; // Step forward 16 pixels left
        }
        int hy = 24 + static_cast<int>(i) * 26;

        std::array<uint8_t, 4> hpal = loader.get_player_palette(hero.char_class);

        int tbl_idx = 2; // Left-facing standing
        if (pose == HeroPose::CROUCH) {
            tbl_idx = 7; // Down/fallen crouch
        } else if (pose == HeroPose::ATTACK_SWING) {
            tbl_idx = 3; // Attack swing frame
        }
        const uint8_t* spr_tbl = lut_PlayerMapmanSprTbl[tbl_idx];

        size_t hero_class_offset = 0x1000 + (static_cast<size_t>(hero.char_class) % 12) * 0x100;
        size_t o_ul = hero_class_offset + (static_cast<size_t>(spr_tbl[0]) * 16);
        size_t o_dl = hero_class_offset + (static_cast<size_t>(spr_tbl[2]) * 16);
        size_t o_ur = hero_class_offset + (static_cast<size_t>(spr_tbl[4]) * 16);
        size_t o_dr = hero_class_offset + (static_cast<size_t>(spr_tbl[6]) * 16);

        bool ul_fx = (spr_tbl[1] & 0x40) != 0; bool ul_fy = (spr_tbl[1] & 0x80) != 0;
        bool dl_fx = (spr_tbl[3] & 0x40) != 0; bool dl_fy = (spr_tbl[3] & 0x80) != 0;
        bool ur_fx = (spr_tbl[5] & 0x40) != 0; bool ur_fy = (spr_tbl[5] & 0x80) != 0;
        bool dr_fx = (spr_tbl[7] & 0x40) != 0; bool dr_fy = (spr_tbl[7] & 0x80) != 0;

        if (o_ul + 16 <= bank_02.size() && o_dl + 16 <= bank_02.size() &&
            o_ur + 16 <= bank_02.size() && o_dr + 16 <= bank_02.size()) {

            PixelBuffer8x8 subtile_ul = CHRDecoder::decode_chr_tile(&bank_02[o_ul], hpal, ul_fx, ul_fy, true);
            PixelBuffer8x8 subtile_dl = CHRDecoder::decode_chr_tile(&bank_02[o_dl], hpal, dl_fx, dl_fy, true);
            PixelBuffer8x8 subtile_ur = CHRDecoder::decode_chr_tile(&bank_02[o_ur], hpal, ur_fx, ur_fy, true);
            PixelBuffer8x8 subtile_dr = CHRDecoder::decode_chr_tile(&bank_02[o_dr], hpal, dr_fx, dr_fy, true);

            draw_chr_tile(hx, hy, subtile_ul);
            draw_chr_tile(hx, hy + 8, subtile_dl);
            draw_chr_tile(hx + 8, hy, subtile_ur);
            draw_chr_tile(hx + 8, hy + 8, subtile_dr);
        } else {
            draw_rect(hx, hy, 16, 16, lut_NESPalette[hpal[1] % 64]);
        }
    }

    // 4. Lower Combat UI
    BattleState bstate = battle.get_state();

    if (bstate == BattleState::SUBMENU_MAGIC) {
        // Magic Selection Sub-Menu
        WindowBox::draw_box(buffer_.data(), width_, 0, 16, 15, 14);
        uint8_t a_hero = battle.get_active_hero();
        const auto& hero = save_data.party[a_hero];
        uint8_t t_cur = battle.get_magic_tier_cursor();
        uint8_t s_cur = battle.get_magic_slot_cursor();

        std::string tier_header = "T" + std::to_string(t_cur + 1) + " MP:" +
                                  std::to_string(hero.stats.mp[t_cur]) + "/" + std::to_string(hero.stats.max_mp[t_cur]);
        Font::draw_string(buffer_.data(), width_, 2, 17, tier_header);

        for (int s = 0; s < 3; ++s) {
            uint8_t sp_id = hero.spells[t_cur][s];
            if (sp_id != 0xFF) {
                const auto& sp = loader.get_spell(sp_id);
                Font::draw_string(buffer_.data(), width_, 3, 19 + s * 2, sp.name);
            } else {
                Font::draw_string(buffer_.data(), width_, 3, 19 + s * 2, "------");
            }
        }
        Font::draw_string(buffer_.data(), width_, 1, 19 + s_cur * 2, ">");
    } else if (bstate == BattleState::SUBMENU_ITEM) {
        // Item Selection Sub-Menu
        WindowBox::draw_box(buffer_.data(), width_, 0, 16, 15, 14);
        Font::draw_string(buffer_.data(), width_, 2, 17, "ITEMS");
        Font::draw_string(buffer_.data(), width_, 3, 19, "HEAL Potion (" + std::to_string(save_data.consumables.heal_potions) + ")");
        Font::draw_string(buffer_.data(), width_, 3, 21, "PURE Potion (" + std::to_string(save_data.consumables.pure_potions) + ")");
        Font::draw_string(buffer_.data(), width_, 3, 23, "TENT (" + std::to_string(save_data.consumables.tents) + ")");
        Font::draw_string(buffer_.data(), width_, 1, 19 + battle.get_item_cursor() * 2, ">");
    } else {
        // Standard Command Box (Bottom Left)
        WindowBox::draw_box(buffer_.data(), width_, 0, 16, 15, 14);
        Font::draw_string(buffer_.data(), width_, 2, 18, "FIGHT");
        Font::draw_string(buffer_.data(), width_, 9, 18, "MAGIC");
        Font::draw_string(buffer_.data(), width_, 2, 21, "DRINK");
        Font::draw_string(buffer_.data(), width_, 9, 21, "ITEM");
        Font::draw_string(buffer_.data(), width_, 2, 24, "RUN");

        if (bstate == BattleState::HERO_COMMAND_SELECT) {
            uint8_t c_cur = battle.get_command_cursor();
            if (c_cur == 0) Font::draw_string(buffer_.data(), width_, 1, 18, ">");
            else if (c_cur == 1) Font::draw_string(buffer_.data(), width_, 8, 18, ">");
            else if (c_cur == 2) Font::draw_string(buffer_.data(), width_, 1, 21, ">");
            else if (c_cur == 3) Font::draw_string(buffer_.data(), width_, 8, 21, ">");
            else if (c_cur == 4) Font::draw_string(buffer_.data(), width_, 1, 24, ">");
        }
    }

    // Party HP/MP Status (Bottom Right)
    WindowBox::draw_box(buffer_.data(), width_, 15, 16, 17, 14);
    for (size_t i = 0; i < 4; ++i) {
        const auto& hero = save_data.party[i];
        std::string name_str = hero.name.substr(0, 4);
        std::string hp_str = std::to_string(hero.stats.hp);
        while (hp_str.length() < 3) hp_str = " " + hp_str;

        int row_y = 18 + static_cast<int>(i) * 3;
        Font::draw_string(buffer_.data(), width_, 17, row_y, name_str);
        Font::draw_string(buffer_.data(), width_, 24, row_y, "H" + hp_str);
    }

    // Battle Narrative Banner (Top)
    WindowBox::draw_box(buffer_.data(), width_, 0, 0, 32, 4);
    std::string msg = battle.get_current_narrative();
    if (msg.empty()) {
        const auto& log = battle.get_log();
        msg = log.empty() ? "BATTLE IN PROGRESS" : log.back();
    }
    Font::draw_string(buffer_.data(), width_, 1, 1, msg.substr(0, 30));

    // Victory Summary Popup Box
    if (bstate == BattleState::VICTORY_SUMMARY) {
        WindowBox::draw_box(buffer_.data(), width_, 6, 8, 20, 10);
        Font::draw_string(buffer_.data(), width_, 11, 9, "VICTORY!");
        Font::draw_string(buffer_.data(), width_, 8, 11, "EXP UP: " + std::to_string(battle.get_reward_exp()) + " P");
        Font::draw_string(buffer_.data(), width_, 8, 13, "GOLD:   " + std::to_string(battle.get_reward_gp()) + " G");
        Font::draw_string(buffer_.data(), width_, 8, 15, "[SPACE] Continue");
    }

    // Level-Up Popup Box
    if (bstate == BattleState::LEVEL_UP) {
        const auto& lvl = battle.get_current_level_up();
        WindowBox::draw_box(buffer_.data(), width_, 5, 6, 22, 14);
        Font::draw_string(buffer_.data(), width_, 10, 7, "LEVEL UP!");
        for (size_t m = 0; m < lvl.messages.size() && m < 5; ++m) {
            Font::draw_string(buffer_.data(), width_, 7, 9 + static_cast<int>(m) * 2, lvl.messages[m]);
        }
        Font::draw_string(buffer_.data(), width_, 7, 18, "[SPACE] Continue");
    }
}

void Renderer::draw_cutscene(const CutsceneEngine& cutscene, const DataLoader& loader) {
    CutsceneType type = cutscene.get_active_type();

    // 1. Upper Scene Visuals (Y=0..160)
    if (type == CutsceneType::OPENING_BRIDGE) {
        // Sky / Horizon gradient
        for (int y = 0; y < 160; ++y) {
            uint32_t sky = (y < 90) ? 0xFF081830 : 0xFF183850;
            for (int x = 0; x < width_; ++x) {
                buffer_[y * width_ + x] = sky;
            }
        }
        // Castle Conelia Silhouette in distance
        draw_rect(100, 50, 56, 40, 0xFF102030);
        draw_rect(112, 35, 32, 20, 0xFF102030);
        draw_rect(122, 20, 12, 20, 0xFF102030);

        // Stone Bridge across the water
        draw_rect(0, 120, width_, 40, 0xFF505050);
        draw_rect(0, 116, width_, 4, 0xFF707070);

        // 4 Light Warriors standing on the bridge looking towards the castle (facing UP)
        const auto& bank_02 = loader.get_chr_bank_02();
        const uint8_t* spr_tbl = lut_PlayerMapmanSprTbl[4]; // Facing UP

        for (int h = 0; h < 4; ++h) {
            int hx = 76 + h * 28;
            int hy = 104;
            std::array<uint8_t, 4> hpal = loader.get_player_palette(static_cast<ClassType>(h));
            size_t hero_offset = 0x1000 + (h * 0x100);

            size_t o_ul = hero_offset + (static_cast<size_t>(spr_tbl[0]) * 16);
            size_t o_dl = hero_offset + (static_cast<size_t>(spr_tbl[2]) * 16);
            size_t o_ur = hero_offset + (static_cast<size_t>(spr_tbl[4]) * 16);
            size_t o_dr = hero_offset + (static_cast<size_t>(spr_tbl[6]) * 16);

            if (o_ul + 16 <= bank_02.size() && o_dl + 16 <= bank_02.size() &&
                o_ur + 16 <= bank_02.size() && o_dr + 16 <= bank_02.size()) {
                PixelBuffer8x8 subtile_ul = CHRDecoder::decode_chr_tile(&bank_02[o_ul], hpal, false, false, true);
                PixelBuffer8x8 subtile_dl = CHRDecoder::decode_chr_tile(&bank_02[o_dl], hpal, false, false, true);
                PixelBuffer8x8 subtile_ur = CHRDecoder::decode_chr_tile(&bank_02[o_ur], hpal, false, false, true);
                PixelBuffer8x8 subtile_dr = CHRDecoder::decode_chr_tile(&bank_02[o_dr], hpal, false, false, true);

                draw_chr_tile(hx, hy, subtile_ul);
                draw_chr_tile(hx, hy + 8, subtile_dl);
                draw_chr_tile(hx + 8, hy, subtile_ur);
                draw_chr_tile(hx + 8, hy + 8, subtile_dr);
            }
        }
    } else if (type == CutsceneType::ENDING_CREDITS) {
        // Grand Epilogue Cliff Horizon
        for (int y = 0; y < 160; ++y) {
            uint32_t sky = (y < 60) ? 0xFF001030 : ((y < 110) ? 0xFF102850 : 0xFF204870);
            for (int x = 0; x < width_; ++x) {
                buffer_[y * width_ + x] = sky;
            }
        }
        // Golden sunrise glow
        draw_rect(100, 70, 56, 30, 0xFF706030);

        // Grassy Cliff on left
        for (int cy = 110; cy < 160; ++cy) {
            int cx_width = 160 - (cy - 110) * 2;
            draw_rect(0, cy, std::max(0, cx_width), 1, 0xFF185018);
        }

        // 4 Light Warriors standing in profile on the cliff
        const auto& bank_02 = loader.get_chr_bank_02();
        const uint8_t* spr_tbl = lut_PlayerMapmanSprTbl[0]; // Facing RIGHT in profile

        for (int h = 0; h < 4; ++h) {
            int hx = 24 + h * 20;
            int hy = 96;
            std::array<uint8_t, 4> hpal = loader.get_player_palette(static_cast<ClassType>(h));
            size_t hero_offset = 0x1000 + (h * 0x100);

            size_t o_ul = hero_offset + (static_cast<size_t>(spr_tbl[0]) * 16);
            size_t o_dl = hero_offset + (static_cast<size_t>(spr_tbl[2]) * 16);
            size_t o_ur = hero_offset + (static_cast<size_t>(spr_tbl[4]) * 16);
            size_t o_dr = hero_offset + (static_cast<size_t>(spr_tbl[6]) * 16);

            if (o_ul + 16 <= bank_02.size() && o_dl + 16 <= bank_02.size() &&
                o_ur + 16 <= bank_02.size() && o_dr + 16 <= bank_02.size()) {
                PixelBuffer8x8 subtile_ul = CHRDecoder::decode_chr_tile(&bank_02[o_ul], hpal, false, false, true);
                PixelBuffer8x8 subtile_dl = CHRDecoder::decode_chr_tile(&bank_02[o_dl], hpal, false, false, true);
                PixelBuffer8x8 subtile_ur = CHRDecoder::decode_chr_tile(&bank_02[o_ur], hpal, false, false, true);
                PixelBuffer8x8 subtile_dr = CHRDecoder::decode_chr_tile(&bank_02[o_dr], hpal, false, false, true);

                draw_chr_tile(hx, hy, subtile_ul);
                draw_chr_tile(hx, hy + 8, subtile_dl);
                draw_chr_tile(hx + 8, hy, subtile_ur);
                draw_chr_tile(hx + 8, hy + 8, subtile_dr);
            }
        }

        std::string sub = cutscene.get_current_subtitle();
        if (sub == "THE END") {
            WindowBox::draw_box(buffer_.data(), width_, 10, 6, 12, 5);
            Font::draw_string(buffer_.data(), width_, 13, 8, "THE END");
        }
    } else {
        // Generic starry scene
        for (int y = 0; y < 160; ++y) {
            for (int x = 0; x < width_; ++x) {
                buffer_[y * width_ + x] = 0xFF000000;
            }
        }
    }

    // 2. Lower Narrative Subtitle Window (Y=160..240)
    WindowBox::draw_box(buffer_.data(), width_, 1, 20, 30, 9);
    Font::draw_string(buffer_.data(), width_, 2, 22, cutscene.get_current_subtitle());
    Font::draw_string(buffer_.data(), width_, 2, 26, "Press SPACE to continue...");
}

void Renderer::draw_puzzle(const MiniGameEngine& minigame, const DataLoader& loader) {
    clear(0xFF000000);

    // Frame the 15-Puzzle board in center
    WindowBox::draw_box(buffer_.data(), width_, 4, 2, 24, 26);
    Font::draw_string(buffer_.data(), width_, 6, 4, "15-PUZZLE EASTER EGG");

    std::string move_str = "MOVES: " + std::to_string(minigame.get_move_count());
    Font::draw_string(buffer_.data(), width_, 6, 6, move_str);

    const auto& board = minigame.get_board();
    const auto& puzzle_chr = loader.get_puzzle_chr();

    // 4x4 Grid
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            int slot = r * 4 + c;
            uint8_t tile_num = board[slot];
            int px = 56 + c * 36;
            int py = 72 + r * 32;

            if (tile_num == 0) {
                // Empty slot
                draw_rect(px, py, 32, 28, 0xFF181818);
            } else {
                // Tile block
                draw_rect(px, py, 32, 28, 0xFF284878);
                draw_rect(px + 1, py + 1, 30, 26, 0xFF3868A8);

                // Decode 1bpp number tile if puzzle_chr is available
                size_t num_offset = static_cast<size_t>(tile_num) * 8;
                if (num_offset + 8 <= puzzle_chr.size()) {
                    PixelBuffer8x8 num_tile = CHRDecoder::decode_1bpp_tile(
                        &puzzle_chr[num_offset], 0xFFFFFFFF, 0x00000000
                    );
                    draw_chr_tile(px + 12, py + 10, num_tile);
                } else {
                    std::string tstr = std::to_string(tile_num);
                    Font::draw_string(buffer_.data(), width_, (px + 12) / 8, (py + 10) / 8, tstr);
                }
            }
        }
    }

    Font::draw_string(buffer_.data(), width_, 6, 24, "SPACE to Slide, [P] Exit");
}

void Renderer::draw_intro_story(const IntroEngine& intro) {
    (void)intro;
    clear(0xFF000000);

    // Opening Prologue Story Text (from NES 0D_BF20_introtext.bin)
    static const char* story_lines[] = {
        "The world is veiled in",
        "darkness. The wind stops,",
        "the sea is wild, and the",
        "earth begins to rot.",
        "The people wait, their",
        "only hope, a prophecy....",
        "",
        "\"When the world is in",
        " darkness, Four Warriors",
        " will come....\"",
        "",
        "After a long journey,",
        "four young warriors arrive,",
        "each holding an ORB."
    };

    int start_y = 4;
    for (int i = 0; i < 14; ++i) {
        if (story_lines[i][0] != '\0') {
            Font::draw_string(buffer_.data(), width_, 3, start_y + i, story_lines[i]);
        }
    }

    // Flashing prompt at bottom
    if ((frame_counter_ / 30) % 2 == 0) {
        Font::draw_string(buffer_.data(), width_, 6, 22, "PRESS ENTER / SPACE");
    }
}

void Renderer::draw_title_screen(const IntroEngine& intro, bool has_save_file) {
    clear(0xFF000000);

    // Box 1: CONTINUE (X=11, Y=8, W=10, H=4 in 8x8 tile coords)
    WindowBox::draw_box(buffer_.data(), width_, 11, 8, 10, 4);
    Font::draw_string(buffer_.data(), width_, 12, 9, "CONTINUE");

    // Box 2: NEW GAME (X=11, Y=13, W=10, H=4 in 8x8 tile coords)
    WindowBox::draw_box(buffer_.data(), width_, 11, 13, 10, 4);
    Font::draw_string(buffer_.data(), width_, 12, 14, "NEW GAME");

    // Box 3: RESPOND RATE (X=8, Y=18, W=16, H=4 in 8x8 tile coords)
    WindowBox::draw_box(buffer_.data(), width_, 8, 18, 16, 4);
    std::string rate_str = "RESPOND RATE " + std::to_string(intro.get_respond_rate());
    Font::draw_string(buffer_.data(), width_, 9, 19, rate_str);

    // Cursor indicator
    uint8_t cursor = intro.get_title_cursor();
    if (cursor == 0) {
        Font::draw_string(buffer_.data(), width_, 9, 9, ">");
    } else {
        Font::draw_string(buffer_.data(), width_, 9, 14, ">");
    }

    // Bottom Copyright text
    Font::draw_string(buffer_.data(), width_, 8, 25, "(C) 1987 SQUARE");
    Font::draw_string(buffer_.data(), width_, 7, 27, "(C) 1990 NINTENDO");
}

void Renderer::draw_party_creation(const IntroEngine& intro, const DataLoader& loader) {
    clear(0xFF000000);

    const auto& bank_02 = loader.get_chr_bank_02();
    uint8_t active_slot = intro.get_active_slot();
    bool anim_frame = (frame_counter_ / 16) % 2 != 0;

    // 4 Boxes in 2x2 Grid matching disassembly lut_PtyGenBuf
    static const int box_coords[4][2] = {
        {4, 3}, {17, 3},
        {4, 14}, {17, 14}
    };

    for (int i = 0; i < 4; ++i) {
        int bx = box_coords[i][0];
        int by = box_coords[i][1];
        const auto& slot = intro.get_slot(i);

        // Draw blue box with double-line border
        WindowBox::draw_box(buffer_.data(), width_, bx, by, 11, 10);

        // 1. Class Name centered inside top of box
        std::string cname = IntroEngine::get_class_name(slot.class_id);
        int name_x = bx + 1 + std::max(0, (9 - (int)cname.length()) / 2);
        Font::draw_string(buffer_.data(), width_, name_x, by + 1, cname);

        // 2. Animated Class Sprite in center (16x24)
        int spr_px = bx * 8 + 36;
        int spr_py = by * 8 + 24;
        auto pal = loader.get_player_palette(IntroEngine::get_class_type(slot.class_id), 0);

        // Hero sprite CHR in bank_02 (offset 0x1000 + class_id * 0x100 + anim_frame * 0x40)
        size_t hero_base = 0x1000 + (slot.class_id * 0x100) + (anim_frame ? 0x40 : 0x00);
        if (hero_base + 64 <= bank_02.size()) {
            PixelBuffer8x8 t_ul = CHRDecoder::decode_chr_tile(&bank_02[hero_base + 0], pal, false, false, true);
            PixelBuffer8x8 t_ur = CHRDecoder::decode_chr_tile(&bank_02[hero_base + 16], pal, false, false, true);
            PixelBuffer8x8 t_dl = CHRDecoder::decode_chr_tile(&bank_02[hero_base + 32], pal, false, false, true);
            PixelBuffer8x8 t_dr = CHRDecoder::decode_chr_tile(&bank_02[hero_base + 48], pal, false, false, true);

            draw_chr_tile(spr_px, spr_py, t_ul);
            draw_chr_tile(spr_px + 8, spr_py, t_ur);
            draw_chr_tile(spr_px, spr_py + 8, t_dl);
            draw_chr_tile(spr_px + 8, spr_py + 8, t_dr);
        } else {
            draw_rect(spr_px, spr_py, 16, 16, 0xFF4080FF);
        }

        // 3. Name or placeholder underline below sprite
        std::string dname = slot.name;
        while (dname.length() < 4) dname.push_back('_');
        Font::draw_string(buffer_.data(), width_, bx + 3, by + 7, dname);

        // 4. Cursor on active slot
        if (i == active_slot && intro.get_state() == IntroState::PARTY_CREATION_CLASS) {
            Font::draw_string(buffer_.data(), width_, bx + 1, by + 4, ">");
        }
    }

    // If currently typing name, draw the Name Input Keyboard overlay
    if (intro.get_state() == IntroState::PARTY_CREATION_NAME) {
        draw_name_input_screen(intro);
    }
}

void Renderer::draw_name_input_screen(const IntroEngine& intro) {
    // 1. Top Small Name Window (X=13, Y=2, W=6, H=4 in tile coords)
    WindowBox::draw_box(buffer_.data(), width_, 13, 2, 6, 4);
    uint8_t slot_idx = intro.get_active_slot();
    const auto& slot = intro.get_slot(slot_idx);
    std::string typed_name = slot.name;
    while (typed_name.length() < 4) typed_name.push_back('_');
    Font::draw_string(buffer_.data(), width_, 14, 3, typed_name);

    // 2. Big Keyboard Window (X=4, Y=7, W=24, H=19 in tile coords)
    WindowBox::draw_box(buffer_.data(), width_, 4, 7, 24, 19);

    uint8_t cur_x = intro.get_kb_cursor_x();
    uint8_t cur_y = intro.get_kb_cursor_y();

    // 7 Rows x 10 Columns
    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 10; ++c) {
            char ch = intro.get_kb_char_at(c, r);
            int tx = 6 + c * 2;
            int ty = 9 + r * 2;
            std::string s(1, ch);
            Font::draw_string(buffer_.data(), width_, tx, ty, s);

            // Highlight cursor
            if (c == cur_x && r == cur_y) {
                Font::draw_string(buffer_.data(), width_, tx - 1, ty, ">");
            }
        }
    }

    // Bottom caption: "SELECT  NAME"
    Font::draw_string(buffer_.data(), width_, 10, 24, "SELECT  NAME");
}

void Renderer::draw_main_menu(const MenuEngine& menu, const GameSaveData& save_data, const DataLoader& loader) {
    (void)loader;
    clear(0xFF0A1428); // Classic dark blue backdrop

    // 1. Orbs Box (X=1, Y=1, W=10, H=6)
    WindowBox::draw_box(buffer_.data(), width_, 1, 1, 10, 6);
    Font::draw_string(buffer_.data(), width_, 2, 2, "ORBS");
    
    uint32_t c_earth = save_data.orbs_lit[0] ? 0xFFE0C020 : 0xFF606060;
    uint32_t c_fire  = save_data.orbs_lit[1] ? 0xFFE04040 : 0xFF606060;
    uint32_t c_water = save_data.orbs_lit[2] ? 0xFF4080E0 : 0xFF606060;
    uint32_t c_wind  = save_data.orbs_lit[3] ? 0xFF60E0E0 : 0xFF606060;

    Font::draw_string(buffer_.data(), width_, 2, 3, "EARTH", c_earth);
    Font::draw_string(buffer_.data(), width_, 2, 4, "FIRE", c_fire);
    Font::draw_string(buffer_.data(), width_, 2, 5, "WATER", c_water);
    Font::draw_string(buffer_.data(), width_, 2, 6, "WIND", c_wind);

    // 2. Gold Box (X=1, Y=7, W=10, H=4)
    WindowBox::draw_box(buffer_.data(), width_, 1, 7, 10, 4);
    Font::draw_string(buffer_.data(), width_, 2, 8, "GOLD");
    Font::draw_string(buffer_.data(), width_, 2, 9, std::to_string(save_data.gold) + " G");

    // 3. Command Menu Box (X=1, Y=11, W=10, H=12)
    WindowBox::draw_box(buffer_.data(), width_, 1, 11, 10, 12);
    const char* options[5] = {"ITEM", "MAGIC", "WEAPON", "ARMOR", "STATUS"};
    for (int i = 0; i < 5; ++i) {
        Font::draw_string(buffer_.data(), width_, 3, 13 + i * 2, options[i]);
    }
    Font::draw_string(buffer_.data(), width_, 2, 13 + menu.get_main_cursor() * 2, ">");

    // 4. Party Cards (X=12, Y=1, W=19, H=28)
    WindowBox::draw_box(buffer_.data(), width_, 12, 1, 19, 28);
    for (int i = 0; i < 4; ++i) {
        const auto& hero = save_data.party[i];
        int by = 2 + i * 6;

        if (menu.get_state() == MenuState::LINEUP_SELECT && menu.get_char_cursor() == i) {
            Font::draw_string(buffer_.data(), width_, 13, by, ">");
        }

        // Line 1: Name and Class
        std::string class_str = IntroEngine::get_class_name(static_cast<uint8_t>(hero.char_class));
        Font::draw_string(buffer_.data(), width_, 14, by, hero.name + " " + class_str);

        // Line 2: Level & HP
        std::string hp_str = "L" + std::to_string(hero.level) + " HP " + std::to_string(hero.stats.hp) + "/" + std::to_string(hero.stats.max_hp);
        Font::draw_string(buffer_.data(), width_, 14, by + 1, hp_str);

        // Line 3: Magic MP Matrix (Tier 1-4)
        std::string mp_str1 = "MP:" + std::to_string(hero.stats.mp[0]) + "/" + std::to_string(hero.stats.mp[1]) + "/" + std::to_string(hero.stats.mp[2]) + "/" + std::to_string(hero.stats.mp[3]);
        Font::draw_string(buffer_.data(), width_, 14, by + 2, mp_str1);

        // Line 4: Magic MP Matrix (Tier 5-8)
        std::string mp_str2 = "   " + std::to_string(hero.stats.mp[4]) + "/" + std::to_string(hero.stats.mp[5]) + "/" + std::to_string(hero.stats.mp[6]) + "/" + std::to_string(hero.stats.mp[7]);
        Font::draw_string(buffer_.data(), width_, 14, by + 3, mp_str2);

        // Status Ailment tag
        if (hero.stats.hp == 0 || (hero.status_ailments & Status::DEATH)) {
            Font::draw_string(buffer_.data(), width_, 24, by + 1, "DEAD", 0xFFE04040);
        } else if (hero.status_ailments & Status::STONE) {
            Font::draw_string(buffer_.data(), width_, 24, by + 1, "STON", 0xFFB0B0B0);
        } else if (hero.status_ailments & Status::POISON) {
            Font::draw_string(buffer_.data(), width_, 24, by + 1, "POIS", 0xFF60E060);
        }
    }

    // Bottom Help Banner (X=1, Y=24, W=10, H=5)
    WindowBox::draw_box(buffer_.data(), width_, 1, 24, 10, 5);
    Font::draw_string(buffer_.data(), width_, 2, 25, "SELECT:");
    Font::draw_string(buffer_.data(), width_, 2, 26, "LINEUP");
    Font::draw_string(buffer_.data(), width_, 2, 27, "M: EXIT");
}

void Renderer::draw_item_menu(const MenuEngine& menu, const GameSaveData& save_data, const DataLoader& loader) {
    (void)loader;
    clear(0xFF0A1428);

    // Left Items Box (X=1, Y=1, W=19, H=22)
    WindowBox::draw_box(buffer_.data(), width_, 1, 1, 19, 22);
    Font::draw_string(buffer_.data(), width_, 2, 2, "ITEMS");

    std::vector<std::string> item_names = {
        "HEAL  *" + std::to_string(save_data.consumables.heal_potions),
        "PURE  *" + std::to_string(save_data.consumables.pure_potions),
        "SOFT  *" + std::to_string(save_data.consumables.soft_potions),
        "TENT  *" + std::to_string(save_data.consumables.tents),
        "CABIN *" + std::to_string(save_data.consumables.cabins),
        "HOUSE *" + std::to_string(save_data.consumables.houses)
    };

    for (size_t k = 0; k < static_cast<size_t>(KeyItem::COUNT); ++k) {
        if (save_data.key_items_and_flags[k] > 0) {
            KeyItemInfo info = get_key_item_info(static_cast<KeyItem>(k));
            item_names.push_back(info.name);
        }
    }

    int cur = menu.get_item_cursor();
    for (size_t i = 0; i < item_names.size() && i < 8; ++i) {
        Font::draw_string(buffer_.data(), width_, 4, 4 + i * 2, item_names[i]);
    }
    if (cur < 8) {
        Font::draw_string(buffer_.data(), width_, 2, 4 + cur * 2, ">");
    }

    // Right Party Target Box (X=21, Y=1, W=10, H=22)
    WindowBox::draw_box(buffer_.data(), width_, 21, 1, 10, 22);
    Font::draw_string(buffer_.data(), width_, 22, 2, "PARTY");
    for (int i = 0; i < 4; ++i) {
        const auto& hero = save_data.party[i];
        int by = 4 + i * 4;
        if (menu.get_state() == MenuState::ITEM_TARGET_SELECT && menu.get_target_char_cursor() == i) {
            Font::draw_string(buffer_.data(), width_, 22, by, ">");
        }
        Font::draw_string(buffer_.data(), width_, 23, by, hero.name);
        Font::draw_string(buffer_.data(), width_, 23, by + 1, std::to_string(hero.stats.hp) + "/" + std::to_string(hero.stats.max_hp));
    }

    // Bottom Message Box (X=1, Y=24, W=30, H=5)
    WindowBox::draw_box(buffer_.data(), width_, 1, 24, 30, 5);
    if (menu.get_state() == MenuState::CAMPING_SAVE_PROMPT) {
        Font::draw_string(buffer_.data(), width_, 2, 25, "Rest & Save on Overworld?");
        Font::draw_string(buffer_.data(), width_, 4, 27, "YES");
        Font::draw_string(buffer_.data(), width_, 14, 27, "NO");
        Font::draw_string(buffer_.data(), width_, (menu.get_camping_confirm_cursor() == 0 ? 3 : 13), 27, ">");
    } else {
        Font::draw_string(buffer_.data(), width_, 2, 25, "CONFIRM: Use/Select  CANCEL: Back");
    }
}

void Renderer::draw_equip_menu(const MenuEngine& menu, const GameSaveData& save_data, const DataLoader& loader) {
    clear(0xFF0A1428);

    uint8_t c_idx = menu.get_char_cursor();
    const auto& hero = save_data.party[c_idx];

    // Top Header Window (X=1, Y=1, W=30, H=4)
    WindowBox::draw_box(buffer_.data(), width_, 1, 1, 30, 4);
    std::string title = hero.name + " " + IntroEngine::get_class_name(static_cast<uint8_t>(hero.char_class));
    Font::draw_string(buffer_.data(), width_, 2, 2, title);

    const char* tabs[3] = {"[EQUIP]", "[TRADE]", "[DROP]"};
    int tab_idx = static_cast<int>(menu.get_equip_tab());
    Font::draw_string(buffer_.data(), width_, 16, 2, tabs[0], tab_idx == 0 ? 0xFFE0C020 : 0xFFFFFFFF);
    Font::draw_string(buffer_.data(), width_, 21, 2, tabs[1], tab_idx == 1 ? 0xFFE0C020 : 0xFFFFFFFF);
    Font::draw_string(buffer_.data(), width_, 26, 2, tabs[2], tab_idx == 2 ? 0xFFE0C020 : 0xFFFFFFFF);

    // Left Slots Window (X=1, Y=5, W=18, H=18)
    WindowBox::draw_box(buffer_.data(), width_, 1, 5, 18, 18);
    Font::draw_string(buffer_.data(), width_, 2, 6, "WEAPONS");
    for (int i = 0; i < 4; ++i) {
        uint8_t wid = hero.weapons[i];
        std::string wname = (wid == 0xFF) ? "---" : ("E-" + loader.get_weapon(wid).name);
        Font::draw_string(buffer_.data(), width_, 4, 7 + i, wname);
    }

    Font::draw_string(buffer_.data(), width_, 2, 12, "ARMORS");
    for (int i = 0; i < 4; ++i) {
        uint8_t aid = hero.armors[i];
        std::string aname = (aid == 0xFF) ? "---" : ("E-" + loader.get_armor(aid).name);
        Font::draw_string(buffer_.data(), width_, 4, 13 + i, aname);
    }

    // Draw slot cursor
    uint8_t sc = menu.get_equip_slot_cursor();
    if (sc < 4) {
        Font::draw_string(buffer_.data(), width_, 2, 7 + sc, ">");
    } else {
        Font::draw_string(buffer_.data(), width_, 2, 13 + (sc - 4), ">");
    }

    // Right Stats Window (X=20, Y=5, W=11, H=18)
    WindowBox::draw_box(buffer_.data(), width_, 20, 5, 11, 18);
    Font::draw_string(buffer_.data(), width_, 21, 6, "STATS");
    Font::draw_string(buffer_.data(), width_, 21, 8, "DMG  " + std::to_string(hero.stats.damage));
    Font::draw_string(buffer_.data(), width_, 21, 10, "HIT% " + std::to_string(hero.stats.hit_rate));
    Font::draw_string(buffer_.data(), width_, 21, 12, "ABS  " + std::to_string(hero.stats.absorb));
    Font::draw_string(buffer_.data(), width_, 21, 14, "EVA% " + std::to_string(hero.stats.evade));

    // Bottom Controls Info (X=1, Y=24, W=30, H=5)
    WindowBox::draw_box(buffer_.data(), width_, 1, 24, 30, 5);
    Font::draw_string(buffer_.data(), width_, 2, 25, "SELECT: Tab  CONFIRM: Action");
    Font::draw_string(buffer_.data(), width_, 2, 26, "LEFT/RIGHT: Hero  CANCEL: Back");
}

void Renderer::draw_magic_menu(const MenuEngine& menu, const GameSaveData& save_data, const DataLoader& loader) {
    (void)loader;
    clear(0xFF0A1428);

    uint8_t c_idx = menu.get_char_cursor();
    const auto& hero = save_data.party[c_idx];

    // Top Header Window (X=1, Y=1, W=30, H=4)
    WindowBox::draw_box(buffer_.data(), width_, 1, 1, 30, 4);
    std::string title = hero.name + " " + IntroEngine::get_class_name(static_cast<uint8_t>(hero.char_class)) + " MAGIC";
    Font::draw_string(buffer_.data(), width_, 2, 2, title);

    // 8-Tier Magic Grid Box (X=1, Y=5, W=21, H=18)
    WindowBox::draw_box(buffer_.data(), width_, 1, 5, 21, 18);

    for (int t = 0; t < 8; ++t) {
        int ty = 6 + t * 2;
        std::string tier_hdr = "L" + std::to_string(t + 1) + "[" + std::to_string(hero.stats.mp[t]) + "]:";
        Font::draw_string(buffer_.data(), width_, 2, ty, tier_hdr);

        for (int s = 0; s < 3; ++s) {
            uint8_t sp_id = hero.spells[t][s];
            std::string sp_name = (sp_id == 0xFF) ? "---" : loader.get_spell(sp_id).name;
            Font::draw_string(buffer_.data(), width_, 8 + s * 4, ty, sp_name);
        }
    }

    // Cursor indicator
    uint8_t cur = menu.get_magic_cursor();
    int cur_tier = cur / 3;
    int cur_slot = cur % 3;
    Font::draw_string(buffer_.data(), width_, 7 + cur_slot * 4, 6 + cur_tier * 2, ">");

    // Right Target Box (X=23, Y=5, W=8, H=18)
    WindowBox::draw_box(buffer_.data(), width_, 23, 5, 8, 18);
    Font::draw_string(buffer_.data(), width_, 24, 6, "PARTY");
    for (int i = 0; i < 4; ++i) {
        const auto& h = save_data.party[i];
        int by = 8 + i * 3;
        if (menu.get_state() == MenuState::MAGIC_TARGET_SELECT && menu.get_target_char_cursor() == i) {
            Font::draw_string(buffer_.data(), width_, 23, by, ">");
        }
        Font::draw_string(buffer_.data(), width_, 24, by, h.name);
        Font::draw_string(buffer_.data(), width_, 24, by + 1, std::to_string(h.stats.hp));
    }

    // Bottom Help Banner (X=1, Y=24, W=30, H=5)
    WindowBox::draw_box(buffer_.data(), width_, 1, 24, 30, 5);
    Font::draw_string(buffer_.data(), width_, 2, 25, "CONFIRM: Cast Field Spell");
    Font::draw_string(buffer_.data(), width_, 2, 26, "LEFT/RIGHT: Hero  CANCEL: Back");
}

void Renderer::draw_status_menu(const MenuEngine& menu, const GameSaveData& save_data, const DataLoader& loader) {
    (void)loader;
    clear(0xFF0A1428);

    uint8_t c_idx = menu.get_char_cursor();
    const auto& hero = save_data.party[c_idx];

    // Top Header (X=1, Y=1, W=30, H=4)
    WindowBox::draw_box(buffer_.data(), width_, 1, 1, 30, 4);
    std::string title = hero.name + " " + IntroEngine::get_class_name(static_cast<uint8_t>(hero.char_class)) + "  LEV " + std::to_string(hero.level);
    Font::draw_string(buffer_.data(), width_, 2, 2, title);

    // EXP Box (X=1, Y=5, W=30, H=5)
    WindowBox::draw_box(buffer_.data(), width_, 1, 5, 30, 5);
    Font::draw_string(buffer_.data(), width_, 2, 6, "EXP. POINTS: " + std::to_string(hero.exp));
    uint32_t needed = MenuEngine::get_exp_needed_for_next_level(hero.level, hero.exp);
    Font::draw_string(buffer_.data(), width_, 2, 7, "FOR LEV UP:  " + std::to_string(needed));

    // Base Attributes Panel (X=1, Y=11, W=15, H=12)
    WindowBox::draw_box(buffer_.data(), width_, 1, 11, 15, 12);
    Font::draw_string(buffer_.data(), width_, 2, 12, "ATTRIBUTES");
    Font::draw_string(buffer_.data(), width_, 2, 14, "STR.  " + std::to_string(hero.stats.strength));
    Font::draw_string(buffer_.data(), width_, 2, 16, "AGI.  " + std::to_string(hero.stats.agility));
    Font::draw_string(buffer_.data(), width_, 2, 18, "INT.  " + std::to_string(hero.stats.intelligence));
    Font::draw_string(buffer_.data(), width_, 2, 20, "VIT.  " + std::to_string(hero.stats.vitality));
    Font::draw_string(buffer_.data(), width_, 2, 21, "LUCK  " + std::to_string(hero.stats.luck));

    // Combat Parameters Panel (X=17, Y=11, W=14, H=12)
    WindowBox::draw_box(buffer_.data(), width_, 17, 11, 14, 12);
    Font::draw_string(buffer_.data(), width_, 18, 12, "COMBAT STATS");
    Font::draw_string(buffer_.data(), width_, 18, 14, "DAMAGE  " + std::to_string(hero.stats.damage));
    Font::draw_string(buffer_.data(), width_, 18, 16, "HIT %   " + std::to_string(hero.stats.hit_rate));
    Font::draw_string(buffer_.data(), width_, 18, 18, "ABSORB  " + std::to_string(hero.stats.absorb));
    Font::draw_string(buffer_.data(), width_, 18, 20, "EVADE % " + std::to_string(hero.stats.evade));

    // Bottom Box (X=1, Y=24, W=30, H=5)
    WindowBox::draw_box(buffer_.data(), width_, 1, 24, 30, 5);
    Font::draw_string(buffer_.data(), width_, 2, 25, "LEFT/RIGHT: Switch Hero");
    Font::draw_string(buffer_.data(), width_, 2, 26, "CONFIRM/CANCEL: Back to Menu");
}

void Renderer::draw_world_map_screen(const DataLoader& loader, int player_x, int player_y) {
    clear(0xFF000000);

    // Ornate World Map Border (X=2, Y=1, W=28, H=28)
    WindowBox::draw_box(buffer_.data(), width_, 2, 1, 28, 28);
    Font::draw_string(buffer_.data(), width_, 11, 2, "WORLD MAP");

    const auto& ow_map = loader.get_overworld_map();
    int map_origin_x = 64; // pixel coords (256/2 - 128/2 = 64)
    int map_origin_y = 32;

    if (ow_map.size() >= 256 * 256) {
        for (int my = 0; my < 128; ++my) {
            for (int mx = 0; mx < 128; ++mx) {
                // Downsample 2x2 macroblock into 1 pixel
                uint8_t tile = ow_map[(my * 2) * 256 + (mx * 2)];
                uint32_t pcol = 0xFF145214; // Default grass green
                if (tile == 1) pcol = 0xFF0E380E; // Forest dark green
                else if (tile == 2) pcol = 0xFF8A6A40; // Mountain brown
                else if (tile == 3) pcol = 0xFF183890; // Ocean dark blue
                else if (tile >= 10 && tile <= 13) pcol = 0xFFE0C020; // Town/Castle yellow

                int sx = map_origin_x + mx;
                int sy = map_origin_y + my;
                if (sx >= 0 && sx < width_ && sy >= 0 && sy < height_) {
                    buffer_[sy * width_ + sx] = pcol;
                }
            }
        }
    } else {
        // Fallback procedural mini-map
        draw_rect(map_origin_x, map_origin_y, 128, 128, 0xFF183890);
        draw_rect(map_origin_x + 30, map_origin_y + 30, 68, 68, 0xFF145214);
    }

    // Blinking Player Crosshair (+)
    if ((frame_counter_ / 15) % 2 == 0) {
        int px = map_origin_x + (player_x / 2);
        int py = map_origin_y + (player_y / 2);
        draw_rect(px - 2, py, 5, 1, 0xFFFFFFFF);
        draw_rect(px, py - 2, 1, 5, 0xFFFFFFFF);
    }

    // Location coordinates & help banner
    std::string loc_str = "X:" + std::to_string(player_x) + " Y:" + std::to_string(player_y);
    Font::draw_string(buffer_.data(), width_, 4, 25, loc_str);
    Font::draw_string(buffer_.data(), width_, 14, 25, "[M/ESC]: Close");
}

void Renderer::draw_shop(const MenuEngine& menu, const GameSaveData& save_data, const DataLoader& loader) {
    (void)loader;
    clear(0xFF000000);
    const auto& shop = menu.get_current_shop();
    ShopMode mode = menu.get_shop_mode();
    uint8_t cursor = menu.get_shop_cursor();
    uint8_t sub_cursor = menu.get_shop_sub_cursor();

    // 1. Top-Left Box: Shopkeeper / Counter Area (X=0, Y=0, W=16, H=10)
    WindowBox::draw_box(buffer_.data(), width_, 0, 0, 16, 10);
    std::string shop_title = "SHOP";
    switch (shop.type) {
        case ShopType::WEAPON: shop_title = "WEAPON SHOP"; break;
        case ShopType::ARMOR: shop_title = "ARMOR SHOP"; break;
        case ShopType::WHITE_MAGIC: shop_title = "WHITE MAGIC"; break;
        case ShopType::BLACK_MAGIC: shop_title = "BLACK MAGIC"; break;
        case ShopType::ITEM: shop_title = "ITEM SHOP"; break;
        case ShopType::CARAVAN: shop_title = "CARAVAN"; break;
        case ShopType::INN: shop_title = "INN"; break;
        case ShopType::CLINIC: shop_title = "CLINIC"; break;
    }
    Font::draw_string(buffer_.data(), width_, 2, 1, shop_title);

    // Draw Shopkeeper counter and avatar
    draw_rect(32, 32, 64, 32, 0xFF402818); // Wooden Counter
    draw_rect(56, 16, 16, 16, 0xFF5090D0); // Shopkeeper Sprite Head
    Font::draw_string(buffer_.data(), width_, 7, 2, "o_o");

    // 2. Top-Right Box: Party GP & Status / Action selection (X=16, Y=0, W=16, H=10)
    WindowBox::draw_box(buffer_.data(), width_, 16, 0, 16, 10);
    Font::draw_string(buffer_.data(), width_, 18, 1, "GOLD");
    Font::draw_string(buffer_.data(), width_, 18, 2, std::to_string(save_data.gold) + " G");

    if (mode == ShopMode::BUY_SELL_EXIT) {
        bool has_sell = (shop.type == ShopType::WEAPON || shop.type == ShopType::ARMOR);
        Font::draw_string(buffer_.data(), width_, 19, 4, "BUY");
        if (has_sell) {
            Font::draw_string(buffer_.data(), width_, 19, 5, "SELL");
            Font::draw_string(buffer_.data(), width_, 19, 6, "EXIT");
            Font::draw_string(buffer_.data(), width_, 18, 4 + cursor, ">");
        } else {
            Font::draw_string(buffer_.data(), width_, 19, 5, "EXIT");
            Font::draw_string(buffer_.data(), width_, 18, 4 + cursor, ">");
        }
    }

    // 3. Bottom-Left Box: Item Inventory / Hero Selector (X=0, Y=10, W=16, H=14)
    WindowBox::draw_box(buffer_.data(), width_, 0, 10, 16, 14);

    if (mode == ShopMode::BUY_SELECT_ITEM) {
        for (int i = 0; i < 4; ++i) {
            uint8_t item_id = shop.items[i];
            if (item_id != 0xFF) {
                std::string iname = menu.get_item_name(shop.type, item_id);
                uint32_t price = menu.get_item_price(shop.type, item_id);
                Font::draw_string(buffer_.data(), width_, 2, 12 + i * 2, iname);
                Font::draw_string(buffer_.data(), width_, 2, 13 + i * 2, std::to_string(price) + " G");
            } else {
                Font::draw_string(buffer_.data(), width_, 2, 12 + i * 2, "--");
            }
        }
        Font::draw_string(buffer_.data(), width_, 1, 12 + cursor * 2, ">");
    } else if (mode == ShopMode::BUY_CHOOSE_HERO || mode == ShopMode::SELL_CHOOSE_HERO || mode == ShopMode::CLINIC_SELECT_HERO) {
        for (int i = 0; i < 4; ++i) {
            const auto& hero = save_data.party[i];
            std::string hstr = hero.name + " " + std::to_string(hero.stats.hp) + "/" + std::to_string(hero.stats.max_hp);
            Font::draw_string(buffer_.data(), width_, 2, 12 + i * 2, hstr);
        }
        Font::draw_string(buffer_.data(), width_, 1, 12 + cursor * 2, ">");
    } else if (mode == ShopMode::SELL_SELECT_ITEM) {
        uint8_t target_h = menu.get_shop_target_hero();
        const auto& hero = save_data.party[target_h];
        for (int i = 0; i < 4; ++i) {
            uint8_t item_id = (shop.type == ShopType::WEAPON) ? hero.weapons[i] : hero.armors[i];
            if (item_id != 0xFF) {
                std::string iname = menu.get_item_name(shop.type, item_id);
                uint32_t sprice = menu.get_sell_price(shop.type, item_id);
                Font::draw_string(buffer_.data(), width_, 2, 12 + i * 2, iname);
                Font::draw_string(buffer_.data(), width_, 2, 13 + i * 2, std::to_string(sprice) + " G");
            } else {
                Font::draw_string(buffer_.data(), width_, 2, 12 + i * 2, "--");
            }
        }
        Font::draw_string(buffer_.data(), width_, 1, 12 + cursor * 2, ">");
    }

    // 4. Bottom-Right Box: Shop Dialogue & Confirmation (X=16, Y=10, W=16, H=14)
    WindowBox::draw_box(buffer_.data(), width_, 16, 10, 16, 14);
    std::string dlg = menu.get_shop_dialogue();

    // Wrap dialogue text into 14-char lines
    size_t dpos = 0;
    int line_num = 0;
    while (dpos < dlg.size() && line_num < 4) {
        std::string chunk = dlg.substr(dpos, 14);
        Font::draw_string(buffer_.data(), width_, 18, 12 + line_num, chunk);
        dpos += 14;
        line_num++;
    }

    // Yes/No confirmation options
    if (mode == ShopMode::BUY_CONFIRM || mode == ShopMode::SELL_CONFIRM || mode == ShopMode::INN_PROMPT || mode == ShopMode::CLINIC_CONFIRM) {
        Font::draw_string(buffer_.data(), width_, 19, 18, "YES");
        Font::draw_string(buffer_.data(), width_, 19, 20, "NO");
        Font::draw_string(buffer_.data(), width_, 18, 18 + sub_cursor * 2, ">");
    }
}

} // namespace ff1
