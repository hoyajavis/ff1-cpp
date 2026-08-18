#ifndef MAP_TYPES_HPP
#define MAP_TYPES_HPP

#include "game_types.hpp"
#include <vector>
#include <array>
#include <string>
#include <cstdint>

namespace ff1 {

// Standard NES FF1 Special Tile Types
namespace SpecialTile {
    constexpr uint8_t WALKABLE     = 0x00;
    constexpr uint8_t IMPASSABLE   = 0x01;
    constexpr uint8_t CLOSED_DOOR  = 0x02;
    constexpr uint8_t OPEN_DOOR    = 0x03;
    constexpr uint8_t TREASURE     = 0x04;
    constexpr uint8_t DAMAGE_LAVA  = 0x06;
    constexpr uint8_t TELEPORT     = 0x80;
}

// 16x16 Macroblock TSA Tile Structure (composed of four 8x8 NES CHR sub-tiles)
struct TSABlock {
    uint8_t top_left = 0;
    uint8_t top_right = 0;
    uint8_t bottom_left = 0;
    uint8_t bottom_right = 0;
    uint8_t palette_attr = 0;
    uint8_t tile_prop = 0; // 0=walk, 1=wall, 2=door, 4=chest, 6=damage
};

// Map NPC Spawn Entry (from 0E_95D5_objectdata.bin)
struct NPCObjectData {
    uint8_t obj_id = 0;
    uint8_t map_id = 0;
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t graphic_id = 0;
    uint8_t move_type = 0; // 0=stationary, 1=wander, 2=look around
    uint8_t dialogue_id = 0;
};

// Teleport Entry
struct TeleportEntry {
    uint8_t from_map = 0;
    uint8_t from_x = 0;
    uint8_t from_y = 0;
    uint8_t target_map = 0;
    uint8_t target_x = 0;
    uint8_t target_y = 0;
    bool is_warp = false;
};

// Standard Map Definition (64x64 or 32x32)
struct StandardMapData {
    uint8_t map_id = 0;
    std::string name;
    uint8_t width = 32;
    uint8_t height = 32;
    uint8_t tileset_id = 0;
    std::vector<uint8_t> layout;
    std::vector<TSABlock> tsa_blocks;
};

} // namespace ff1

#endif // MAP_TYPES_HPP
