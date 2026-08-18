#ifndef SAVE_SYSTEM_HPP
#define SAVE_SYSTEM_HPP

#include "data/game_types.hpp"
#include <string>
#include <vector>
#include <array>

namespace ff1 {

struct GameSaveData {
    uint32_t gold = 0;
    uint8_t vehicle = static_cast<uint8_t>(VehicleType::WALK);
    uint8_t cur_map = 0;
    uint8_t player_x = 0;
    uint8_t player_y = 0;

    // Vehicle persistence
    uint8_t ship_x = 140;
    uint8_t ship_y = 150;
    bool ship_visible = false;
    bool has_canoe = false;
    uint8_t airship_x = 175;
    uint8_t airship_y = 180;
    bool airship_visible = false;

    std::array<PartyCharacter, 4> party;
    Consumables consumables;
    std::array<bool, 4> orbs_lit = {false, false, false, false};
    std::array<uint8_t, 256> key_items_and_flags = {};
    std::array<uint8_t, 256> opened_chests = {};

    bool valid = false;
};

class SaveSystem {
public:
    static bool save_game(const std::string& filepath, const GameSaveData& data);
    static bool load_game(const std::string& filepath, GameSaveData& data);
};

} // namespace ff1

#endif // SAVE_SYSTEM_HPP
