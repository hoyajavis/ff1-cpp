#include "save_system.hpp"
#include <fstream>
#include <iostream>

namespace ff1 {

bool SaveSystem::save_game(const std::string& filepath, const GameSaveData& data) {
    std::ofstream out(filepath, std::ios::binary);
    if (!out) return false;

    // Header magic "FF1SAVE"
    const char magic[8] = "FF1SAVE";
    out.write(magic, 8);

    out.write(reinterpret_cast<const char*>(&data.gold), sizeof(data.gold));
    out.write(reinterpret_cast<const char*>(&data.vehicle), sizeof(data.vehicle));
    out.write(reinterpret_cast<const char*>(&data.cur_map), sizeof(data.cur_map));
    out.write(reinterpret_cast<const char*>(&data.player_x), sizeof(data.player_x));
    out.write(reinterpret_cast<const char*>(&data.player_y), sizeof(data.player_y));
    out.write(reinterpret_cast<const char*>(&data.ship_x), sizeof(data.ship_x));
    out.write(reinterpret_cast<const char*>(&data.ship_y), sizeof(data.ship_y));
    out.write(reinterpret_cast<const char*>(&data.ship_visible), sizeof(data.ship_visible));
    out.write(reinterpret_cast<const char*>(&data.has_canoe), sizeof(data.has_canoe));
    out.write(reinterpret_cast<const char*>(&data.airship_x), sizeof(data.airship_x));
    out.write(reinterpret_cast<const char*>(&data.airship_y), sizeof(data.airship_y));
    out.write(reinterpret_cast<const char*>(&data.airship_visible), sizeof(data.airship_visible));

    for (int i = 0; i < 4; ++i) {
        const auto& ch = data.party[i];
        uint8_t name_len = static_cast<uint8_t>(ch.name.size());
        out.write(reinterpret_cast<const char*>(&name_len), 1);
        if (name_len > 0) out.write(ch.name.data(), name_len);

        uint8_t ctype = static_cast<uint8_t>(ch.char_class);
        out.write(reinterpret_cast<const char*>(&ctype), 1);
        out.write(reinterpret_cast<const char*>(&ch.level), 1);
        out.write(reinterpret_cast<const char*>(&ch.exp), sizeof(ch.exp));
        out.write(reinterpret_cast<const char*>(&ch.stats), sizeof(ch.stats));
        out.write(reinterpret_cast<const char*>(ch.weapons.data()), 4);
        out.write(reinterpret_cast<const char*>(ch.armors.data()), 4);
        out.write(reinterpret_cast<const char*>(ch.spells.data()), 24);
    }

    out.write(reinterpret_cast<const char*>(&data.consumables), sizeof(data.consumables));
    out.write(reinterpret_cast<const char*>(data.orbs_lit.data()), 4);
    out.write(reinterpret_cast<const char*>(data.key_items_and_flags.data()), 256);
    out.write(reinterpret_cast<const char*>(data.opened_chests.data()), 256);

    return true;
}

bool SaveSystem::load_game(const std::string& filepath, GameSaveData& data) {
    std::ifstream in(filepath, std::ios::binary);
    if (!in) return false;

    char magic[8];
    in.read(magic, 8);
    if (std::string(magic, 7) != "FF1SAVE") return false;

    in.read(reinterpret_cast<char*>(&data.gold), sizeof(data.gold));
    in.read(reinterpret_cast<char*>(&data.vehicle), sizeof(data.vehicle));
    in.read(reinterpret_cast<char*>(&data.cur_map), sizeof(data.cur_map));
    in.read(reinterpret_cast<char*>(&data.player_x), sizeof(data.player_x));
    in.read(reinterpret_cast<char*>(&data.player_y), sizeof(data.player_y));
    in.read(reinterpret_cast<char*>(&data.ship_x), sizeof(data.ship_x));
    in.read(reinterpret_cast<char*>(&data.ship_y), sizeof(data.ship_y));
    in.read(reinterpret_cast<char*>(&data.ship_visible), sizeof(data.ship_visible));
    in.read(reinterpret_cast<char*>(&data.has_canoe), sizeof(data.has_canoe));
    in.read(reinterpret_cast<char*>(&data.airship_x), sizeof(data.airship_x));
    in.read(reinterpret_cast<char*>(&data.airship_y), sizeof(data.airship_y));
    in.read(reinterpret_cast<char*>(&data.airship_visible), sizeof(data.airship_visible));

    for (int i = 0; i < 4; ++i) {
        auto& ch = data.party[i];
        uint8_t name_len = 0;
        in.read(reinterpret_cast<char*>(&name_len), 1);
        if (name_len > 0) {
            ch.name.resize(name_len);
            in.read(&ch.name[0], name_len);
        }

        uint8_t ctype = 0;
        in.read(reinterpret_cast<char*>(&ctype), 1);
        ch.char_class = static_cast<ClassType>(ctype);

        in.read(reinterpret_cast<char*>(&ch.level), 1);
        in.read(reinterpret_cast<char*>(&ch.exp), sizeof(ch.exp));
        in.read(reinterpret_cast<char*>(&ch.stats), sizeof(ch.stats));
        in.read(reinterpret_cast<char*>(ch.weapons.data()), 4);
        in.read(reinterpret_cast<char*>(ch.armors.data()), 4);
        in.read(reinterpret_cast<char*>(ch.spells.data()), 24);
    }

    if (in.peek() != EOF) {
        in.read(reinterpret_cast<char*>(&data.consumables), sizeof(data.consumables));
        in.read(reinterpret_cast<char*>(data.orbs_lit.data()), 4);
    }
    in.read(reinterpret_cast<char*>(data.key_items_and_flags.data()), 256);
    in.read(reinterpret_cast<char*>(data.opened_chests.data()), 256);

    data.valid = true;
    return true;
}

} // namespace ff1
