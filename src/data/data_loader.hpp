#ifndef DATA_LOADER_HPP
#define DATA_LOADER_HPP

#include "game_types.hpp"
#include <string>
#include <vector>

namespace ff1 {

class DataLoader {
public:
    DataLoader(const std::string& base_path = "");

    bool load_all();

    const std::vector<WeaponData>& get_weapons() const { return weapons_; }
    const std::vector<ArmorData>& get_armors() const { return armors_; }
    const std::vector<MagicData>& get_magic() const { return magic_; }
    const std::vector<EnemyData>& get_enemies() const { return enemies_; }
    const std::vector<BattleFormation>& get_formations() const { return formations_; }
    const std::vector<EnemyAIData>& get_ai_scripts() const { return ai_scripts_; }
    const std::vector<ShopInventory>& get_shops() const { return shops_; }

    const std::vector<uint8_t>& get_chr_bank_00() const { return chr_bank_00_; }
    const std::vector<uint8_t>& get_chr_bank_02() const { return chr_bank_02_; }
    const std::vector<uint8_t>& get_chr_bank_03() const { return chr_bank_03_; }
    const std::vector<uint8_t>& get_chr_bank_04() const { return chr_bank_04_; }
    const std::vector<uint8_t>& get_chr_bank_05() const { return chr_bank_05_; }
    const std::vector<uint8_t>& get_chr_bank_06() const { return chr_bank_06_; }
    const std::vector<uint8_t>& get_chr_bank_07() const { return chr_bank_07_; }
    const std::vector<uint8_t>& get_chr_bank_08() const { return chr_bank_08_; }
    const std::vector<uint8_t>& get_chr_bank_09() const { return chr_bank_09_; }

    std::vector<uint8_t> decompress_standard_map(uint8_t map_id) const;

    const std::vector<uint8_t>& get_fiend_tsa() const { return fiend_tsa_; }
    const std::vector<uint8_t>& get_chaos_tsa() const { return chaos_tsa_; }

    const std::vector<uint8_t>& get_bridge_cutscene_data() const { return bridge_cutscene_data_; }
    const std::vector<uint8_t>& get_puzzle_chr() const { return puzzle_chr_; }
    const std::vector<uint8_t>& get_ending_draw_data() const { return the_end_draw_data_; }
    const std::vector<uint8_t>& get_ending_luts() const { return the_end_luts_; }

    const std::vector<uint8_t>& get_overworld_map() const { return overworld_map_; }
    const std::vector<uint8_t>& get_sm_palettes() const { return sm_palettes_; }
    const std::vector<uint8_t>& get_mapman_palettes() const { return mapman_palettes_; }
    const std::vector<uint8_t>& get_backdrop_palettes() const { return backdrop_palettes_; }
    const std::vector<uint8_t>& get_battle_palettes() const { return battle_palettes_; }
    const std::vector<uint8_t>& get_tileset_assignments() const { return tileset_assignments_; }

    std::array<uint8_t, 4> get_overworld_palette(uint8_t pal_attr) const;
    std::array<uint8_t, 4> get_standard_map_palette(uint8_t map_id, uint8_t pal_attr) const;
    std::array<uint8_t, 4> get_player_palette(ClassType char_class, uint8_t palette_sub_index = 0) const;
    std::array<uint8_t, 4> get_monster_palette(uint8_t palette_id) const;

    const WeaponData& get_weapon(uint8_t id) const;
    const ArmorData& get_armor(uint8_t id) const;
    const MagicData& get_spell(uint8_t id) const;
    const EnemyData& get_enemy(uint8_t id) const;
    const BattleFormation& get_formation(uint8_t id) const;
    const EnemyAIData& get_ai_script(uint8_t id) const;
    const ShopInventory& get_shop(uint8_t id) const;

private:
    std::string base_path_;

    std::vector<WeaponData> weapons_;
    std::vector<ArmorData> armors_;
    std::vector<MagicData> magic_;
    std::vector<EnemyData> enemies_;
    std::vector<BattleFormation> formations_;
    std::vector<EnemyAIData> ai_scripts_;
    std::vector<ShopInventory> shops_;

    std::vector<uint8_t> chr_bank_00_;
    std::vector<uint8_t> chr_bank_02_;
    std::vector<uint8_t> chr_bank_03_;
    std::vector<uint8_t> chr_bank_04_;
    std::vector<uint8_t> chr_bank_05_;
    std::vector<uint8_t> chr_bank_06_;
    std::vector<uint8_t> chr_bank_07_;
    std::vector<uint8_t> chr_bank_08_;
    std::vector<uint8_t> chr_bank_09_;

    std::vector<uint8_t> fiend_tsa_;
    std::vector<uint8_t> chaos_tsa_;

    std::vector<uint8_t> bridge_cutscene_data_;
    std::vector<uint8_t> puzzle_chr_;
    std::vector<uint8_t> the_end_draw_data_;
    std::vector<uint8_t> the_end_luts_;

    std::vector<uint8_t> overworld_map_;
    std::vector<uint8_t> sm_palettes_;
    std::vector<uint8_t> mapman_palettes_;
    std::vector<uint8_t> backdrop_palettes_;
    std::vector<uint8_t> battle_palettes_;
    std::vector<uint8_t> tileset_assignments_;

    bool load_weapons();
    bool load_armors();
    bool load_magic();
    bool load_enemies();
    bool load_formations();
    bool load_enemy_ai();
    bool load_shops();
    bool load_chr_banks();
    bool load_palettes_and_overworld();

    std::vector<uint8_t> read_binary_file(const std::string& filename);
};

} // namespace ff1

#endif // DATA_LOADER_HPP
