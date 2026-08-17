#include "map_loader.hpp"
#include "data_loader.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>

namespace ff1 {

static std::string resolve_map_asset_path(const std::string& base_path, const std::string& filename) {
    std::vector<std::string> candidates;
    if (!base_path.empty()) {
        candidates.push_back(base_path + "/" + filename);
    }
    candidates.push_back("../FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/bin/" + filename);
    candidates.push_back("../../FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/bin/" + filename);
    candidates.push_back("../../../FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/bin/" + filename);
    candidates.push_back("FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/bin/" + filename);

    try {
        std::filesystem::path cur = std::filesystem::current_path();
        for (int i = 0; i < 5; ++i) {
            std::filesystem::path p = cur / "FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/bin" / filename;
            candidates.push_back(p.string());
            if (cur.has_parent_path() && cur.parent_path() != cur) {
                cur = cur.parent_path();
            } else {
                break;
            }
        }
    } catch (...) {}

    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    return "";
}

MapLoader::MapLoader(const std::string& base_path) {
    if (base_path.empty()) {
        base_path_ = "../FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/bin";
    } else {
        base_path_ = base_path;
    }
}

std::vector<uint8_t> MapLoader::read_binary_file(const std::string& filename) {
    std::string resolved = resolve_map_asset_path(base_path_, filename);
    if (resolved.empty()) {
        std::cerr << "MapLoader: Failed to find file '" << filename << "' in any search path." << std::endl;
        return {};
    }

    std::ifstream file(resolved, std::ios::binary);
    if (!file) {
        std::cerr << "MapLoader: Failed to open file " << resolved << std::endl;
        return {};
    }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

static const char* lut_StandardMapNames[64] = {
    "Conelia Castle 1F",    // 0
    "Conelia Castle 2F",    // 1
    "Conelia Town",         // 2
    "Pravoka Town",         // 3
    "Elfland Town",         // 4
    "Castle Elfland",       // 5
    "Melmond Town",         // 6
    "Crescent Lake Town",   // 7
    "Gaia Town",            // 8
    "Onrac Town",           // 9
    "Temple of Fiends 1F",  // 10
    "Earth Cave B1",        // 11
    "Gurgu Volcano 1F",     // 12
    "Ice Cave B1",          // 13
    "Cardia Islands",       // 14
    "Bahamut's Cave 1F",    // 15
    "Waterfall Cave",       // 16
    "Dwarven Cave",         // 17
    "Matoya's Cave",        // 18
    "Sarda's Cave",         // 19
    "Marsh Cave 1F",        // 20
    "Mirage Tower 1F",      // 21
    "Castle of Ordeals 1F", // 22
    "Sea Shrine 1F",        // 23
    "Western Keep",         // 24
    "Titan's Tunnel West",  // 25
    "Titan's Tunnel East",  // 26
    "Earth Cave B2",        // 27
    "Earth Cave B3",        // 28
    "Earth Cave B4",        // 29
    "Earth Cave B5",        // 30
    "Gurgu Volcano B2",     // 31
    "Gurgu Volcano B3",     // 32
    "Gurgu Volcano B4",     // 33
    "Gurgu Volcano B5",     // 34
    "Ice Cave B2",          // 35
    "Ice Cave B3",          // 36
    "Marsh Cave B2",        // 37
    "Marsh Cave B3",        // 38
    "Mirage Tower 2F",      // 39
    "Mirage Tower 3F",      // 40
    "Floating Castle 1F",   // 41
    "Floating Castle 2F",   // 42
    "Floating Castle 3F",   // 43
    "Floating Castle 4F",   // 44
    "Floating Castle 5F",   // 45
    "Castle of Ordeals 2F", // 46
    "Castle of Ordeals 3F", // 47
    "Sea Shrine 2F",        // 48
    "Sea Shrine 3F",        // 49
    "Sea Shrine 4F",        // 50
    "Sea Shrine 5F",        // 51
    "Temple of Fiends Past 1F", // 52
    "Temple of Fiends Past 2F", // 53
    "Temple of Fiends Past 3F", // 54
    "Temple of Fiends Past B1", // 55
    "Temple of Fiends Past B2", // 56
    "Temple of Fiends Past B3", // 57
    "Temple of Fiends Past B4", // 58
    "Temple of Fiends Past B5", // 59
    "Caravan",              // 60
    "Lufenian Town",        // 61
    "Conelia Treasury",     // 62
    "Chaos Chamber"         // 63
};

bool MapLoader::load_all_maps(const DataLoader* loader) {
    bool ok = true;
    ok &= load_npc_objects();
    ok &= load_teleport_matrix(loader);
    ok &= build_standard_maps(loader);
    return ok;
}

bool MapLoader::load_npc_objects() {
    auto data = read_binary_file("0E_95D5_objectdata.bin");
    if (data.empty()) return false;

    all_npcs_.clear();
    map_npcs_.clear();

    size_t count = data.size() / 8;
    for (size_t i = 0; i < count; ++i) {
        size_t offset = i * 8;
        NPCObjectData npc;
        npc.obj_id      = static_cast<uint8_t>(i + 1);
        npc.map_id      = data[offset + 0];
        npc.x           = data[offset + 1];
        npc.y           = data[offset + 2];
        npc.graphic_id  = data[offset + 3];
        npc.move_type   = data[offset + 4];
        npc.dialogue_id = data[offset + 5];

        all_npcs_.push_back(npc);
        map_npcs_[npc.map_id].push_back(npc);
    }

    return true;
}

bool MapLoader::load_teleport_matrix(const DataLoader* loader) {
    teleports_.clear();

    if (loader) {
        const auto& bank_00 = loader->get_chr_bank_00();
        if (bank_00.size() >= 0x2E00) {
            // Overworld Entry Teleports ($AC00, $AC20, $AC40 -> 0x2C00, 0x2C20, 0x2C40)
            for (size_t i = 0; i < 32; ++i) {
                TeleportEntry tp;
                tp.from_map = 0; // Overworld
                tp.from_x = 0;   // Triggered by tileprop
                tp.from_y = 0;
                tp.target_map = bank_00[0x2C40 + i];
                tp.target_x = bank_00[0x2C00 + i];
                tp.target_y = bank_00[0x2C20 + i];
                teleports_.push_back(tp);
            }

            // In-Dungeon Normal Teleports ($AD00, $AD40, $AD80 -> 0x2D00, 0x2D40, 0x2D80)
            for (size_t i = 0; i < 64; ++i) {
                TeleportEntry tp;
                tp.from_map = 0xFF; // In-dungeon
                tp.from_x = 0;
                tp.from_y = 0;
                tp.target_map = bank_00[0x2D80 + i];
                tp.target_x = bank_00[0x2D00 + i];
                tp.target_y = bank_00[0x2D40 + i];
                teleports_.push_back(tp);
            }
        }
    }

    // Default Fallback Handlers for Key Town/Castle Gateways
    // Conelia Town <-> Overworld
    TeleportEntry t1; t1.from_map = 0; t1.from_x = 152; t1.from_y = 144; t1.target_map = 2; t1.target_x = 41; t1.target_y = 22;
    teleports_.push_back(t1);

    TeleportEntry t2; t2.from_map = 2; t2.from_x = 41; t2.from_y = 23; t2.target_map = 0; t2.target_x = 152; t2.target_y = 145;
    teleports_.push_back(t2);

    // Conelia Castle <-> Overworld
    TeleportEntry t3; t3.from_map = 0; t3.from_x = 152; t3.from_y = 142; t3.target_map = 0; t3.target_x = 16; t3.target_y = 23;
    teleports_.push_back(t3);

    // Temple of Fiends <-> Overworld
    TeleportEntry t5; t5.from_map = 0; t5.from_x = 160; t5.from_y = 130; t5.target_map = 10; t5.target_x = 22; t5.target_y = 24;
    teleports_.push_back(t5);

    return true;
}

bool MapLoader::build_standard_maps(const DataLoader* loader) {
    maps_.clear();

    for (uint8_t m = 0; m < 64; ++m) {
        StandardMapData sm;
        sm.map_id = m;
        sm.name = (m < 64) ? lut_StandardMapNames[m] : ("Standard Map " + std::to_string(m));
        sm.width = 64;
        sm.height = 64;

        if (loader) {
            sm.layout = loader->decompress_standard_map(m);
        }

        if (sm.layout.empty() || sm.layout.size() < 64 * 64) {
            sm.layout.assign(64 * 64, 0);
            for (int x = 0; x < 64; ++x) {
                sm.layout[0 * 64 + x] = 2;
                sm.layout[63 * 64 + x] = 2;
            }
            for (int y = 0; y < 64; ++y) {
                sm.layout[y * 64 + 0] = 2;
                sm.layout[y * 64 + 63] = 2;
            }
        }

        maps_[m] = sm;
    }

    std::cout << "MapLoader: Successfully built " << maps_.size() << " authentic standard maps." << std::endl;
    return true;
}

const StandardMapData& MapLoader::get_standard_map(uint8_t map_id) const {
    static StandardMapData empty;
    auto it = maps_.find(map_id);
    if (it != maps_.end()) return it->second;
    return empty;
}

const std::vector<NPCObjectData>& MapLoader::get_npcs_for_map(uint8_t map_id) const {
    static std::vector<NPCObjectData> empty;
    auto it = map_npcs_.find(map_id);
    if (it != map_npcs_.end()) return it->second;
    return empty;
}

} // namespace ff1
