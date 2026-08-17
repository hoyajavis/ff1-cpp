#ifndef MENU_ENGINE_HPP
#define MENU_ENGINE_HPP

#include "data/game_types.hpp"
#include "data/data_loader.hpp"
#include "state/save_system.hpp"
#include <string>

namespace ff1 {

enum class MenuState {
    CLOSED,
    MAIN_MENU,
    STATUS,
    EQUIPMENT,
    MAGIC,
    ITEMS,
    LINEUP,
    SHOP
};

class MenuEngine {
public:
    MenuEngine(const DataLoader& loader);

    void open_main_menu();
    void close_menu();

    MenuState get_state() const { return current_state_; }

    // Equipment Manager
    bool equip_weapon(PartyCharacter& hero, uint8_t weapon_slot, uint8_t item_id);
    bool equip_armor(PartyCharacter& hero, uint8_t armor_slot, uint8_t item_id);
    void recalculate_hero_stats(PartyCharacter& hero);

    // Shop Transactions
    void open_shop(uint8_t shop_id, GameSaveData& save_data);
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
};

} // namespace ff1

#endif // MENU_ENGINE_HPP
