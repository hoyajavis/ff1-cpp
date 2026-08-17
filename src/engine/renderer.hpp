#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "core/map_engine.hpp"
#include "core/battle_engine.hpp"
#include "core/cutscene_engine.hpp"
#include "core/minigame_engine.hpp"
#include "core/intro_engine.hpp"
#include "data/data_loader.hpp"
#include "state/save_system.hpp"
#include "engine/chr_decoder.hpp"
#include <vector>
#include <cstdint>

namespace ff1 {

class Renderer {
public:
    Renderer(int width = 256, int height = 240);

    void clear(uint32_t color = 0xFF000000);
    uint32_t* get_buffer() { return buffer_.data(); }
    const uint32_t* get_buffer() const { return buffer_.data(); }

    int get_width() const { return width_; }
    int get_height() const { return height_; }

    // Map & Entity Rasterizers reading real NES CHR banks
    void draw_map(const MapEngine& map, const DataLoader& loader, int center_x, int center_y);
    void draw_player(const PartyCharacter& lead_char, const DataLoader& loader, Direction facing, VehicleType vehicle = VehicleType::WALK);
    void draw_npc(int tile_x, int tile_y, int camera_x, int camera_y, const DataLoader& loader, uint32_t color = 0xFF40E0D0);

    // Full Battle Visual Engine
    void draw_battle(const BattleEngine& battle, const GameSaveData& save_data, const DataLoader& loader);

    // Title Screen & Party Creation Rasterizers
    void draw_intro_story(const IntroEngine& intro);
    void draw_title_screen(const IntroEngine& intro, bool has_save_file);
    void draw_party_creation(const IntroEngine& intro, const DataLoader& loader);
    void draw_name_input_screen(const IntroEngine& intro);

    // Cinematic & 15-Puzzle Rasterizers
    void draw_cutscene(const CutsceneEngine& cutscene, const DataLoader& loader);
    void draw_puzzle(const MiniGameEngine& minigame, const DataLoader& loader);

    // CHR Tile Rasterizer
    void draw_chr_tile(int px, int py, const PixelBuffer8x8& tile);

private:
    int width_ = 256;
    int height_ = 240;
    std::vector<uint32_t> buffer_;
    uint64_t frame_counter_ = 0;

    void draw_rect(int px, int py, int pw, int ph, uint32_t color);
};

} // namespace ff1

#endif // RENDERER_HPP
