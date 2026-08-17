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

    std::array<PartyCharacter, 4> party;
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
