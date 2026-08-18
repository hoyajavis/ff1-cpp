#ifndef MENU_ENGINE_HPP
#define MENU_ENGINE_HPP

#include "data/game_types.hpp"
#include "data/data_loader.hpp"
#include "state/save_system.hpp"
#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace ff1 {

// Forward declarations
class MapEngine;
class AudioEngine;

enum class MenuState {
    CLOSED,
    MAIN_MENU,
    ITEM_MENU,
    ITEM_TARGET_SELECT,
    CAMPING_SAVE_PROMPT,
    MAGIC_MENU,
    MAGIC_TARGET_SELECT,
    EQUIP_MENU,
    STATUS_MENU,
    LINEUP_SELECT,
    WORLD_MAP_SCREEN,
    SHOP
};

enum class EquipTab : uint8_t {
    EQUIP = 0,
    TRADE = 1,
    DROP = 2
};

enum class MenuAction {
    NONE,
    SOUND_MOVE,
    SOUND_SEL,
    SOUND_CANCEL,
    SOUND_CAST,
    CLOSE_MENU,
    SAVE_GAME_TRIGGERED
};

enum class ShopMode {
    NONE,
    BUY_SELL_EXIT,       // [BUY | SELL | EXIT] or [BUY | EXIT]
    BUY_SELECT_ITEM,     // Selecting item/spell from shop inventory
    BUY_CHOOSE_HERO,     // "Who will take it?" (Weapons/Armor) or "Who will learn the spell?" (Magic)
    BUY_CONFIRM,         // [Price] Gold OK? (Yes / No)
    SELL_CHOOSE_HERO,    // "Whose item do you want to sell?"
    SELL_SELECT_ITEM,    // Selecting item from character's equipment
    SELL_CONFIRM,        // [SellPrice] Gold OK? (Yes / No)
    INN_PROMPT,          // [Cost] Gold OK? (Yes / No)
    INN_RESTING,         // Resting animation / sound
    CLINIC_SELECT_HERO,  // "Who shall be revived ...." (Dead characters list)
    CLINIC_CONFIRM       // [ReviveCost] Gold OK? (Yes / No)
};

class MenuEngine {
public:
    explicit MenuEngine(const DataLoader& loader);

    void open_main_menu();
    void open_world_map();
    void close_menu();

    MenuState get_state() const { return current_state_; }
    void set_state(MenuState s) { current_state_ = s; }

    // Navigation Accessors
    uint8_t get_main_cursor() const { return main_cursor_; }
    uint8_t get_char_cursor() const { return char_cursor_; }
    uint8_t get_item_cursor() const { return item_cursor_; }
    uint8_t get_magic_cursor() const { return magic_cursor_; }
    EquipTab get_equip_tab() const { return equip_tab_; }
    uint8_t get_equip_slot_cursor() const { return equip_slot_cursor_; }
    uint8_t get_target_char_cursor() const { return target_char_cursor_; }
    uint8_t get_camping_confirm_cursor() const { return camping_confirm_cursor_; }
    int get_trade_stage() const { return trade_stage_; }
    uint8_t get_trade_src_char() const { return trade_src_char_; }
    uint8_t get_trade_src_slot() const { return trade_src_slot_; }
    uint8_t get_selected_spell_id() const { return selected_spell_id_; }
    uint8_t get_selected_spell_tier() const { return selected_spell_tier_; }

    // Shop Subsystem Accessors
    ShopMode get_shop_mode() const { return shop_mode_; }
    const ShopInventory& get_current_shop() const { return current_shop_; }
    uint8_t get_shop_cursor() const { return shop_cursor_; }
    uint8_t get_shop_sub_cursor() const { return shop_sub_cursor_; }
    uint8_t get_shop_item_idx() const { return shop_item_idx_; }
    uint8_t get_shop_target_hero() const { return shop_target_hero_; }
    const std::string& get_shop_dialogue() const { return shop_dialogue_; }
    uint32_t get_shop_service_cost() const { return shop_service_cost_; }

    std::string get_item_name(ShopType type, uint8_t item_id) const;
    uint32_t get_item_price(ShopType type, uint8_t item_id) const;
    uint32_t get_sell_price(ShopType type, uint8_t item_id) const;

    // Input Handling
    MenuAction handle_input(
        InputKey key,
        GameSaveData& save_data,
        MapEngine& map_engine,
        AudioEngine& audio,
        std::string& out_msg
    );

