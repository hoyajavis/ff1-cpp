#include "data_loader.hpp"
#include "text_decoder.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>

namespace ff1 {

static std::string resolve_asset_path(const std::string& base_path, const std::string& filename) {
    std::vector<std::string> candidates;
    if (!base_path.empty()) {
        candidates.push_back(base_path + "/" + filename);
    }
    candidates.push_back("../FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/bin/" + filename);
    candidates.push_back("../../FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/bin/" + filename);
    candidates.push_back("../../../FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/bin/" + filename);
    candidates.push_back("FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/bin/" + filename);
    candidates.push_back("../FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/" + filename);
    candidates.push_back("../../FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/" + filename);
    candidates.push_back("../../../FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/" + filename);
    candidates.push_back("FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/" + filename);

    // Support cloned Entroper/FF1Disassembly repository paths
    candidates.push_back("../FF1Disassembly/Final Fantasy Disassembly/bin/" + filename);
    candidates.push_back("../../FF1Disassembly/Final Fantasy Disassembly/bin/" + filename);
    candidates.push_back("../../../FF1Disassembly/Final Fantasy Disassembly/bin/" + filename);
    candidates.push_back("FF1Disassembly/Final Fantasy Disassembly/bin/" + filename);
    candidates.push_back("../FF1Disassembly/Final Fantasy Disassembly/" + filename);
    candidates.push_back("../../FF1Disassembly/Final Fantasy Disassembly/" + filename);
    candidates.push_back("../../../FF1Disassembly/Final Fantasy Disassembly/" + filename);
    candidates.push_back("FF1Disassembly/Final Fantasy Disassembly/" + filename);

    try {
        std::filesystem::path cur = std::filesystem::current_path();
        for (int i = 0; i < 5; ++i) {
            std::filesystem::path p1 = cur / "FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/bin" / filename;
            candidates.push_back(p1.string());
            std::filesystem::path p2 = cur / "FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly" / filename;
            candidates.push_back(p2.string());
            std::filesystem::path p3 = cur / "FF1Disassembly/Final Fantasy Disassembly/bin" / filename;
            candidates.push_back(p3.string());
            std::filesystem::path p4 = cur / "FF1Disassembly/Final Fantasy Disassembly" / filename;
            candidates.push_back(p4.string());
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

DataLoader::DataLoader(const std::string& base_path) {
    if (base_path.empty()) {
        base_path_ = "../FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/bin";
    } else {
        base_path_ = base_path;
    }
}

std::vector<uint8_t> DataLoader::read_binary_file(const std::string& filename) {
    std::string resolved = resolve_asset_path(base_path_, filename);
    if (resolved.empty()) {
        std::cerr << "DataLoader: Failed to find file '" << filename << "' in any search path." << std::endl;
        return {};
    }

    std::ifstream file(resolved, std::ios::binary);
    if (!file) {
        std::cerr << "DataLoader: Failed to open file " << resolved << std::endl;
        return {};
    }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

bool DataLoader::load_all() {
    bool ok = true;
    ok &= load_weapons();
    ok &= load_armors();
    ok &= load_magic();
    ok &= load_enemies();
    ok &= load_formations();
    ok &= load_enemy_ai();
    ok &= load_shops();
    load_chr_banks();
    load_palettes_and_overworld();
    return ok;
}

bool DataLoader::load_chr_banks() {
    chr_bank_00_ = read_binary_file("bank_00.dat");
    chr_bank_02_ = read_binary_file("bank_02.dat");
    chr_bank_03_ = read_binary_file("bank_03.dat");
    chr_bank_04_ = read_binary_file("bank_04.dat");
    chr_bank_05_ = read_binary_file("bank_05.dat");
    chr_bank_06_ = read_binary_file("bank_06.dat");
    chr_bank_07_ = read_binary_file("bank_07.dat");
    chr_bank_08_ = read_binary_file("bank_08.dat");
    chr_bank_09_ = read_binary_file("bank_09_data.bin");

    fiend_tsa_ = read_binary_file("0B_92E0_fiendtsa.bin");
    chaos_tsa_ = read_binary_file("0B_9420_chaostsa.bin");

    bridge_cutscene_data_ = read_binary_file("0B_A800_endingbridge_chrnt.bin");
    puzzle_chr_ = read_binary_file("0D_9E00_puzzle_1bpp.chr");
    the_end_draw_data_ = read_binary_file("0D_A000_theenddrawdata.bin");
    the_end_luts_ = read_binary_file("0D_A681_theendluts.bin");

    std::cout << "DataLoader: Loaded CHR Banks 00(" << chr_bank_00_.size()
              << "), 02(" << chr_bank_02_.size()
              << "), 03(" << chr_bank_03_.size()
              << "), 04(" << chr_bank_04_.size()
              << "), 05(" << chr_bank_05_.size()
              << "), 06(" << chr_bank_06_.size()
              << "), 07(" << chr_bank_07_.size()
              << "), 08(" << chr_bank_08_.size()
              << "), 09(" << chr_bank_09_.size() << ") bytes." << std::endl;
    std::cout << "DataLoader: Loaded Cutscenes & Puzzle CHR (" << bridge_cutscene_data_.size()
              << " bridge, " << puzzle_chr_.size() << " puzzle CHR, "
              << the_end_draw_data_.size() << " ending) bytes." << std::endl;
    return !chr_bank_02_.empty();
}

bool DataLoader::load_palettes_and_overworld() {
    auto compressed_ow = read_binary_file("bank_01.bin");
    if (compressed_ow.empty()) {
        compressed_ow = read_binary_file("bank_01_data.bin");
    }
    battle_palettes_ = read_binary_file("0C_8F20_battlepalettes.bin");

    if (compressed_ow.size() >= 512) {
        overworld_map_.assign(256 * 256, 0);

        for (int row = 0; row < 256; ++row) {
            uint16_t ptr = compressed_ow[row * 2] | (compressed_ow[row * 2 + 1] << 8);
            size_t src_offset = (ptr >= 0x8000) ? (ptr - 0x8000) : ptr;

            size_t col = 0;
            while (src_offset < compressed_ow.size() && col < 256) {
                uint8_t byte = compressed_ow[src_offset++];
                if (byte == 0xFF) {
                    break; // End of row
                }
                if (byte & 0x80) { // Run-length encoded
                    uint8_t tile_id = byte & 0x7F;
                    if (src_offset >= compressed_ow.size()) break;
                    uint8_t count = compressed_ow[src_offset++];
                    int run_len = (count == 0) ? 256 : count;
                    for (int k = 0; k < run_len && col < 256; ++k) {
                        overworld_map_[row * 256 + col++] = tile_id;
                    }
                } else { // Single tile
                    overworld_map_[row * 256 + col++] = byte;
                }
            }
        }
    }

    if (chr_bank_00_.size() >= 0x3300) {
        // Mapman palettes at 0x03A0 ($83A0)
        size_t mapman_off = 0x03A0;
        mapman_palettes_.assign(chr_bank_00_.begin() + mapman_off, chr_bank_00_.begin() + mapman_off + 64);

        // SM palettes at 0x2000 ($A000)
        size_t sm_pal_off = 0x2000;
        size_t sm_pal_size = std::min<size_t>(chr_bank_00_.size() - sm_pal_off, 64 * 48);
        sm_palettes_.assign(chr_bank_00_.begin() + sm_pal_off, chr_bank_00_.begin() + sm_pal_off + sm_pal_size);

        // Backdrop palettes at 0x3200 ($B200)
        size_t bd_off = 0x3200;
        size_t bd_size = std::min<size_t>(chr_bank_00_.size() - bd_off, 32);
        backdrop_palettes_.assign(chr_bank_00_.begin() + bd_off, chr_bank_00_.begin() + bd_off + bd_size);

        // Tileset assignments at 0x2CC0 ($ACC0)
        size_t ts_off = 0x2CC0;
        size_t ts_size = std::min<size_t>(chr_bank_00_.size() - ts_off, 64);
        tileset_assignments_.assign(chr_bank_00_.begin() + ts_off, chr_bank_00_.begin() + ts_off + ts_size);
    }
    std::cout << "DataLoader: Decompressed Overworld map (" << overworld_map_.size() << " bytes), "
              << "SM Palettes (" << sm_palettes_.size() << " bytes), "
              << "Mapman Palettes (" << mapman_palettes_.size() << " bytes)." << std::endl;
    return !overworld_map_.empty();
}

bool DataLoader::load_weapons() {
    auto data = read_binary_file("0C_8000_weapondata.bin");
    if (data.size() < 320) return false;

    weapons_.clear();
    for (size_t i = 0; i < 40; ++i) {
        size_t offset = i * 8;
        WeaponData w;
        w.hit_rate   = data[offset + 0];
        w.damage     = data[offset + 1];
        w.crit_rate  = data[offset + 2];
        w.spell_cast = data[offset + 3];
        w.element    = data[offset + 4];
        w.category   = data[offset + 5];
        w.graphic    = data[offset + 6];
        w.palette    = data[offset + 7];
        weapons_.push_back(w);
    }
    return true;
}

bool DataLoader::load_armors() {
    auto data = read_binary_file("0C_8140_armordata.bin");
    if (data.empty()) return false;

    armors_.clear();
    size_t count = data.size() / 8;
    if (count == 0) count = data.size() / 4;
    size_t stride = data.size() / count;

    for (size_t i = 0; i < count; ++i) {
        size_t offset = i * stride;
        ArmorData a;
        a.evade_penalty = data[offset + 0];
        a.absorb        = data[offset + 1];
        a.element_def   = data[offset + 2];
        a.spell_cast    = data[offset + 3];
        armors_.push_back(a);
    }
    return true;
}

bool DataLoader::load_magic() {
    auto data = read_binary_file("0C_81E0_magicdata.bin");
    if (data.size() < 512) return false;

    magic_.clear();
    for (size_t i = 0; i < 64; ++i) {
        size_t offset = i * 8;
        MagicData m;
        m.hit_rate    = data[offset + 0];
        m.effectivity = data[offset + 1];
        m.element     = data[offset + 2];
        m.target      = data[offset + 3];
        m.effect      = data[offset + 4];
        m.graphic     = data[offset + 5];
        m.palette     = data[offset + 6];
        m.unused      = data[offset + 7];
        magic_.push_back(m);
    }
    return true;
}

bool DataLoader::load_enemies() {
    auto data = read_binary_file("0C_8520_enemydata.bin");
    auto names_data = read_binary_file("0B_94E0_enemynames.bin");
    if (data.size() < 2560) return false;

    enemies_.clear();
    for (size_t i = 0; i < 128; ++i) {
        size_t offset = i * 20;
        EnemyData e;
        e.exp           = data[offset + 0] | (data[offset + 1] << 8);
        e.gp            = data[offset + 2] | (data[offset + 3] << 8);
        e.hp_max        = data[offset + 4] | (data[offset + 5] << 8);
        e.morale        = data[offset + 6];
        e.ai_id         = data[offset + 7];
        e.evade         = data[offset + 8];
        e.absorb        = data[offset + 9];
        e.num_hits      = data[offset + 10];
        e.hit_rate      = data[offset + 11];
        e.damage        = data[offset + 12];
        e.crit_rate     = data[offset + 13];
        e.attack_ailment = data[offset + 15];
        e.category      = data[offset + 16];
        e.mag_def       = data[offset + 17];
        e.elem_weak     = data[offset + 18];
        e.elem_resist   = data[offset + 19];

        if (names_data.size() >= (i + 1) * 8) {
            e.name = TextDecoder::decode_string(&names_data[i * 8], 8);
        } else {
            e.name = "ENEM" + std::to_string(i);
        }

        enemies_.push_back(e);
    }
    return true;
}

bool DataLoader::load_formations() {
    auto data = read_binary_file("0B_8400_battleformations.bin");
    if (data.size() < 2048) return false;

    formations_.clear();
    for (size_t i = 0; i < 128; ++i) {
        size_t offset = i * 16;
        BattleFormation f;
        f.battle_type   = (data[offset + 0] >> 4) & 0x0F;
        f.pattern_sel   = data[offset + 0] & 0x0F;
        f.pic_assign    = data[offset + 1];
        f.enemy_ids     = {data[offset + 2], data[offset + 3], data[offset + 4], data[offset + 5]};
        f.min_max_a     = {data[offset + 6], data[offset + 7], data[offset + 8], data[offset + 9]};
        f.palette_id    = {data[offset + 10], data[offset + 11]};
        f.surprised_rate = data[offset + 12];
        f.palette_assign = (data[offset + 13] >> 4) & 0x0F;
        f.no_run        = (data[offset + 13] & 0x01) != 0;
        f.min_max_b     = {data[offset + 14], data[offset + 15]};
        formations_.push_back(f);
    }
    return true;
}

bool DataLoader::load_enemy_ai() {
    auto data = read_binary_file("0C_9020_aidata.bin");
    if (data.empty()) return false;

    ai_scripts_.clear();
    size_t count = data.size() / 16;
    for (size_t i = 0; i < count; ++i) {
        size_t offset = i * 16;
        EnemyAIData ai;
        ai.spell_chance = data[offset + 0];
        ai.skill_chance = data[offset + 1];

        for (int s = 0; s < 8; ++s) {
            ai.spell_list[s] = data[offset + 2 + s];
        }
        for (int k = 0; k < 4; ++k) {
            ai.skill_list[k] = data[offset + 10 + k];
        }
        ai_scripts_.push_back(ai);
    }
    return true;
}

bool DataLoader::load_shops() {
    auto data = read_binary_file("0E_8300_shopdata.bin");
    if (data.empty()) return false;

    shops_.clear();
    size_t count = data.size() / 8;
    for (size_t i = 0; i < count; ++i) {
        size_t offset = i * 8;
        ShopInventory s;
        s.shop_id = static_cast<uint8_t>(i);
        s.type = (i % 5 == 0) ? ShopType::WEAPON : (i % 5 == 1 ? ShopType::ARMOR : (i % 5 == 2 ? ShopType::WHITE_MAGIC : ShopType::BLACK_MAGIC));
        s.items = {data[offset + 0], data[offset + 1], data[offset + 2], data[offset + 3]};
        s.prices = {
            static_cast<uint16_t>(data[offset + 4] * 10),
            static_cast<uint16_t>(data[offset + 5] * 10),
            static_cast<uint16_t>(data[offset + 6] * 10),
            static_cast<uint16_t>(data[offset + 7] * 10)
        };
        shops_.push_back(s);
    }
    return true;
}

const WeaponData& DataLoader::get_weapon(uint8_t id) const {
    static WeaponData empty;
    if (id < weapons_.size()) return weapons_[id];
    return empty;
}

const ArmorData& DataLoader::get_armor(uint8_t id) const {
    static ArmorData empty;
    if (id < armors_.size()) return armors_[id];
    return empty;
}

const MagicData& DataLoader::get_spell(uint8_t id) const {
    static MagicData empty;
    if (id < magic_.size()) return magic_[id];
    return empty;
}

const EnemyData& DataLoader::get_enemy(uint8_t id) const {
    static EnemyData empty;
    if (id < enemies_.size()) return enemies_[id];
    return empty;
}

const BattleFormation& DataLoader::get_formation(uint8_t id) const {
    static BattleFormation empty;
    if (id < formations_.size()) return formations_[id];
    return empty;
}

const EnemyAIData& DataLoader::get_ai_script(uint8_t id) const {
    static EnemyAIData empty;
    if (id < ai_scripts_.size()) return ai_scripts_[id];
    return empty;
}

const ShopInventory& DataLoader::get_shop(uint8_t id) const {
    static ShopInventory empty;
    if (id < shops_.size()) return shops_[id];
    return empty;
}

std::array<uint8_t, 4> DataLoader::get_overworld_palette(uint8_t pal_attr) const {
    size_t pal_idx = (pal_attr & 0x03) * 4;
    if (chr_bank_00_.size() >= 0x0380 + pal_idx + 4) {
        return {
            chr_bank_00_[0x0380 + pal_idx + 0],
            chr_bank_00_[0x0380 + pal_idx + 1],
            chr_bank_00_[0x0380 + pal_idx + 2],
            chr_bank_00_[0x0380 + pal_idx + 3]
        };
    }
    switch (pal_attr & 0x03) {
        case 0: return {0x0F, 0x1A, 0x10, 0x30};
        case 1: return {0x0F, 0x1A, 0x27, 0x37};
        case 2: return {0x0F, 0x1A, 0x31, 0x21};
        case 3: return {0x0F, 0x1A, 0x29, 0x19};
        default: return {0x0F, 0x1A, 0x10, 0x30};
    }
}

std::array<uint8_t, 4> DataLoader::get_standard_map_palette(uint8_t map_id, uint8_t pal_attr) const {
    size_t pal_idx = (pal_attr & 0x03) * 4;
    size_t map_off = static_cast<size_t>(map_id) * 48;
    if (map_off + pal_idx + 4 <= sm_palettes_.size()) {
        return {
            sm_palettes_[map_off + pal_idx + 0],
            sm_palettes_[map_off + pal_idx + 1],
            sm_palettes_[map_off + pal_idx + 2],
            sm_palettes_[map_off + pal_idx + 3]
        };
    }
    switch (pal_attr & 0x03) {
        case 0: return {0x0F, 0x30, 0x10, 0x00};
        case 1: return {0x0F, 0x30, 0x16, 0x27};
        case 2: return {0x0F, 0x30, 0x12, 0x22};
        case 3: return {0x0F, 0x30, 0x17, 0x07};
        default: return {0x0F, 0x30, 0x10, 0x00};
    }
}

std::array<uint8_t, 4> DataLoader::get_player_palette(ClassType char_class, uint8_t palette_sub_index) const {
    size_t class_idx = static_cast<size_t>(char_class) % 12;
    uint8_t c0 = 0x16;
    uint8_t c1 = 0x16;
    if (mapman_palettes_.size() >= (class_idx * 2 + 2)) {
        c0 = mapman_palettes_[class_idx * 2 + 0];
        c1 = mapman_palettes_[class_idx * 2 + 1];
    } else {
        switch (char_class) {
            case ClassType::WARRIOR:
            case ClassType::KNIGHT:       c0 = 0x16; c1 = 0x16; break;
            case ClassType::THIEF:
            case ClassType::NINJA:        c0 = 0x12; c1 = 0x17; break;
            case ClassType::BLACK_BELT:
            case ClassType::MASTER:       c0 = 0x27; c1 = 0x12; break;
            case ClassType::RED_MAGE:
            case ClassType::RED_WIZARD:   c0 = 0x16; c1 = 0x16; break;
            case ClassType::WHITE_MAGE:   c0 = 0x30; c1 = 0x30; break;
            case ClassType::WHITE_WIZARD: c0 = 0x16; c1 = 0x30; break;
            case ClassType::BLACK_MAGE:   c0 = 0x27; c1 = 0x12; break;
            case ClassType::BLACK_WIZARD: c0 = 0x27; c1 = 0x13; break;
            default:                      c0 = 0x16; c1 = 0x16; break;
        }
    }
    uint8_t chosen_c = (palette_sub_index == 1) ? c1 : c0;
    return {0x0F, 0x30, chosen_c, 0x0F};
}

std::array<uint8_t, 4> DataLoader::get_monster_palette(uint8_t palette_id) const {
    size_t off = static_cast<size_t>(palette_id) * 4;
    if (off + 4 <= battle_palettes_.size()) {
        return {
            battle_palettes_[off + 0],
            battle_palettes_[off + 1],
            battle_palettes_[off + 2],
            battle_palettes_[off + 3]
        };
    }
    return {0x0F, 0x16, 0x30, 0x05};
}

std::vector<uint8_t> DataLoader::decompress_standard_map(uint8_t map_id) const {
    if (chr_bank_04_.empty() || map_id >= 64) return {};

    size_t ptr_idx = static_cast<size_t>(map_id) * 2;
    if (ptr_idx + 1 >= chr_bank_04_.size()) return {};

    uint16_t raw_ptr = chr_bank_04_[ptr_idx] | (chr_bank_04_[ptr_idx + 1] << 8);
    uint8_t bank_idx = (raw_ptr >> 14) & 0x03;
    size_t offset = raw_ptr & 0x3FFF;

    // Combine standard map banks 04, 05, 06 into a contiguous address space
    std::vector<uint8_t> map_pool;
    map_pool.reserve(chr_bank_04_.size() + chr_bank_05_.size() + chr_bank_06_.size());
    map_pool.insert(map_pool.end(), chr_bank_04_.begin(), chr_bank_04_.end());
    map_pool.insert(map_pool.end(), chr_bank_05_.begin(), chr_bank_05_.end());
    map_pool.insert(map_pool.end(), chr_bank_06_.begin(), chr_bank_06_.end());

    size_t curr = bank_idx * 0x4000 + offset;
    std::vector<uint8_t> out;
    out.reserve(4096);

    while (curr < map_pool.size() && out.size() < 4096) {
        uint8_t b = map_pool[curr++];
        if (b == 0xFF) break;
        if (b < 0x80) {
            out.push_back(b);
        } else {
            uint8_t tile = b & 0x7F;
            if (curr >= map_pool.size()) break;
            uint8_t len = map_pool[curr++];
            int count = (len == 0) ? 256 : len;
            out.insert(out.end(), count, tile);
        }
    }
    return out;
}

} // namespace ff1
