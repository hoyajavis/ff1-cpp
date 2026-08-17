#include "menu_engine.hpp"
#include <algorithm>

namespace ff1 {

MenuEngine::MenuEngine(const DataLoader& loader) : loader_(loader) {}

void MenuEngine::open_main_menu() {
    current_state_ = MenuState::MAIN_MENU;
}

void MenuEngine::close_menu() {
    current_state_ = MenuState::CLOSED;
}

void MenuEngine::recalculate_hero_stats(PartyCharacter& hero) {
    // Reset to base level stats
    int total_absorb = 0;
    int total_evade_penalty = 0;
    int bonus_damage = 0;
    int bonus_hit = 0;
    int weapon_crit = 0;

    // Check equipped weapons
    for (uint8_t w_id : hero.weapons) {
        if (w_id != 0xFF) {
            const auto& wdata = loader_.get_weapon(w_id);
            bonus_damage += wdata.damage;
            bonus_hit += wdata.hit_rate;
            weapon_crit = std::max<int>(weapon_crit, wdata.crit_rate);
        }
    }

    // Check equipped armors
    for (uint8_t a_id : hero.armors) {
        if (a_id != 0xFF) {
            const auto& adata = loader_.get_armor(a_id);
            total_absorb += adata.absorb;
            total_evade_penalty += adata.evade_penalty;
        }
    }

    // Black Belt / Master special unarmed absorb rule
    if (hero.char_class == ClassType::BLACK_BELT || hero.char_class == ClassType::MASTER) {
        bool no_armor = std::all_of(hero.armors.begin(), hero.armors.end(), [](uint8_t id) { return id == 0xFF; });
        if (no_armor) {
            total_absorb = hero.level;
        }
    }

    hero.stats.absorb = total_absorb;
    hero.stats.evade = std::max(0, 48 + hero.stats.agility - total_evade_penalty);
    hero.stats.damage = (hero.stats.strength / 2) + bonus_damage;
    hero.stats.hit_rate = 5 + (hero.level * 3) + bonus_hit;
    hero.stats.crit_rate = weapon_crit;
}

bool MenuEngine::equip_weapon(PartyCharacter& hero, uint8_t weapon_slot, uint8_t item_id) {
    if (weapon_slot >= 4) return false;
    hero.weapons[weapon_slot] = item_id;
    recalculate_hero_stats(hero);
    return true;
}

bool MenuEngine::equip_armor(PartyCharacter& hero, uint8_t armor_slot, uint8_t item_id) {
    if (armor_slot >= 4) return false;
    hero.armors[armor_slot] = item_id;
    recalculate_hero_stats(hero);
    return true;
}

void MenuEngine::open_shop(uint8_t shop_id, GameSaveData& save_data) {
    (void)shop_id;
    (void)save_data;
    current_state_ = MenuState::SHOP;
}

bool MenuEngine::buy_item(uint8_t item_id, GameSaveData& save_data, uint32_t price) {
    (void)item_id;
    if (save_data.gold >= price) {
        save_data.gold -= price;
        return true;
    }
    return false;
}

bool MenuEngine::buy_magic_spell(PartyCharacter& hero, uint8_t spell_id, GameSaveData& save_data, uint32_t price) {
    if (save_data.gold < price) return false;

    // Check open spell slot (3 per level)
    const auto& spell = loader_.get_spell(spell_id);
    int level_idx = (spell_id / 8);

    for (int slot = 0; slot < 3; ++slot) {
        if (hero.spells[level_idx][slot] == 0xFF) {
            hero.spells[level_idx][slot] = spell_id;
            save_data.gold -= price;
            return true;
        }
    }
    return false;
}

bool MenuEngine::rest_at_inn(GameSaveData& save_data, uint32_t inn_price) {
    if (save_data.gold < inn_price) return false;

    save_data.gold -= inn_price;
    for (auto& hero : save_data.party) {
        if (hero.stats.hp > 0) {
            hero.stats.hp = hero.stats.max_hp;
            hero.stats.mp = hero.stats.max_mp;
            hero.status_ailments &= ~(Status::POISON | Status::BLIND | Status::SILENCE | Status::SLEEP | Status::PARALYSIS);
        }
    }

    // Save SRAM
    SaveSystem::save_game("ff1_sram.sav", save_data);
    return true;
}

bool MenuEngine::revive_at_clinic(PartyCharacter& hero, GameSaveData& save_data, uint32_t clinic_price) {
    if (save_data.gold < clinic_price) return false;
    if (hero.stats.hp > 0 && !(hero.status_ailments & (Status::DEATH | Status::STONE))) return false;

    save_data.gold -= clinic_price;
    hero.stats.hp = 1; // Resurrect with 1 HP
    hero.status_ailments &= ~(Status::DEATH | Status::STONE);
    return true;
}

bool MenuEngine::promote_party_classes(GameSaveData& save_data, std::string& out_message) {
    bool promoted_any = false;

    for (auto& hero : save_data.party) {
        switch (hero.char_class) {
            case ClassType::WARRIOR:
                hero.char_class = ClassType::KNIGHT;
                promoted_any = true;
                break;
            case ClassType::THIEF:
                hero.char_class = ClassType::NINJA;
                promoted_any = true;
                break;
            case ClassType::BLACK_BELT:
                hero.char_class = ClassType::MASTER;
                promoted_any = true;
                break;
            case ClassType::RED_MAGE:
                hero.char_class = ClassType::RED_WIZARD;
                promoted_any = true;
                break;
            case ClassType::WHITE_MAGE:
                hero.char_class = ClassType::WHITE_WIZARD;
                promoted_any = true;
                break;
            case ClassType::BLACK_MAGE:
                hero.char_class = ClassType::BLACK_WIZARD;
                promoted_any = true;
                break;
            default:
                break;
        }
        recalculate_hero_stats(hero);
    }

    if (promoted_any) {
        out_message = "King Bahamut granted class promotion to the Light Warriors!";
    } else {
        out_message = "The party has already been promoted!";
    }

    return promoted_any;
}

void MenuEngine::reorder_party_lineup(GameSaveData& save_data, size_t slot_a, size_t slot_b) {
    if (slot_a < 4 && slot_b < 4 && slot_a != slot_b) {
        std::swap(save_data.party[slot_a], save_data.party[slot_b]);
    }
}

} // namespace ff1