    // Equipment Manager
    bool equip_weapon(PartyCharacter& hero, uint8_t weapon_slot, uint8_t item_id);
    bool equip_armor(PartyCharacter& hero, uint8_t armor_slot, uint8_t item_id);
    void recalculate_hero_stats(PartyCharacter& hero);
    void toggle_equipment_slot(PartyCharacter& hero, uint8_t slot_idx);
    void trade_equipment_slots(PartyCharacter& hero_a, uint8_t slot_a, PartyCharacter& hero_b, uint8_t slot_b);
    void drop_equipment_slot(PartyCharacter& hero, uint8_t slot_idx);

    // Consumables & Camping Pipeline
    bool use_consumable_potion(GameSaveData& save_data, uint8_t item_idx, uint8_t target_hero_idx, std::string& out_msg);
    bool execute_camping_rest(GameSaveData& save_data, uint8_t camping_type, std::string& out_msg);

    // Field Magic Casting
    bool is_field_spell(uint8_t spell_id) const;
    bool cast_field_spell(PartyCharacter& caster, uint8_t spell_id, uint8_t tier, GameSaveData& save_data, uint8_t target_idx, std::string& out_msg);

    // EXP and Level Math
    static uint32_t get_exp_for_level(uint8_t level);
    static uint32_t get_exp_needed_for_next_level(uint8_t current_level, uint32_t current_exp);

    // Shop Transactions
    void open_shop(uint8_t shop_id, GameSaveData& save_data);
    void open_shop_direct(ShopType type, const std::array<uint8_t, 4>& items, const std::array<uint16_t, 4>& prices, uint32_t service_cost, GameSaveData& save_data);
    bool buy_item(uint8_t item_id, GameSaveData& save_data, uint32_t price);
    bool buy_magic_spell(PartyCharacter& hero, uint8_t spell_id, GameSaveData& save_data, uint32_t price);
    bool rest_at_inn(GameSaveData& save_data, uint32_t inn_price);
    bool revive_at_clinic(PartyCharacter& hero, GameSaveData& save_data, uint32_t clinic_price);

    // Bahamut Class Promotion
    bool promote_party_classes(GameSaveData& save_data, std::string& out_message);

    // Lineup Manager
    void reorder_party_lineup(GameSaveData& save_data, size_t slot_a, size_t slot_b);

private:
    const DataLoader& loader_;
    MenuState current_state_ = MenuState::CLOSED;

    // Cursors
    uint8_t main_cursor_ = 0;          // 0: ITEM, 1: MAGIC, 2: WEAPON, 3: ARMOR, 4: STATUS
    uint8_t char_cursor_ = 0;          // 0..3 party character
    uint8_t item_cursor_ = 0;          // 0..N item index
    uint8_t magic_cursor_ = 0;         // 0..23 (8 tiers x 3 slots)
    EquipTab equip_tab_ = EquipTab::EQUIP;
    uint8_t equip_slot_cursor_ = 0;    // 0..3 weapon, 4..7 armor
    uint8_t target_char_cursor_ = 0;   // 0..3 target party member

    // Camping State
    uint8_t camping_type_ = 0;         // 0: TENT, 1: CABIN, 2: HOUSE
    uint8_t camping_confirm_cursor_ = 0; // 0: YES, 1: NO

    // Trade State
    int trade_stage_ = 0;              // 0: Select src item, 1: Select dest hero & item
    uint8_t trade_src_char_ = 0;
    uint8_t trade_src_slot_ = 0;

    // Magic Casting State
    uint8_t selected_spell_id_ = 0xFF;
    uint8_t selected_spell_tier_ = 0;

    // Lineup Swap State
    int lineup_swap_stage_ = 0;        // 0: Select Hero A, 1: Select Hero B
    uint8_t lineup_src_char_ = 0;

    // Shop Subsystem State
    ShopMode shop_mode_ = ShopMode::NONE;
    ShopInventory current_shop_;
    uint8_t shop_cursor_ = 0;
    uint8_t shop_sub_cursor_ = 0;
    uint8_t shop_item_idx_ = 0;
    uint8_t shop_target_hero_ = 0;
    uint32_t shop_service_cost_ = 0;
    std::string shop_dialogue_ = "";

    MenuAction handle_shop_input(
        InputKey key,
        GameSaveData& save_data,
        AudioEngine& audio,
        std::string& out_msg
    );
};

} // namespace ff1

#endif // MENU_ENGINE_HPP
