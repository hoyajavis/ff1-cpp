#include "map_engine.hpp"
#include <algorithm>

namespace ff1 {

MapEngine::MapEngine(const DataLoader& loader, const MapLoader& map_loader, RNG& rng)
    : loader_(loader), map_loader_(map_loader), rng_(rng) {}

void MapEngine::load_map(uint8_t map_id, MapType type) {
    current_map_id_ = map_id;
    map_type_ = type;

    if (type == MapType::OVERWORLD) {
        width_ = 256;
        height_ = 256;
        current_map_name_ = "World Map";

        const auto& ow_data = loader_.get_overworld_map();
        if (ow_data.size() >= 256 * 256) {
            tile_data_ = ow_data;
        } else {
            tile_data_.assign(width_ * height_, 0); // 0 = Grass
            for (int y = 0; y < height_; ++y) {
                for (int x = 0; x < width_; ++x) {
                    size_t idx = y * width_ + x;
                    if (x == 152 && y == 144) tile_data_[idx] = 10; // Town
                    else if (x == 152 && y == 142) tile_data_[idx] = 11; // Castle
                    else if (x == 160 && y == 130) tile_data_[idx] = 12; // Temple of Fiends
                    else if (x == 140 && y == 150) tile_data_[idx] = 13; // Port
                    else if ((x >= 148 && x <= 156 && y >= 145 && y <= 150) ||
                             (x >= 155 && x <= 165 && y >= 135 && y <= 142)) {
                        tile_data_[idx] = 1; // Forest
                    } else if (x < 140 || x > 170 || y < 125 || y > 160) {
                        tile_data_[idx] = 2; // Mountain
                    } else if (x > 162) {
                        tile_data_[idx] = 3; // Ocean
                    } else {
                        tile_data_[idx] = 0; // Grass
                    }
                }
            }
        }
    } else { // Standard Map
        const auto& smap = map_loader_.get_standard_map(map_id);
        if (smap.layout.empty()) {
            width_ = 32;
            height_ = 32;
            current_map_name_ = "Standard Map " + std::to_string(map_id);
            tile_data_.assign(width_ * height_, 0);
        } else {
            width_ = smap.width;
            height_ = smap.height;
            current_map_name_ = smap.name;
            tile_data_ = smap.layout;
        }
    }

    setup_map_npcs(map_id);
    teleports_ = map_loader_.get_teleports();
}

void MapEngine::setup_map_npcs(uint8_t map_id) {
    npcs_.clear();
    chests_.clear();

    const auto& npc_objs = map_loader_.get_npcs_for_map(map_id);
    for (const auto& obj : npc_objs) {
        MapNPC npc;
        npc.id = obj.obj_id;
        npc.x = obj.x;
        npc.y = obj.y;
        npc.facing = Direction::DOWN;
        npc.graphic_id = obj.graphic_id;
        npc.dialogue = "NPC Object #" + std::to_string(obj.obj_id) + " (Dialogue " + std::to_string(obj.dialogue_id) + ")";
        npc.active = true;
        npcs_.push_back(npc);
    }

    if (map_id == 1) { // Conelia Town
        MapNPC n1; n1.id = 101; n1.x = 14; n1.y = 16; n1.dialogue = "Welcome to the City of Conelia!"; npcs_.push_back(n1);
        MapNPC n2; n2.id = 102; n2.x = 18; n2.y = 12; n2.dialogue = "The King of Conelia seeks the 4 Light Warriors."; npcs_.push_back(n2);
        MapChest c1; c1.x = 10; c1.y = 10; c1.chest_id = 1; c1.value = 300; chests_.push_back(c1);
    } else if (map_id == 2) { // Conelia Castle
        MapNPC n1; n1.id = 201; n1.x = 16; n1.y = 7; n1.dialogue = "King Conelia: Please rescue Princess Sarah from Garland!"; npcs_.push_back(n1);
        MapChest c1; c1.x = 7; c1.y = 11; c1.chest_id = 2; c1.value = 500; chests_.push_back(c1);
    }
}

uint8_t MapEngine::get_tile_at(int x, int y) const {
    if (map_type_ == MapType::OVERWORLD) {
        x = (x + width_) % width_;
        y = (y + height_) % height_;
    } else {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return 2; // Wall
    }
    return tile_data_[y * width_ + x];
}

bool MapEngine::can_move_to(int x, int y, VehicleType vehicle) const {
    uint8_t tile = get_tile_at(x, y);
    size_t tile_idx = static_cast<size_t>(tile & 0x7F);
    const auto& bank_00 = loader_.get_chr_bank_00();

    if (map_type_ == MapType::OVERWORLD) {
        if (bank_00.size() >= 0x0100) {
            uint8_t prop0 = bank_00[tile_idx * 2];
            if (vehicle == VehicleType::WALK) {
                // bit 0 = no walk
                return (prop0 & 0x01) == 0;
            } else if (vehicle == VehicleType::SHIP) {
                // bit 2 = no ship (0 means can sail), bit 5 = dock ship
                return ((prop0 & 0x04) == 0) || ((prop0 & 0x20) != 0);
            } else if (vehicle == VehicleType::AIRSHIP) {
                return true;
            }
        }
        if (tile_idx == 2 || tile_idx == 3) return false;
    } else {
        // Standard Map collision
        uint8_t tileset_id = 0;
        if (bank_00.size() >= 0x2D00) {
            tileset_id = bank_00[0x2CC0 + current_map_id_] & 0x07;
        }
        size_t prop_off = 0x0800 + (static_cast<size_t>(tileset_id) * 256) + (tile_idx * 2);
        if (prop_off < bank_00.size()) {
            uint8_t prop0 = bank_00[prop_off];
            // bit 0 = no walk (solid wall)
            return (prop0 & 0x01) == 0;
        }
        if (tile == 2 || tile == 36) return false;
    }
    return true;
}

bool MapEngine::check_door_unlock(int target_x, int target_y, GameSaveData& save_data, std::string& out_message) {
    uint8_t tile = get_tile_at(target_x, target_y);
    if (tile == 36) {
        bool has_key = (save_data.key_items_and_flags[static_cast<size_t>(KeyItem::MYSTIC_KEY)] != 0);
        if (has_key) {
            size_t idx = target_y * width_ + target_x;
            tile_data_[idx] = 0;
            out_message = "Unlocked door using the Mystic Key!";
            return true;
        } else {
            out_message = "The door is locked. Needs the Mystic Key!";
            return false;
        }
    }
    return false;
}

bool MapEngine::check_event_trigger(GameSaveData& save_data, std::string& out_message) {
    // Garland Altar in Temple of Fiends (Map 10, x=16, y=16)
    if (current_map_id_ == 10 && save_data.player_x == 16 && save_data.player_y == 16) {
        if (save_data.key_items_and_flags[0] == 0) { // Garland not yet defeated
            save_data.key_items_and_flags[0] = 1; // Mark rescued Sarah
            out_message = "Defeated Garland! Princess Sarah was rescued!";
            return true;
        }
    }
    return false;
}

bool MapEngine::move_player(Direction dir, GameSaveData& save_data, std::string& out_message) {
    player_facing_ = dir;
    out_message.clear();

    int dx = 0, dy = 0;
    if (dir == Direction::UP) dy = -1;
    else if (dir == Direction::DOWN) dy = 1;
    else if (dir == Direction::LEFT) dx = -1;
    else if (dir == Direction::RIGHT) dx = 1;

    int target_x = save_data.player_x + dx;
    int target_y = save_data.player_y + dy;

    check_door_unlock(target_x, target_y, save_data, out_message);

    if (!can_move_to(target_x, target_y, static_cast<VehicleType>(save_data.vehicle))) {
        return false;
    }

    if (map_type_ == MapType::OVERWORLD) {
        save_data.player_x = (target_x + width_) % width_;
        save_data.player_y = (target_y + height_) % height_;
    } else {
        save_data.player_x = target_x;
        save_data.player_y = target_y;
    }

    check_event_trigger(save_data, out_message);

    uint8_t step_tile = get_tile_at(save_data.player_x, save_data.player_y);
    size_t tile_idx = static_cast<size_t>(step_tile & 0x7F);

    if (step_tile == 6) {
        for (auto& hero : save_data.party) {
            if (hero.stats.hp > 1) hero.stats.hp -= 1;
        }
        out_message = "Stepped on lava! -1 HP";
    }

    const auto& bank_00 = loader_.get_chr_bank_00();

    // 1. Check Overworld ROM Teleport Triggers from bank_00.dat ($8000 + tileset_prop)
    if (map_type_ == MapType::OVERWORLD) {
        if (bank_00.size() >= 0x2E00) {
            uint8_t prop1 = bank_00[tile_idx * 2 + 1];
            if (prop1 & 0x80) {
                uint8_t tele_id = prop1 & 0x3F;
                uint8_t target_map = bank_00[0x2C40 + tele_id];
                uint8_t target_x   = bank_00[0x2C00 + tele_id];
                uint8_t target_y   = bank_00[0x2C20 + tele_id];

                load_map(target_map, MapType::STANDARD_MAP);
                save_data.cur_map = target_map;
                save_data.player_x = target_x;
                save_data.player_y = target_y;
                out_message = "Entered " + current_map_name_;
                return true;
            }
        }
    } else {
        // 2. Check Standard Map ROM Teleports & Exits
        uint8_t tileset_id = 0;
        if (bank_00.size() >= 0x2D00) {
            tileset_id = bank_00[0x2CC0 + current_map_id_] & 0x07;
        }
        size_t prop_off = 0x0800 + (static_cast<size_t>(tileset_id) * 256) + (tile_idx * 2);
        if (prop_off + 1 < bank_00.size()) {
            uint8_t prop1 = bank_00[prop_off + 1];
            if (prop1 & 0x80) {
                uint8_t tele_id = prop1 & 0x7F;
                if (tele_id >= 0x40) {
                    // Exit to Overworld ($AC60, $AC70)
                    size_t exit_idx = tele_id & 0x0F;
                    uint8_t exit_x = (0x2C60 + exit_idx < bank_00.size()) ? bank_00[0x2C60 + exit_idx] : 152;
                    uint8_t exit_y = (0x2C70 + exit_idx < bank_00.size()) ? bank_00[0x2C70 + exit_idx] : 145;

                    load_map(0, MapType::OVERWORLD);
                    save_data.cur_map = 0;
                    save_data.player_x = exit_x;
                    save_data.player_y = exit_y;
                    out_message = "Exited to World Map";
                    return true;
                } else if (tele_id < 64) {
                    // In-dungeon stairs/door teleport ($AD00, $AD40, $AD80)
                    uint8_t target_map = bank_00[0x2D80 + tele_id];
                    uint8_t target_x   = bank_00[0x2D00 + tele_id];
                    uint8_t target_y   = bank_00[0x2D40 + tele_id];

                    load_map(target_map, MapType::STANDARD_MAP);
                    save_data.cur_map = target_map;
                    save_data.player_x = target_x;
                    save_data.player_y = target_y;
                    out_message = "Entered " + current_map_name_;
                    return true;
                }
            }
        }
    }

    // 3. Check Custom Configured Teleports
    for (const auto& tp : teleports_) {
        if (tp.from_map == current_map_id_ && tp.from_x == save_data.player_x && tp.from_y == save_data.player_y) {
            load_map(tp.target_map, tp.target_map == 0 ? MapType::OVERWORLD : MapType::STANDARD_MAP);
            save_data.cur_map = tp.target_map;
            save_data.player_x = tp.target_x;
            save_data.player_y = tp.target_y;
            out_message = "Warped to " + current_map_name_;
            return true;
        }
    }

    return true;
}

bool MapEngine::check_interaction(GameSaveData& save_data, std::string& out_message) {
    int dx = 0, dy = 0;
    if (player_facing_ == Direction::UP) dy = -1;
    else if (player_facing_ == Direction::DOWN) dy = 1;
    else if (player_facing_ == Direction::LEFT) dx = -1;
    else if (player_facing_ == Direction::RIGHT) dx = 1;

    int facing_x = save_data.player_x + dx;
    int facing_y = save_data.player_y + dy;

    for (const auto& npc : npcs_) {
        if (npc.active && npc.x == facing_x && npc.y == facing_y) {
            out_message = npc.dialogue;
            return true;
        }
    }

    for (auto& chest : chests_) {
        if (chest.x == facing_x && chest.y == facing_y) {
            if (chest.opened) {
                out_message = "The chest is empty.";
            } else {
                chest.opened = true;
                if (chest.item_or_gp == 0) {
                    save_data.gold += chest.value;
                    out_message = "Opened chest! Found " + std::to_string(chest.value) + " GP!";
                } else {
                    out_message = "Opened chest! Found an Item!";
                }
            }
            return true;
        }
    }

    return false;
}

bool MapEngine::check_encounter(VehicleType vehicle) {
    if (vehicle == VehicleType::AIRSHIP) return false;
    if (map_type_ == MapType::STANDARD_MAP && (current_map_id_ == 1 || current_map_id_ == 2)) return false;

    uint8_t roll = rng_.next_byte();
    return (roll < 12);
}

} // namespace ff1
