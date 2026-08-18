#include "map_engine.hpp"
#include <algorithm>
#include <iostream>

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
        npc.dialogue = "NPC Object #" + std::to_string(obj.obj_id);
        npc.active = true;
        npc.quest_id = obj.obj_id;

        // Map authentic shopkeeper NPC IDs to shop transactions
        if (map_id == 2) { // Conelia Town
            if (npc.x == 11 && npc.y == 7) { npc.shop_id = 0; npc.dialogue = "Welcome to the Weapon Shop!"; }
            else if (npc.x == 19 && npc.y == 7) { npc.shop_id = 1; npc.dialogue = "Welcome to the Armor Shop!"; }
            else if (npc.x == 11 && npc.y == 15) { npc.shop_id = 2; npc.dialogue = "White Magic Shop Level 1."; }
            else if (npc.x == 19 && npc.y == 15) { npc.shop_id = 3; npc.dialogue = "Black Magic Shop Level 1."; }
            else if (npc.x == 7 && npc.y == 21) { npc.shop_id = 100; npc.dialogue = "Conelia Inn (30 GP)."; } // Inn
            else if (npc.x == 23 && npc.y == 21) { npc.shop_id = 101; npc.dialogue = "Conelia Clinic (40 GP)."; } // Clinic
        } else if (map_id == 3) { // Pravoka Town
            if (npc.x == 15 && npc.y == 15) { npc.shop_id = 4; npc.dialogue = "Pravoka Weapon Shop."; }
            else if (npc.x == 23 && npc.y == 15) { npc.shop_id = 5; npc.dialogue = "Pravoka Armor Shop."; }
            else if (npc.x == 15 && npc.y == 23) { npc.shop_id = 6; npc.dialogue = "White Magic Shop Level 2."; }
            else if (npc.x == 23 && npc.y == 23) { npc.shop_id = 7; npc.dialogue = "Black Magic Shop Level 2."; }
            else if (npc.x == 7 && npc.y == 15) { npc.shop_id = 102; npc.dialogue = "Pravoka Inn (50 GP)."; }
            else if (npc.x == 7 && npc.y == 23) { npc.shop_id = 103; npc.dialogue = "Pravoka Clinic (80 GP)."; }
        } else if (map_id == 4) { // Elfland Town
            if (npc.x == 15 && npc.y == 15) { npc.shop_id = 8; npc.dialogue = "Elfland Weapon Shop."; }
            else if (npc.x == 23 && npc.y == 15) { npc.shop_id = 9; npc.dialogue = "Elfland Armor Shop."; }
            else if (npc.x == 15 && npc.y == 23) { npc.shop_id = 10; npc.dialogue = "White Magic Shop Level 3."; }
            else if (npc.x == 23 && npc.y == 23) { npc.shop_id = 11; npc.dialogue = "Black Magic Shop Level 3."; }
            else if (npc.x == 31 && npc.y == 23) { npc.shop_id = 12; npc.dialogue = "White Magic Shop Level 4."; }
            else if (npc.x == 39 && npc.y == 23) { npc.shop_id = 13; npc.dialogue = "Black Magic Shop Level 4."; }
            else if (npc.x == 7 && npc.y == 15) { npc.shop_id = 104; npc.dialogue = "Elfland Inn (100 GP)."; }
            else if (npc.x == 7 && npc.y == 23) { npc.shop_id = 105; npc.dialogue = "Elfland Clinic (200 GP)."; }
        } else if (map_id == 6) { // Melmond Town
            if (npc.x == 15 && npc.y == 15) { npc.shop_id = 14; npc.dialogue = "Melmond Weapon Shop."; }
            else if (npc.x == 23 && npc.y == 15) { npc.shop_id = 15; npc.dialogue = "Melmond Armor Shop."; }
            else if (npc.x == 15 && npc.y == 23) { npc.shop_id = 16; npc.dialogue = "White Magic Shop Level 5."; }
            else if (npc.x == 23 && npc.y == 23) { npc.shop_id = 17; npc.dialogue = "Black Magic Shop Level 5."; }
            else if (npc.x == 7 && npc.y == 15) { npc.shop_id = 106; npc.dialogue = "Melmond Inn (100 GP)."; }
            else if (npc.x == 7 && npc.y == 23) { npc.shop_id = 107; npc.dialogue = "Melmond Clinic (400 GP)."; }
        } else if (map_id == 7) { // Crescent Lake Town
            if (npc.x == 15 && npc.y == 15) { npc.shop_id = 18; npc.dialogue = "Crescent Lake Weapon Shop."; }
            else if (npc.x == 23 && npc.y == 15) { npc.shop_id = 19; npc.dialogue = "Crescent Lake Armor Shop."; }
            else if (npc.x == 15 && npc.y == 23) { npc.shop_id = 20; npc.dialogue = "White Magic Shop Level 6."; }
            else if (npc.x == 23 && npc.y == 23) { npc.shop_id = 21; npc.dialogue = "Black Magic Shop Level 6."; }
            else if (npc.x == 7 && npc.y == 15) { npc.shop_id = 108; npc.dialogue = "Crescent Lake Inn (200 GP)."; }
            else if (npc.x == 7 && npc.y == 23) { npc.shop_id = 109; npc.dialogue = "Crescent Lake Clinic (400 GP)."; }
        } else if (map_id == 8) { // Gaia Town
            if (npc.x == 15 && npc.y == 15) { npc.shop_id = 22; npc.dialogue = "Gaia Weapon Shop."; }
            else if (npc.x == 23 && npc.y == 15) { npc.shop_id = 23; npc.dialogue = "Gaia Armor Shop."; }
            else if (npc.x == 15 && npc.y == 23) { npc.shop_id = 24; npc.dialogue = "White Magic Shop Level 7."; }
            else if (npc.x == 23 && npc.y == 23) { npc.shop_id = 25; npc.dialogue = "Black Magic Shop Level 7."; }
            else if (npc.x == 31 && npc.y == 23) { npc.shop_id = 26; npc.dialogue = "White Magic Shop Level 8."; }
            else if (npc.x == 39 && npc.y == 23) { npc.shop_id = 27; npc.dialogue = "Black Magic Shop Level 8."; }
            else if (npc.x == 7 && npc.y == 15) { npc.shop_id = 110; npc.dialogue = "Gaia Inn (500 GP)."; }
            else if (npc.x == 7 && npc.y == 23) { npc.shop_id = 111; npc.dialogue = "Gaia Clinic (800 GP)."; }
        } else if (map_id == 60) { // Caravan
            npc.shop_id = 200; // Caravan special shop
            npc.dialogue = "Welcome to the Desert Caravan! Look at this rare item.";
        }

        npcs_.push_back(npc);
    }

    // Default Town & Castle NPC and Chest Populators
    if (map_id == 1 || map_id == 0) { // Conelia Castle
        MapNPC king; king.id = 1; king.quest_id = 1; king.x = 16; king.y = 7; king.dialogue = "King Conelia: Please rescue Princess Sarah from Garland!"; npcs_.push_back(king);
        MapNPC sarah; sarah.id = 0x12; sarah.quest_id = 0x12; sarah.x = 18; sarah.y = 7; sarah.dialogue = "Princess Sarah: The 4 Light Warriors will save our world."; npcs_.push_back(sarah);
        MapChest c1; c1.x = 7; c1.y = 11; c1.chest_id = 1; c1.value = 500; chests_.push_back(c1);
    } else if (map_id == 2) { // Conelia Town
        MapNPC n1; n1.id = 101; n1.x = 14; n1.y = 16; n1.dialogue = "Welcome to the City of Conelia!"; npcs_.push_back(n1);
        MapNPC n2; n2.id = 102; n2.x = 18; n2.y = 12; n2.dialogue = "The King seeks the 4 Light Warriors."; npcs_.push_back(n2);
        MapChest c1; c1.x = 10; c1.y = 10; c1.chest_id = 2; c1.value = 300; chests_.push_back(c1);
    } else if (map_id == 3) { // Pravoka Town
        MapNPC bikke; bikke.id = 4; bikke.quest_id = 4; bikke.x = 16; bikke.y = 10; bikke.dialogue = "Yarr! Hand over your gold!"; npcs_.push_back(bikke);
    } else if (map_id == 5) { // Castle Elfland
        MapNPC prince; prince.id = 6; prince.quest_id = 6; prince.x = 16; prince.y = 12; prince.dialogue = "The Elf Prince sleeps deeply..."; npcs_.push_back(prince);
    } else if (map_id == 10) { // Temple of Fiends 1F
        MapNPC garland; garland.id = 2; garland.quest_id = 2; garland.x = 16; garland.y = 16; garland.dialogue = "I, Garland, will knock you all down!"; npcs_.push_back(garland);
        MapNPC sarah1; sarah1.id = 3; sarah1.quest_id = 3; sarah1.x = 16; sarah1.y = 15; sarah1.dialogue = "Princess Sarah: Help me, brave warriors!"; npcs_.push_back(sarah1);
    } else if (map_id == 17) { // Dwarven Cave
        MapNPC nerrick; nerrick.id = 8; nerrick.quest_id = 8; nerrick.x = 12; nerrick.y = 14; nerrick.dialogue = "I'm digging a canal, but the rock is too hard!"; npcs_.push_back(nerrick);
        MapNPC smyth; smyth.id = 9; smyth.quest_id = 9; smyth.x = 24; smyth.y = 14; smyth.dialogue = "Bring me ADAMANT and I will forge Excalibur!"; npcs_.push_back(smyth);
    } else if (map_id == 18) { // Matoya's Cave
        MapNPC matoya; matoya.id = 0x0A; matoya.quest_id = 0x0A; matoya.x = 16; matoya.y = 12; matoya.dialogue = "I can't see anything without my CRYSTAL EYE!"; npcs_.push_back(matoya);
    } else if (map_id == 6) { // Melmond Town
        MapNPC unne; unne.id = 0x15; unne.quest_id = 0x15; unne.x = 16; unne.y = 8; unne.dialogue = "Dr. Unne: I study ancient civilizations. If only I had a decipherable SLAB..."; npcs_.push_back(unne);
    } else if (map_id == 8) { // Gaia Town
        MapNPC fairy; fairy.id = 0x18; fairy.quest_id = 0x18; fairy.x = 20; fairy.y = 10; fairy.dialogue = "The spring is peaceful and serene."; npcs_.push_back(fairy);
    } else if (map_id == 9) { // Lufenia Town
        MapNPC elder; elder.id = 0x16; elder.quest_id = 0x16; elder.x = 16; elder.y = 12; elder.dialogue = "Lu-pa-ga-to-mu..."; npcs_.push_back(elder);
        MapNPC envoy; envoy.id = 0x17; envoy.quest_id = 0x17; envoy.x = 20; envoy.y = 12; envoy.dialogue = "Ku-ri-si-ta-ru..."; npcs_.push_back(envoy);
    } else if (map_id == 19) { // Sarda's Cave
        MapNPC sarda; sarda.id = 0x0D; sarda.quest_id = 0x0D; sarda.x = 16; sarda.y = 16; sarda.dialogue = "I am Sarda. Defeat the Fiend of Earth!"; npcs_.push_back(sarda);
    } else if (map_id == 24) { // Western Keep
        MapNPC astos; astos.id = 7; astos.quest_id = 7; astos.x = 16; astos.y = 12; astos.dialogue = "Bring me the CROWN from the Marsh Cave."; npcs_.push_back(astos);
    } else if (map_id == 25) { // Titan's Tunnel
        MapNPC titan; titan.id = 0x14; titan.quest_id = 0x14; titan.x = 16; titan.y = 16; titan.dialogue = "Titan hungry... Want delicious gems..."; npcs_.push_back(titan);
    } else if (map_id == 15) { // Bahamut's Cave
        MapNPC bahamut; bahamut.id = 0x0E; bahamut.quest_id = 0x0E; bahamut.x = 16; bahamut.y = 12; bahamut.dialogue = "I am Bahamut, King of Dragons. Bring me the TAIL of courage."; npcs_.push_back(bahamut);
    } else if (map_id == 38) { // Marsh Cave B3
        MapChest crown_chest; crown_chest.x = 24; crown_chest.y = 24; crown_chest.chest_id = 10; crown_chest.item_or_gp = 1; crown_chest.value = static_cast<uint16_t>(KeyItem::CROWN); chests_.push_back(crown_chest);
    } else if (map_id == 42) { // Ice Cave B3
        MapChest floater_chest; floater_chest.x = 16; floater_chest.y = 15; floater_chest.chest_id = 20; floater_chest.item_or_gp = 1; floater_chest.value = static_cast<uint16_t>(KeyItem::FLOATER); chests_.push_back(floater_chest);
    } else if (map_id == 48) { // Sunken Shrine 5F
        MapChest slab_chest; slab_chest.x = 24; slab_chest.y = 24; slab_chest.chest_id = 21; slab_chest.item_or_gp = 1; slab_chest.value = static_cast<uint16_t>(KeyItem::SLAB); chests_.push_back(slab_chest);
    } else if (map_id == 60) { // ToF Past 5F / Sanctum of Chaos
        MapNPC garland_chaos;
        garland_chaos.id = 0x78; garland_chaos.quest_id = 0x78; garland_chaos.x = 16; garland_chaos.y = 16;
        garland_chaos.dialogue = "Garland: In 2000 years, I was defeated... But the 4 Fiends sent me to the past! The time loop is eternal! I am CHAOS!";
        npcs_.push_back(garland_chaos);

        MapChest masamune_chest;
        masamune_chest.x = 8; masamune_chest.y = 8; masamune_chest.chest_id = 30; masamune_chest.item_or_gp = 0; masamune_chest.value = 38; // Masamune
        chests_.push_back(masamune_chest);
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

void MapEngine::set_tile_at(int x, int y, uint8_t tile) {
    if (map_type_ == MapType::OVERWORLD) {
        x = (x + width_) % width_;
        y = (y + height_) % height_;
    } else {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    }
    tile_data_[y * width_ + x] = tile;
}

bool MapEngine::is_port_tile(int x, int y) const {
    if (map_type_ != MapType::OVERWORLD) return false;
    uint8_t tile = get_tile_at(x, y);
    size_t tile_idx = static_cast<size_t>(tile & 0x7F);

    if (tile_idx == 13) return true; // Default mock port tile

    const auto& bank_00 = loader_.get_chr_bank_00();
    if (bank_00.size() >= 0x0100) {
        uint8_t prop0 = bank_00[tile_idx * 2];
        return (prop0 & 0x20) != 0; // bit 5 = OWTP_DOCKSHIP
    }
    return false;
}

bool MapEngine::can_land_airship(int x, int y) const {
    if (map_type_ != MapType::OVERWORLD) return false;
    uint8_t tile = get_tile_at(x, y);
    size_t tile_idx = static_cast<size_t>(tile & 0x7F);

    // In NES FF1, the Airship can only land on flat grasslands (Tile 0)
    // Non-landable tiles: 1 (Forest), 2 (Mountain), 3 (Ocean), 4 (River), 5 (Desert), 6 (Swamp)
    if (tile_idx == 0) return true;

    const auto& bank_00 = loader_.get_chr_bank_00();
    if (bank_00.size() >= 0x0100) {
        uint8_t prop0 = bank_00[tile_idx * 2];
        return (prop0 == 0); // Flat grassland property
    }
    return false;
}

bool MapEngine::land_airship(GameSaveData& save_data, std::string& out_message) {
    if (save_data.vehicle != static_cast<uint8_t>(VehicleType::AIRSHIP)) {
        return false;
    }
    if (can_land_airship(save_data.player_x, save_data.player_y)) {
        save_data.airship_x = save_data.player_x;
        save_data.airship_y = save_data.player_y;
        save_data.vehicle = static_cast<uint8_t>(VehicleType::WALK);
        out_message = "Landed the Airship.";
        return true;
    } else {
        out_message = "Can only land on flat grasslands!";
        return false;
    }
}

bool MapEngine::can_move_to(int x, int y, VehicleType vehicle, const GameSaveData* save) const {
    uint8_t tile = get_tile_at(x, y);
    size_t tile_idx = static_cast<size_t>(tile & 0x7F);
    const auto& bank_00 = loader_.get_chr_bank_00();

    if (map_type_ == MapType::OVERWORLD) {
        // Northern Bridge gate check (x=152, y=138)
        if (x == 152 && y == 138) {
            if (save && save->key_items_and_flags[QuestFlag::BRIDGE_BUILT] == 0) {
                return false; // Northern bridge not yet built by King
            }
        }

        // Canal barrier check (x=144, y=152)
        if (x == 144 && y == 152) {
            if (save && save->key_items_and_flags[QuestFlag::CANAL_DEMOLISHED] != 0) {
                if (vehicle == VehicleType::SHIP) return true; // Canal opened to sea
            }
        }

        if (bank_00.size() >= 0x0100) {
            uint8_t prop0 = bank_00[tile_idx * 2];
            if (vehicle == VehicleType::WALK) {
                return (prop0 & 0x01) == 0; // bit 0 = no walk
            } else if (vehicle == VehicleType::SHIP) {
                // bit 2 = no ship (0 means can sail), bit 5 = dock ship
                return ((prop0 & 0x04) == 0) || ((prop0 & 0x20) != 0);
            } else if (vehicle == VehicleType::CANOE) {
                // River tiles or walk tiles with canoe
                return (tile_idx == 4 || (prop0 & 0x01) == 0);
            } else if (vehicle == VehicleType::AIRSHIP) {
                return true;
            }
        }
        if (vehicle == VehicleType::WALK && (tile_idx == 2 || tile_idx == 3)) return false;
        if (vehicle == VehicleType::SHIP && (tile_idx != 3 && tile_idx != 13)) return false;
    } else {
        // Standard Map collision
        uint8_t tileset_id = 0;
        if (bank_00.size() >= 0x2D00) {
            tileset_id = bank_00[0x2CC0 + current_map_id_] & 0x07;
        }
        size_t prop_off = 0x0800 + (static_cast<size_t>(tileset_id) * 256) + (tile_idx * 2);
        if (prop_off < bank_00.size()) {
            uint8_t prop0 = bank_00[prop_off];
            return (prop0 & 0x01) == 0; // bit 0 = no walk (solid wall)
        }
        if (tile == 2 || tile == 36) return false;
    }
    return true;
}

bool MapEngine::can_move_to(int x, int y, VehicleType vehicle) const {
    return can_move_to(x, y, vehicle, nullptr);
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

bool MapEngine::check_event_trigger(GameSaveData& save_data, std::string& out_message, int& out_spike_battle) {
    out_spike_battle = -1;

    // 0. Ryukahn Desert Airship Raising (Overworld map 0, x=175, y=180)
    if (map_type_ == MapType::OVERWORLD && save_data.player_x == 175 && save_data.player_y == 180) {
        if (save_data.key_items_and_flags[QuestFlag::AIRSHIP_RAISED] == 0) {
            if (save_data.key_items_and_flags[static_cast<size_t>(KeyItem::FLOATER)] != 0) {
                save_data.key_items_and_flags[static_cast<size_t>(KeyItem::FLOATER)] = 0;
                save_data.key_items_and_flags[QuestFlag::AIRSHIP_RAISED] = 1;
                save_data.airship_visible = true;
                save_data.airship_x = 175;
                save_data.airship_y = 180;
                save_data.vehicle = static_cast<uint8_t>(VehicleType::AIRSHIP);
                out_message = "The FLOATER resonates! The ancient Airship rises from the desert sands!";
                return true;
            }
        }
    }

    // 1. Earth Cave B5 Earth Altar Activation (Map 30, x=16, y=14)
    if (current_map_id_ == 30 && save_data.player_x == 16 && save_data.player_y == 14) {
        if (save_data.key_items_and_flags[QuestFlag::LICH_DEFEATED] != 0) {
            save_data.orbs_lit[static_cast<size_t>(OrbType::EARTH)] = true;
            save_data.key_items_and_flags[QuestFlag::EARTH_ORB_LIT] = 1;
            load_map(0, MapType::OVERWORLD);
            save_data.cur_map = 0;
            save_data.player_x = 88;
            save_data.player_y = 114;
            out_message = "The EARTH ORB shines with ancient light! Warped outside.";
            return true;
        }
    }

    // 2. Earth Cave B3 Stone Slab unsealing with Earth Rod (Map 28, x=16, y=18)
    if (current_map_id_ == 28 && save_data.player_x == 16 && save_data.player_y == 18) {
        if (save_data.key_items_and_flags[QuestFlag::EARTH_PLATE_SHATTERED] == 0) {
            if (save_data.key_items_and_flags[static_cast<size_t>(KeyItem::ROD)] != 0) {
                save_data.key_items_and_flags[QuestFlag::EARTH_PLATE_SHATTERED] = 1;
                out_message = "The EARTH ROD shatters the stone plate! Stairway opened.";
                return true;
            } else {
                out_message = "A heavy stone plate blocks the stairway.";
            }
        }
    }

    // 3. Coordinate Spike Encounters
    // Marsh Cave B3 Crown Room spike (Map 38, x=24, y=24)
    if (current_map_id_ == 38 && save_data.player_x == 24 && save_data.player_y == 24) {
        if (save_data.key_items_and_flags[QuestFlag::CROWN_RETRIEVED] == 0) {
            out_spike_battle = 0x30; // Piscodemons
            out_message = "Ambushed by Piscodemons guarding the chest!";
            return true;
        }
    }

    // Earth Cave B3 Vampire Room spike (Map 28, x=16, y=16)
    if (current_map_id_ == 28 && save_data.player_x == 16 && save_data.player_y == 16) {
        if (save_data.key_items_and_flags[QuestFlag::VAMPIRE_DEFEATED] == 0) {
            out_spike_battle = 0x7C; // Vampire Boss
            out_message = "The Vampire lunges from the darkness!";
            return true;
        }
    }

    // Earth Cave B5 Lich Fiend Room spike (Map 30, x=16, y=16)
    if (current_map_id_ == 30 && save_data.player_x == 16 && save_data.player_y == 16) {
        if (save_data.key_items_and_flags[QuestFlag::LICH_DEFEATED] == 0) {
            out_spike_battle = 0x70; // Lich Boss
            out_message = "Lich, Fiend of Earth, emerges!";
            return true;
        }
    }

    // 4. Mt. Gurgu B5 Fire Fiend Room & Altar (Map 23, x=16, y=16 / y=14)
    if (current_map_id_ == 23 && save_data.player_x == 16 && save_data.player_y == 16) {
        if (save_data.key_items_and_flags[QuestFlag::KARY_DEFEATED] == 0) {
            out_spike_battle = 0x71; // Kary Boss
            out_message = "Kary, Fiend of Fire, slithers from the flames!";
            return true;
        }
    }
    if (current_map_id_ == 23 && save_data.player_x == 16 && save_data.player_y == 14) {
        if (save_data.key_items_and_flags[QuestFlag::KARY_DEFEATED] != 0) {
            save_data.orbs_lit[static_cast<size_t>(OrbType::FIRE)] = true;
            save_data.key_items_and_flags[QuestFlag::FIRE_ORB_LIT] = 1;
            load_map(0, MapType::OVERWORLD);
            save_data.cur_map = 0;
            save_data.player_x = 142;
            save_data.player_y = 110;
            out_message = "The FIRE ORB burns brightly once again! Warped outside.";
            return true;
        }
    }

    // 5. Sunken Shrine 5F Water Fiend Room & Altar (Map 48, x=16, y=16 / y=14)
    if (current_map_id_ == 48 && save_data.player_x == 16 && save_data.player_y == 16) {
        if (save_data.key_items_and_flags[QuestFlag::KRAKEN_DEFEATED] == 0) {
            out_spike_battle = 0x72; // Kraken Boss
            out_message = "Kraken, Fiend of Water, surges from the abyss!";
            return true;
        }
    }
    if (current_map_id_ == 48 && save_data.player_x == 16 && save_data.player_y == 14) {
        if (save_data.key_items_and_flags[QuestFlag::KRAKEN_DEFEATED] != 0) {
            save_data.orbs_lit[static_cast<size_t>(OrbType::WATER)] = true;
            save_data.key_items_and_flags[QuestFlag::WATER_ORB_LIT] = 1;
            load_map(0, MapType::OVERWORLD);
            save_data.cur_map = 0;
            save_data.player_x = 190;
            save_data.player_y = 70;
            out_message = "The WATER ORB sparkles with brilliant light! Warped outside.";
            return true;
        }
    }

    // 6. Flying Fortress 5F Wind Fiend Room & Altar (Map 55, x=16, y=16 / y=14)
    if (current_map_id_ == 55 && save_data.player_x == 16 && save_data.player_y == 16) {
        if (save_data.key_items_and_flags[QuestFlag::TIAMAT_DEFEATED] == 0) {
            out_spike_battle = 0x73; // Tiamat Boss
            out_message = "Tiamat, Fiend of Wind, unleashes a tempestuous roar!";
            return true;
        }
    }
    if (current_map_id_ == 55 && save_data.player_x == 16 && save_data.player_y == 14) {
        if (save_data.key_items_and_flags[QuestFlag::TIAMAT_DEFEATED] != 0) {
            save_data.orbs_lit[static_cast<size_t>(OrbType::WIND)] = true;
            save_data.key_items_and_flags[QuestFlag::WIND_ORB_LIT] = 1;
            load_map(0, MapType::OVERWORLD);
            save_data.cur_map = 0;
            save_data.player_x = 195;
            save_data.player_y = 45;
            out_message = "The WIND ORB howls with celestial power! Warped outside.";
            return true;
        }
    }

    // 7. Ice Cave B3 Evil Eye spike (Map 42, x=16, y=16)
    if (current_map_id_ == 42 && save_data.player_x == 16 && save_data.player_y == 16) {
        if (save_data.key_items_and_flags[QuestFlag::EVIL_EYE_DEFEATED] == 0) {
            out_spike_battle = 0x32; // Evil Eye Spike
            out_message = "Ambushed by the Evil Eye guarding the Levistone!";
            return true;
        }
    }

    // 8. Check 4-Orb Confluence
    if (save_data.orbs_lit[0] && save_data.orbs_lit[1] && save_data.orbs_lit[2] && save_data.orbs_lit[3]) {
        save_data.key_items_and_flags[QuestFlag::FOUR_ORBS_LIT] = 1;
    }

    // 9. Temple of Fiends 1F Black Crystal Time Warp (Map 10, x=16, y=14)
    if (current_map_id_ == 10 && save_data.player_x == 16 && save_data.player_y == 14) {
        if (save_data.key_items_and_flags[QuestFlag::FOUR_ORBS_LIT] != 0) {
            if (save_data.key_items_and_flags[static_cast<size_t>(KeyItem::LUTE)] != 0) {
                if (save_data.key_items_and_flags[QuestFlag::TIME_WARP_UNSEALED] == 0) {
                    save_data.key_items_and_flags[QuestFlag::TIME_WARP_UNSEALED] = 1;
                    load_map(56, MapType::STANDARD_MAP); // ToF Past 1F
                    save_data.cur_map = 56;
                    save_data.player_x = 16;
                    save_data.player_y = 16;
                    out_message = "The LUTE plays a sorrowful melody! The Black Crystal opens the gateway 2000 years into the past!";
                    return true;
                }
            } else {
                out_message = "The Black Crystal resonates... A melody is needed to break the seal.";
            }
        }
    }

    // 10. ToF Past 1F Lich 2 Rematch (Map 56, x=16, y=16)
    if (current_map_id_ == 56 && save_data.player_x == 16 && save_data.player_y == 16) {
        if (save_data.key_items_and_flags[QuestFlag::LICH2_DEFEATED] == 0) {
            out_spike_battle = 0x74; // Lich 2 / Phantom
            out_message = "Lich 2 manifests from the primordial earth!";
            return true;
        }
    }

    // 11. ToF Past 2F Kary 2 Rematch (Map 57, x=16, y=16)
    if (current_map_id_ == 57 && save_data.player_x == 16 && save_data.player_y == 16) {
        if (save_data.key_items_and_flags[QuestFlag::KARY2_DEFEATED] == 0) {
            out_spike_battle = 0x75; // Kary 2
            out_message = "Kary 2 bursts forth in a tempest of ancient flames!";
            return true;
        }
    }

    // 12. ToF Past 3F Kraken 2 Rematch (Map 58, x=16, y=16)
    if (current_map_id_ == 58 && save_data.player_x == 16 && save_data.player_y == 16) {
        if (save_data.key_items_and_flags[QuestFlag::KRAKEN2_DEFEATED] == 0) {
            out_spike_battle = 0x76; // Kraken 2
            out_message = "Kraken 2 rises with the fury of the primordial sea!";
            return true;
        }
    }

    // 13. ToF Past 4F Tiamat 2 Rematch (Map 59, x=16, y=16)
    if (current_map_id_ == 59 && save_data.player_x == 16 && save_data.player_y == 16) {
        if (save_data.key_items_and_flags[QuestFlag::TIAMAT2_DEFEATED] == 0) {
            out_spike_battle = 0x77; // Tiamat 2
            out_message = "Tiamat 2 descends with a hurricane of apocalyptic fury!";
            return true;
        }
    }

    // 14. Chaos Final Boss Spike (Map 60, x=16, y=16)
    if (current_map_id_ == 60 && save_data.player_x == 16 && save_data.player_y == 16) {
        if (save_data.key_items_and_flags[QuestFlag::CHAOS_DEFEATED] == 0) {
            out_spike_battle = 0x78; // Final Boss CHAOS
            out_message = "Garland transforms! CHAOS awakens!";
            return true;
        }
    }

    return false;
}

bool MapEngine::check_event_trigger(GameSaveData& save_data, std::string& out_message) {
    int dummy_battle = -1;
    return check_event_trigger(save_data, out_message, dummy_battle);
}

bool MapEngine::move_player(Direction dir, GameSaveData& save_data, std::string& out_message, int& out_spike_battle) {
    player_facing_ = dir;
    out_message.clear();
    out_spike_battle = -1;

    int dx = 0, dy = 0;
    if (dir == Direction::UP) dy = -1;
    else if (dir == Direction::DOWN) dy = 1;
    else if (dir == Direction::LEFT) dx = -1;
    else if (dir == Direction::RIGHT) dx = 1;

    int target_x = save_data.player_x + dx;
    int target_y = save_data.player_y + dy;

    if (map_type_ == MapType::OVERWORLD) {
        target_x = (target_x + width_) % width_;
        target_y = (target_y + height_) % height_;

        // 1. Boarding the Airship
        if (save_data.vehicle != static_cast<uint8_t>(VehicleType::AIRSHIP)) {
            if (save_data.airship_visible && target_x == save_data.airship_x && target_y == save_data.airship_y) {
                save_data.vehicle = static_cast<uint8_t>(VehicleType::AIRSHIP);
                save_data.player_x = target_x;
                save_data.player_y = target_y;
                out_message = "Boarded the Airship!";
                return true;
            }
        }

        // 2. Boarding the Ship
        if (save_data.vehicle != static_cast<uint8_t>(VehicleType::SHIP) && save_data.vehicle != static_cast<uint8_t>(VehicleType::AIRSHIP)) {
            if (save_data.ship_visible && target_x == save_data.ship_x && target_y == save_data.ship_y) {
                save_data.vehicle = static_cast<uint8_t>(VehicleType::SHIP);
                save_data.player_x = target_x;
                save_data.player_y = target_y;
                out_message = "Boarded the Ship!";
                return true;
            }
        } else if (save_data.vehicle == static_cast<uint8_t>(VehicleType::SHIP)) {
            // 3. Disembarking from the Ship
            uint8_t dest_tile = get_tile_at(target_x, target_y);
            size_t dest_idx = static_cast<size_t>(dest_tile & 0x7F);
            if (dest_idx != 3 && dest_idx != 13) { // Attempting to move onto land
                if (!is_port_tile(target_x, target_y)) {
                    out_message = "The Ship can only dock at a stone port!";
                    return false;
                } else {
                    // Dock ship at current water tile and step on land
                    save_data.ship_x = save_data.player_x;
                    save_data.ship_y = save_data.player_y;
                    save_data.vehicle = static_cast<uint8_t>(VehicleType::WALK);
                    save_data.player_x = target_x;
                    save_data.player_y = target_y;
                    out_message = "Disembarked at the port.";
                    return true;
                }
            }
        }
    }

    check_door_unlock(target_x, target_y, save_data, out_message);

    if (!can_move_to(target_x, target_y, static_cast<VehicleType>(save_data.vehicle), &save_data)) {
        return false;
    }

    if (map_type_ == MapType::OVERWORLD) {
        save_data.player_x = target_x;
        save_data.player_y = target_y;
    } else {
        save_data.player_x = target_x;
        save_data.player_y = target_y;
    }

    check_event_trigger(save_data, out_message, out_spike_battle);

    uint8_t step_tile = get_tile_at(save_data.player_x, save_data.player_y);
    size_t tile_idx = static_cast<size_t>(step_tile & 0x7F);

    if (step_tile == 6) {
        for (auto& hero : save_data.party) {
            if (hero.stats.hp > 1) hero.stats.hp -= 1;
        }
        out_message = "Stepped on lava! -1 HP";
    }

    // Poison step damage on foot
    if (save_data.vehicle == static_cast<uint8_t>(VehicleType::WALK)) {
        bool poison_tick = false;
        for (auto& hero : save_data.party) {
            if (hero.stats.hp > 1 && (hero.status_ailments & Status::POISON)) {
                hero.stats.hp -= 1;
                poison_tick = true;
            }
        }
        if (poison_tick && out_message.empty()) {
            out_message = "Poison damage! -1 HP";
        }
    }

    const auto& bank_00 = loader_.get_chr_bank_00();

    // 1. Check Overworld ROM Teleport Triggers from bank_00.dat ($8000 + tileset_prop)
    if (map_type_ == MapType::OVERWORLD) {
        if (bank_00.size() >= 0x2E00) {
            uint8_t prop1 = bank_00[tile_idx * 2 + 1];
            if (prop1 & 0x80) {
                uint8_t tele_id = prop1 & 0x3F;
                uint8_t target_map = bank_00[0x2C40 + tele_id];
                uint8_t t_x        = bank_00[0x2C00 + tele_id];
                uint8_t t_y        = bank_00[0x2C20 + tele_id];

                load_map(target_map, MapType::STANDARD_MAP);
                save_data.cur_map = target_map;
                save_data.player_x = t_x;
                save_data.player_y = t_y;
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
                    uint8_t t_x        = bank_00[0x2D00 + tele_id];
                    uint8_t t_y        = bank_00[0x2D40 + tele_id];

                    load_map(target_map, MapType::STANDARD_MAP);
                    save_data.cur_map = target_map;
                    save_data.player_x = t_x;
                    save_data.player_y = t_y;
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

bool MapEngine::move_player(Direction dir, GameSaveData& save_data, std::string& out_message) {
    int dummy_battle = -1;
    return move_player(dir, save_data, out_message, dummy_battle);
}

bool MapEngine::check_interaction(GameSaveData& save_data, std::string& out_message, int& out_shop_id, int& out_battle_id) {
    out_shop_id = -1;
    out_battle_id = -1;

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

            // 1. Shopkeeper NPC Trigger
            if (npc.shop_id >= 0) {
                out_shop_id = npc.shop_id;
                return true;
            }

            // 2. Boss / Battle Trigger NPC
            if (npc.battle_id >= 0) {
                out_battle_id = npc.battle_id;
                return true;
            }

            // 3. Quest State Machine Actions
            if (npc.quest_id == 2) { // Garland in ToF
                if (save_data.key_items_and_flags[QuestFlag::SARAH_RESCUED] == 0) {
                    out_battle_id = 0x7F; // BTL_GARLAND
                    out_message = "Garland: I will knock you all down!";
                    return true;
                } else {
                    out_message = "Garland has fallen.";
                    return true;
                }
            } else if (npc.quest_id == 3) { // Sarah in ToF
                save_data.key_items_and_flags[QuestFlag::SARAH_RESCUED] = 1;
                load_map(1, MapType::STANDARD_MAP); // Conelia Castle
                save_data.cur_map = 1;
                save_data.player_x = 16;
                save_data.player_y = 10;
                out_message = "Rescued Princess Sarah! Returned to Conelia Castle.";
                return true;
            } else if (npc.quest_id == 1) { // King of Conelia
                if (save_data.key_items_and_flags[QuestFlag::SARAH_RESCUED] != 0) {
                    save_data.key_items_and_flags[QuestFlag::BRIDGE_BUILT] = 1;
                    out_message = "King: Thank you for saving Sarah! I built the Northern Bridge for you!";
                    return true;
                }
            } else if (npc.quest_id == 0x12) { // Princess Sarah in Castle
                if (save_data.key_items_and_flags[static_cast<size_t>(KeyItem::LUTE)] == 0) {
                    save_data.key_items_and_flags[static_cast<size_t>(KeyItem::LUTE)] = 1;
                    out_message = "Sarah: Please take this LUTE, passed down in our royal family.";
                    return true;
                }
            } else if (npc.quest_id == 4) { // Bikke the Pirate
                if (save_data.key_items_and_flags[QuestFlag::PIRATES_DEFEATED] == 0) {
                    out_battle_id = 0x7E; // BTL_BIKKE
                    out_message = "Bikke: Yarr! Attack them, me hearties!";
                    return true;
                } else {
                    out_message = "Bikke: The Ship is yours, cap'n!";
                    return true;
                }
            } else if (npc.quest_id == 7) { // King / Astos in Western Keep
                if (save_data.key_items_and_flags[QuestFlag::ASTOS_DEFEATED] == 0) {
                    if (save_data.key_items_and_flags[static_cast<size_t>(KeyItem::CROWN)] != 0) {
                        out_battle_id = 0x7D; // BTL_ASTOS
                        out_message = "King: Ha! Fools, I am ASTOS! The Crystal Eye is mine!";
                        return true;
                    }
                }
            } else if (npc.quest_id == 0x0A) { // Witch Matoya
                if (save_data.key_items_and_flags[static_cast<size_t>(KeyItem::CRYSTAL)] != 0) {
                    save_data.key_items_and_flags[static_cast<size_t>(KeyItem::CRYSTAL)] = 0;
                    save_data.key_items_and_flags[static_cast<size_t>(KeyItem::HERB)] = 1;
                    save_data.key_items_and_flags[QuestFlag::MATOYA_HERB_TRADED] = 1;
                    out_message = "Matoya: My Crystal Eye! Take this HERB (Jolt Tonic) in return!";
                    return true;
                }
            } else if (npc.quest_id == 6) { // Elf Prince
                if (save_data.key_items_and_flags[QuestFlag::ELF_PRINCE_AWAKE] == 0) {
                    if (save_data.key_items_and_flags[static_cast<size_t>(KeyItem::HERB)] != 0) {
                        save_data.key_items_and_flags[static_cast<size_t>(KeyItem::HERB)] = 0;
                        save_data.key_items_and_flags[static_cast<size_t>(KeyItem::MYSTIC_KEY)] = 1;
                        save_data.key_items_and_flags[QuestFlag::ELF_PRINCE_AWAKE] = 1;
                        out_message = "The HERB woke the Elf Prince! 'Take this MYSTIC KEY!'";
                        return true;
                    }
                }
            } else if (npc.quest_id == 8) { // Dwarf Nerrick
                if (save_data.key_items_and_flags[QuestFlag::CANAL_DEMOLISHED] == 0) {
                    if (save_data.key_items_and_flags[static_cast<size_t>(KeyItem::TNT)] != 0) {
                        save_data.key_items_and_flags[static_cast<size_t>(KeyItem::TNT)] = 0;
                        save_data.key_items_and_flags[QuestFlag::CANAL_DEMOLISHED] = 1;
                        out_message = "Nerrick: TNT! Watch the canal blast open to the outer sea!";
                        return true;
                    }
                }
            } else if (npc.quest_id == 9) { // Dwarf Smyth
                if (save_data.key_items_and_flags[QuestFlag::EXCALIBUR_FORGED] == 0) {
                    if (save_data.key_items_and_flags[static_cast<size_t>(KeyItem::ADAMANT)] != 0) {
                        save_data.key_items_and_flags[static_cast<size_t>(KeyItem::ADAMANT)] = 0;
                        save_data.key_items_and_flags[QuestFlag::EXCALIBUR_FORGED] = 1;
                        save_data.party[0].weapons[0] = 39; // Excalibur
                        out_message = "Smyth: ADAMANT! Clang clang! Here is the legendary EXCALIBUR!";
                        return true;
                    }
                }
            } else if (npc.quest_id == 0x14) { // Giant Titan
                if (save_data.key_items_and_flags[QuestFlag::TITAN_RUBY_FED] == 0) {
                    if (save_data.key_items_and_flags[static_cast<size_t>(KeyItem::RUBY)] != 0) {
                        save_data.key_items_and_flags[static_cast<size_t>(KeyItem::RUBY)] = 0;
                        save_data.key_items_and_flags[QuestFlag::TITAN_RUBY_FED] = 1;
                        out_message = "Titan: STAR RUBY! Crunch crunch! Delicious! Titan step aside!";
                        return true;
                    }
                }
            } else if (npc.quest_id == 0x0D) { // Sage Sarda
                if (save_data.key_items_and_flags[QuestFlag::SARDA_ROD_OBTAINED] == 0) {
                    save_data.key_items_and_flags[static_cast<size_t>(KeyItem::ROD)] = 1;
                    save_data.key_items_and_flags[QuestFlag::SARDA_ROD_OBTAINED] = 1;
                    out_message = "Sarda: Take this EARTH ROD to break the seal in Earth Cave.";
                    return true;
                }
            } else if (npc.quest_id == 0x15) { // Dr. Unne in Melmond
                if (save_data.key_items_and_flags[QuestFlag::SLAB_TRANSLATED] == 0) {
                    if (save_data.key_items_and_flags[static_cast<size_t>(KeyItem::SLAB)] != 0) {
                        save_data.key_items_and_flags[static_cast<size_t>(KeyItem::SLAB)] = 0;
                        save_data.key_items_and_flags[QuestFlag::SLAB_TRANSLATED] = 1;
                        out_message = "Dr. Unne: The ancient SLAB! I have deciphered and taught you the Lefeinish tongue!";
                        return true;
                    }
                } else {
                    out_message = "Dr. Unne: You can now converse with the Sky People of Lufenia.";
                    return true;
                }
            } else if (npc.quest_id == 0x18) { // Gaia Fairy
                if (save_data.key_items_and_flags[QuestFlag::FAIRY_RELEASED] == 0) {
                    if (save_data.key_items_and_flags[static_cast<size_t>(KeyItem::BOTTLE)] != 0) {
                        save_data.key_items_and_flags[static_cast<size_t>(KeyItem::BOTTLE)] = 0;
                        save_data.key_items_and_flags[static_cast<size_t>(KeyItem::OXYALE)] = 1;
                        save_data.key_items_and_flags[QuestFlag::FAIRY_RELEASED] = 1;
                        out_message = "The Fairy is freed! 'Thank you! Take this OXYALE to breathe underwater!'";
                        return true;
                    }
                } else {
                    out_message = "The Fairy happily splashes in the sacred spring.";
                    return true;
                }
            } else if (npc.quest_id == 0x16) { // Lufenia Elder (Chime)
                if (save_data.key_items_and_flags[QuestFlag::SLAB_TRANSLATED] == 0) {
                    out_message = "Lu-pa-ga-to-mu... (Incomprehensible ancient tongue)";
                    return true;
                } else {
                    if (save_data.key_items_and_flags[QuestFlag::CHIME_OBTAINED] == 0) {
                        save_data.key_items_and_flags[static_cast<size_t>(KeyItem::CHIME)] = 1;
                        save_data.key_items_and_flags[QuestFlag::CHIME_OBTAINED] = 1;
                        out_message = "Lufenia Elder: Take the CHIME to open the Mirage Tower.";
                        return true;
                    } else {
                        out_message = "Lufenia Elder: Use the CHIME to breach the Mirage Tower.";
                        return true;
                    }
                }
            } else if (npc.quest_id == 0x17) { // Lufenia Envoy (Cube)
                if (save_data.key_items_and_flags[QuestFlag::SLAB_TRANSLATED] == 0) {
                    out_message = "Ku-ri-si-ta-ru... (Incomprehensible ancient tongue)";
                    return true;
                } else {
                    if (save_data.key_items_and_flags[QuestFlag::CUBE_OBTAINED] == 0) {
                        save_data.key_items_and_flags[static_cast<size_t>(KeyItem::CUBE)] = 1;
                        save_data.key_items_and_flags[QuestFlag::CUBE_OBTAINED] = 1;
                        out_message = "Lufenia Envoy: Take the WARP CUBE to access our Floating Castle.";
                        return true;
                    } else {
                        out_message = "Lufenia Envoy: Ascend to the Floating Castle and defeat the Fiend of Wind.";
                        return true;
                    }
                }
            } else if (npc.quest_id == 0x78) { // Garland in ToF Past 5F / Sanctum of Chaos
                if (save_data.key_items_and_flags[QuestFlag::CHAOS_DEFEATED] == 0) {
                    out_battle_id = 0x78; // Final Boss CHAOS
                    out_message = "Garland: I am CHAOS! Prepare to be crushed for all eternity!";
                    return true;
                } else {
                    out_message = "Chaos has been defeated. The time loop is broken.";
                    return true;
                }
            }

            return true;
        }
    }

    for (auto& chest : chests_) {
        if (chest.x == facing_x && chest.y == facing_y) {
            if (chest.opened) {
                out_message = "The chest is empty.";
            } else {
                chest.opened = true;
                if (chest.chest_id == 30) {
                    save_data.party[0].weapons[0] = 38; // Masamune
                    save_data.key_items_and_flags[QuestFlag::MASAMUNE_OBTAINED] = 1;
                    out_message = "Opened chest! Found the legendary MASAMUNE!";
                } else if (chest.item_or_gp == 0) {
                    save_data.gold += chest.value;
                    out_message = "Opened chest! Found " + std::to_string(chest.value) + " GP!";
                } else {
                    KeyItem kitem = static_cast<KeyItem>(chest.value);
                    save_data.key_items_and_flags[static_cast<size_t>(kitem)] = 1;
                    if (kitem == KeyItem::CROWN) {
                        save_data.key_items_and_flags[QuestFlag::CROWN_RETRIEVED] = 1;
                    }
                    KeyItemInfo info = get_key_item_info(kitem);
                    out_message = "Opened chest! Found " + info.name + "!";
                }
            }
            return true;
        }
    }

    return false;
}

bool MapEngine::check_interaction(GameSaveData& save_data, std::string& out_message) {
    int dummy_shop = -1;
    int dummy_battle = -1;
    return check_interaction(save_data, out_message, dummy_shop, dummy_battle);
}

bool MapEngine::check_encounter(VehicleType vehicle) {
    if (vehicle == VehicleType::AIRSHIP) return false;
    if (map_type_ == MapType::STANDARD_MAP && (current_map_id_ == 1 || current_map_id_ == 2)) return false;

    uint8_t roll = rng_.next_byte();
    return (roll < 12);
}

} // namespace ff1
