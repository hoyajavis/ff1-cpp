#include "menu_engine.hpp"
#include "core/map_engine.hpp"
#include "engine/audio_engine.hpp"
#include <algorithm>
#include <iostream>

namespace ff1 {

// Authentic NES Level-Up Experience Table (Levels 1 to 50)
static const uint32_t lut_LevelUpExp[50] = {
    0,       40,      196,     548,     1196,    2240,    3780,    5916,    8748,    12376,
    16899,   22418,   29033,   36843,   45949,   56450,   68446,   82038,   97325,   114407,
    133385,  154358,  177427,  202690,  230249,  260203,  292652,  327697,  365436,  405971,
    449400,  495825,  545344,  598059,  654068,  713473,  776372,  842867,  913056,  987041,
    1064920, 1146795, 1232764, 1322929, 1417388, 1516243, 1619592, 1727537, 1840176, 1957611
};

MenuEngine::MenuEngine(const DataLoader& loader) : loader_(loader) {}

void MenuEngine::open_main_menu() {
    current_state_ = MenuState::MAIN_MENU;
    main_cursor_ = 0;
    trade_stage_ = 0;
    lineup_swap_stage_ = 0;
}

void MenuEngine::open_world_map() {
    current_state_ = MenuState::WORLD_MAP_SCREEN;
}

void MenuEngine::close_menu() {
    current_state_ = MenuState::CLOSED;
    trade_stage_ = 0;
    lineup_swap_stage_ = 0;
}

uint32_t MenuEngine::get_exp_for_level(uint8_t level) {
    if (level == 0) return 0;
    if (level > 50) return lut_LevelUpExp[49];
    return lut_LevelUpExp[level - 1];
}

uint32_t MenuEngine::get_exp_needed_for_next_level(uint8_t current_level, uint32_t current_exp) {
    if (current_level >= 50) return 0;
    uint32_t next_exp = lut_LevelUpExp[current_level];
    return (next_exp > current_exp) ? (next_exp - current_exp) : 0;
}

void MenuEngine::recalculate_hero_stats(PartyCharacter& hero) {
    int total_absorb = 0;
    int total_evade_penalty = 0;
    int bonus_damage = 0;
    int bonus_hit = 0;
    int weapon_crit = 0;

    // Check equipped weapons (first non-empty or primary)
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

    // Black Belt / Master unarmed rules (bank_0C.asm lines 5719-5750)
    if (hero.char_class == ClassType::BLACK_BELT || hero.char_class == ClassType::MASTER) {
        bool no_armor = std::all_of(hero.armors.begin(), hero.armors.end(), [](uint8_t id) { return id == 0xFF; });
        if (no_armor) {
            total_absorb = hero.level;
        }
        if (bonus_damage == 0) {
            weapon_crit = hero.level * 2;
        }
    }

    hero.stats.absorb = total_absorb;
    hero.stats.evade = std::max(0, 48 + hero.stats.agility - total_evade_penalty);
    if ((hero.char_class == ClassType::BLACK_BELT || hero.char_class == ClassType::MASTER) && bonus_damage == 0) {
        hero.stats.damage = hero.level * 2;
    } else {
        hero.stats.damage = (hero.stats.strength / 2) + bonus_damage;
    }
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

void MenuEngine::toggle_equipment_slot(PartyCharacter& hero, uint8_t slot_idx) {
    // In NES FF1, equipping toggles the weapon/armor in slot
    // Recalculating stats reflects the current configuration
    recalculate_hero_stats(hero);
}

void MenuEngine::trade_equipment_slots(PartyCharacter& hero_a, uint8_t slot_a, PartyCharacter& hero_b, uint8_t slot_b) {
    if (slot_a < 4 && slot_b < 4) {
        std::swap(hero_a.weapons[slot_a], hero_b.weapons[slot_b]);
    } else if (slot_a >= 4 && slot_b >= 4) {
        std::swap(hero_a.armors[slot_a - 4], hero_b.armors[slot_b - 4]);
    }
    recalculate_hero_stats(hero_a);
    recalculate_hero_stats(hero_b);
}

void MenuEngine::drop_equipment_slot(PartyCharacter& hero, uint8_t slot_idx) {
    if (slot_idx < 4) {
        hero.weapons[slot_idx] = 0xFF;
    } else {
        hero.armors[slot_idx - 4] = 0xFF;
    }
    recalculate_hero_stats(hero);
}

bool MenuEngine::use_consumable_potion(GameSaveData& save_data, uint8_t item_idx, uint8_t target_hero_idx, std::string& out_msg) {
    if (target_hero_idx >= 4) return false;
    auto& hero = save_data.party[target_hero_idx];

    if (item_idx == 0) { // HEAL Potion (+30 HP)
        if (save_data.consumables.heal_potions == 0) {
            out_msg = "Out of HEAL potions!";
            return false;
        }
        if (hero.stats.hp == 0 || (hero.status_ailments & Status::DEATH)) {
            out_msg = "It has no effect on the fallen.";
            return false;
        }
        hero.stats.hp = std::min<uint16_t>(hero.stats.max_hp, hero.stats.hp + 30);
        save_data.consumables.heal_potions--;
        out_msg = hero.name + " recovered 30 HP!";
        return true;
    } else if (item_idx == 1) { // PURE Potion (Antidote)
        if (save_data.consumables.pure_potions == 0) {
            out_msg = "Out of PURE potions!";
            return false;
        }
        if (!(hero.status_ailments & Status::POISON)) {
            out_msg = hero.name + " is not poisoned.";
            return false;
        }
        hero.status_ailments &= ~Status::POISON;
        save_data.consumables.pure_potions--;
        out_msg = hero.name + " was cured of poison!";
        return true;
    } else if (item_idx == 2) { // SOFT Potion (Gold Needle)
        if (save_data.consumables.soft_potions == 0) {
            out_msg = "Out of SOFT potions!";
            return false;
        }
        if (!(hero.status_ailments & Status::STONE)) {
            out_msg = hero.name + " is not petrified.";
            return false;
        }
        hero.status_ailments &= ~Status::STONE;
        save_data.consumables.soft_potions--;
        out_msg = hero.name + " was cured of stone!";
        return true;
    }

    return false;
}

bool MenuEngine::execute_camping_rest(GameSaveData& save_data, uint8_t camping_type, std::string& out_msg) {
    if (camping_type == 0) { // TENT
        if (save_data.consumables.tents == 0) {
            out_msg = "No TENT in inventory!";
            return false;
        }
        save_data.consumables.tents--;
        for (auto& hero : save_data.party) {
            if (hero.stats.hp > 0 && !(hero.status_ailments & Status::DEATH)) {
                hero.stats.hp = std::min<uint16_t>(hero.stats.max_hp, hero.stats.hp + 30);
            }
        }
        SaveSystem::save_game("ff1_save.sav", save_data);
        out_msg = "Pitched TENT! Recovered 30 HP. Game Saved!";
        return true;
    } else if (camping_type == 1) { // CABIN
        if (save_data.consumables.cabins == 0) {
            out_msg = "No CABIN in inventory!";
            return false;
        }
        save_data.consumables.cabins--;
        for (auto& hero : save_data.party) {
            if (hero.stats.hp > 0 && !(hero.status_ailments & Status::DEATH)) {
                hero.stats.hp = std::min<uint16_t>(hero.stats.max_hp, hero.stats.hp + 60);
                for (int t = 0; t < 8; ++t) {
                    hero.stats.mp[t] = std::min<uint8_t>(hero.stats.max_mp[t], hero.stats.mp[t] + 3);
                }
            }
        }
        SaveSystem::save_game("ff1_save.sav", save_data);
        out_msg = "Stayed in CABIN! Restored 60 HP, MP. Game Saved!";
        return true;
    } else if (camping_type == 2) { // HOUSE
        if (save_data.consumables.houses == 0) {
            out_msg = "No HOUSE in inventory!";
            return false;
        }
        save_data.consumables.houses--;
        for (auto& hero : save_data.party) {
            if (hero.stats.hp > 0 && !(hero.status_ailments & Status::DEATH)) {
                hero.stats.hp = hero.stats.max_hp;
                hero.stats.mp = hero.stats.max_mp;
            }
        }
        SaveSystem::save_game("ff1_save.sav", save_data);
        out_msg = "Rested in HOUSE! Fully restored HP, MP. Game Saved!";
        return true;
    }

    return false;
}

bool MenuEngine::is_field_spell(uint8_t spell_id) const {
    // Authentic NES Field Spells
    switch (spell_id) {
        case 0:  // CURE
        case 4:  // HEAL
        case 8:  // CUR2
        case 9:  // PURE
        case 12: // HEL2
        case 16: // CUR3
        case 17: // LIFE
        case 20: // HEL3
        case 24: // CUR4
        case 25: // LIF2
        case 26: // SOFT
        case 27: // EXIT
        case 28: // WARP
            return true;
        default:
            return false;
    }
}

bool MenuEngine::cast_field_spell(PartyCharacter& caster, uint8_t spell_id, uint8_t tier, GameSaveData& save_data, uint8_t target_idx, std::string& out_msg) {
    if (tier >= 8) return false;
    if (caster.stats.mp[tier] == 0) {
        out_msg = "Not enough MP in Tier " + std::to_string(tier + 1) + "!";
        return false;
    }
    if (target_idx >= 4) return false;
    auto& target = save_data.party[target_idx];

    bool cast_success = false;

    switch (spell_id) {
        case 0: // CURE
            if (target.stats.hp > 0) {
                target.stats.hp = std::min<uint16_t>(target.stats.max_hp, target.stats.hp + 24);
                out_msg = target.name + " recovered 24 HP!";
                cast_success = true;
            }
            break;
        case 8: // CUR2
            if (target.stats.hp > 0) {
                target.stats.hp = std::min<uint16_t>(target.stats.max_hp, target.stats.hp + 50);
                out_msg = target.name + " recovered 50 HP!";
                cast_success = true;
            }
            break;
        case 16: // CUR3
            if (target.stats.hp > 0) {
                target.stats.hp = std::min<uint16_t>(target.stats.max_hp, target.stats.hp + 100);
                out_msg = target.name + " recovered 100 HP!";
                cast_success = true;
            }
            break;
        case 24: // CUR4
            if (target.stats.hp > 0) {
                target.stats.hp = target.stats.max_hp;
                target.status_ailments &= ~(Status::POISON | Status::BLIND | Status::SILENCE | Status::SLEEP | Status::PARALYSIS);
                out_msg = target.name + " fully restored!";
                cast_success = true;
            }
            break;
        case 4:  // HEAL
        case 12: // HEL2
        case 20: // HEL3
            {
                uint16_t heal_amt = (spell_id == 4) ? 18 : (spell_id == 12 ? 36 : 72);
                for (auto& hero : save_data.party) {
                    if (hero.stats.hp > 0) {
                        hero.stats.hp = std::min<uint16_t>(hero.stats.max_hp, hero.stats.hp + heal_amt);
                    }
                }
                out_msg = "Party recovered " + std::to_string(heal_amt) + " HP!";
                cast_success = true;
            }
            break;
        case 9: // PURE
            if (target.status_ailments & Status::POISON) {
                target.status_ailments &= ~Status::POISON;
                out_msg = target.name + " cured of poison!";
                cast_success = true;
            } else {
                out_msg = target.name + " is not poisoned.";
            }
            break;
        case 26: // SOFT
            if (target.status_ailments & Status::STONE) {
                target.status_ailments &= ~Status::STONE;
                out_msg = target.name + " cured of stone!";
                cast_success = true;
            } else {
                out_msg = target.name + " is not petrified.";
            }
            break;
        case 17: // LIFE
            if (target.stats.hp == 0 || (target.status_ailments & Status::DEATH)) {
                target.stats.hp = 1;
                target.status_ailments &= ~(Status::DEATH | Status::STONE);
                out_msg = target.name + " revived with 1 HP!";
                cast_success = true;
            } else {
                out_msg = target.name + " is already alive.";
            }
            break;
        case 25: // LIF2
            if (target.stats.hp == 0 || (target.status_ailments & Status::DEATH)) {
                target.stats.hp = target.stats.max_hp;
                target.status_ailments &= ~(Status::DEATH | Status::STONE);
                out_msg = target.name + " resurrected with full HP!";
                cast_success = true;
            } else {
                out_msg = target.name + " is already alive.";
            }
            break;
        default:
            out_msg = "Spell cannot be cast here.";
            break;
    }

    if (cast_success) {
        caster.stats.mp[tier]--;
    }

    return cast_success;
}

MenuAction MenuEngine::handle_input(
    InputKey key,
    GameSaveData& save_data,
    MapEngine& map_engine,
    AudioEngine& audio,
    std::string& out_msg
) {
    (void)audio;
    MenuAction action = MenuAction::NONE;

    switch (current_state_) {
        case MenuState::MAIN_MENU: {
            if (key == InputKey::UP) {
                main_cursor_ = (main_cursor_ + 4) % 5;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::DOWN) {
                main_cursor_ = (main_cursor_ + 1) % 5;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::SELECT) {
                current_state_ = MenuState::LINEUP_SELECT;
                lineup_swap_stage_ = 0;
                lineup_src_char_ = char_cursor_;
                out_msg = "SELECT: Choose first party member to swap.";
                action = MenuAction::SOUND_SEL;
            } else if (key == InputKey::CONFIRM) {
                switch (main_cursor_) {
                    case 0: // ITEM
                        current_state_ = MenuState::ITEM_MENU;
                        item_cursor_ = 0;
                        action = MenuAction::SOUND_SEL;
                        break;
                    case 1: // MAGIC
                        current_state_ = MenuState::MAGIC_MENU;
                        magic_cursor_ = 0;
                        action = MenuAction::SOUND_SEL;
                        break;
                    case 2: // WEAPON
                    case 3: // ARMOR
                        current_state_ = MenuState::EQUIP_MENU;
                        equip_tab_ = EquipTab::EQUIP;
                        equip_slot_cursor_ = (main_cursor_ == 2) ? 0 : 4;
                        action = MenuAction::SOUND_SEL;
                        break;
                    case 4: // STATUS
                        current_state_ = MenuState::STATUS_MENU;
                        action = MenuAction::SOUND_SEL;
                        break;
                }
            } else if (key == InputKey::CANCEL || key == InputKey::START) {
                close_menu();
                action = MenuAction::CLOSE_MENU;
            }
            break;
        }

        case MenuState::LINEUP_SELECT: {
            if (key == InputKey::UP) {
                char_cursor_ = (char_cursor_ + 3) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::DOWN) {
                char_cursor_ = (char_cursor_ + 1) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM || key == InputKey::SELECT) {
                if (lineup_swap_stage_ == 0) {
                    lineup_src_char_ = char_cursor_;
                    lineup_swap_stage_ = 1;
                    out_msg = "Select second party member to swap with " + save_data.party[lineup_src_char_].name + ".";
                    action = MenuAction::SOUND_SEL;
                } else {
                    reorder_party_lineup(save_data, lineup_src_char_, char_cursor_);
                    lineup_swap_stage_ = 0;
                    current_state_ = MenuState::MAIN_MENU;
                    out_msg = "Party lineup reordered!";
                    action = MenuAction::SOUND_SEL;
                }
            } else if (key == InputKey::CANCEL) {
                if (lineup_swap_stage_ == 1) {
                    lineup_swap_stage_ = 0;
                    out_msg = "Swap cancelled.";
                    action = MenuAction::SOUND_CANCEL;
                } else {
                    current_state_ = MenuState::MAIN_MENU;
                    action = MenuAction::SOUND_CANCEL;
                }
            }
            break;
        }

        case MenuState::ITEM_MENU: {
            // 6 consumables + up to 16 key items
            int total_items = 6;
            for (size_t k = 0; k < static_cast<size_t>(KeyItem::COUNT); ++k) {
                if (save_data.key_items_and_flags[k] > 0) total_items++;
            }

            if (key == InputKey::UP) {
                item_cursor_ = (item_cursor_ + total_items - 1) % total_items;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::DOWN) {
                item_cursor_ = (item_cursor_ + 1) % total_items;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                if (item_cursor_ < 3) { // HEAL, PURE, SOFT
                    current_state_ = MenuState::ITEM_TARGET_SELECT;
                    target_char_cursor_ = 0;
                    out_msg = "Select party member to use item on.";
                    action = MenuAction::SOUND_SEL;
                } else if (item_cursor_ < 6) { // TENT, CABIN, HOUSE
                    if (map_engine.get_map_type() != MapType::OVERWORLD) {
                        out_msg = "You cannot use it here! (Overworld only)";
                        action = MenuAction::SOUND_CANCEL;
                    } else {
                        camping_type_ = item_cursor_ - 3;
                        current_state_ = MenuState::CAMPING_SAVE_PROMPT;
                        camping_confirm_cursor_ = 0;
                        out_msg = "Rest & Save game? Push A: YES, Push B: NO";
                        action = MenuAction::SOUND_SEL;
                    }
                } else { // Key item inspection
                    size_t key_idx = 0;
                    int count = 6;
                    for (size_t k = 0; k < static_cast<size_t>(KeyItem::COUNT); ++k) {
                        if (save_data.key_items_and_flags[k] > 0) {
                            if (count == item_cursor_) {
                                key_idx = k;
                                break;
                            }
                            count++;
                        }
                    }
                    KeyItemInfo info = get_key_item_info(static_cast<KeyItem>(key_idx));
                    out_msg = info.name + ": " + info.description;
                    action = MenuAction::SOUND_SEL;
                }
            } else if (key == InputKey::CANCEL) {
                current_state_ = MenuState::MAIN_MENU;
                action = MenuAction::SOUND_CANCEL;
            }
            break;
        }

        case MenuState::ITEM_TARGET_SELECT: {
            if (key == InputKey::UP) {
                target_char_cursor_ = (target_char_cursor_ + 3) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::DOWN) {
                target_char_cursor_ = (target_char_cursor_ + 1) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                bool used = use_consumable_potion(save_data, item_cursor_, target_char_cursor_, out_msg);
                if (used) {
                    action = MenuAction::SOUND_SEL;
                } else {
                    action = MenuAction::SOUND_CANCEL;
                }
            } else if (key == InputKey::CANCEL) {
                current_state_ = MenuState::ITEM_MENU;
                action = MenuAction::SOUND_CANCEL;
            }
            break;
        }

        case MenuState::CAMPING_SAVE_PROMPT: {
            if (key == InputKey::LEFT || key == InputKey::RIGHT || key == InputKey::UP || key == InputKey::DOWN) {
                camping_confirm_cursor_ ^= 1;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                if (camping_confirm_cursor_ == 0) { // YES
                    execute_camping_rest(save_data, camping_type_, out_msg);
                    current_state_ = MenuState::ITEM_MENU;
                    action = MenuAction::SAVE_GAME_TRIGGERED;
                } else { // NO
                    current_state_ = MenuState::ITEM_MENU;
                    out_msg = "Camping cancelled.";
                    action = MenuAction::SOUND_CANCEL;
                }
            } else if (key == InputKey::CANCEL) {
                current_state_ = MenuState::ITEM_MENU;
                out_msg = "Camping cancelled.";
                action = MenuAction::SOUND_CANCEL;
            }
            break;
        }

        case MenuState::MAGIC_MENU: {
            if (key == InputKey::LEFT) {
                char_cursor_ = (char_cursor_ + 3) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::RIGHT) {
                char_cursor_ = (char_cursor_ + 1) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::UP) {
                magic_cursor_ = (magic_cursor_ + 23) % 24;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::DOWN) {
                magic_cursor_ = (magic_cursor_ + 1) % 24;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                uint8_t tier = magic_cursor_ / 3;
                uint8_t slot = magic_cursor_ % 3;
                uint8_t sp_id = save_data.party[char_cursor_].spells[tier][slot];

                if (sp_id == 0xFF) {
                    out_msg = "Empty spell slot.";
                    action = MenuAction::SOUND_CANCEL;
                } else if (!is_field_spell(sp_id)) {
                    out_msg = "This spell cannot be cast in the field.";
                    action = MenuAction::SOUND_CANCEL;
                } else {
                    selected_spell_id_ = sp_id;
                    selected_spell_tier_ = tier;
                    current_state_ = MenuState::MAGIC_TARGET_SELECT;
                    target_char_cursor_ = 0;
                    out_msg = "Select target for spell.";
                    action = MenuAction::SOUND_SEL;
                }
            } else if (key == InputKey::CANCEL) {
                current_state_ = MenuState::MAIN_MENU;
                action = MenuAction::SOUND_CANCEL;
            }
            break;
        }

        case MenuState::MAGIC_TARGET_SELECT: {
            if (key == InputKey::UP) {
                target_char_cursor_ = (target_char_cursor_ + 3) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::DOWN) {
                target_char_cursor_ = (target_char_cursor_ + 1) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                bool cast_ok = cast_field_spell(
                    save_data.party[char_cursor_],
                    selected_spell_id_,
                    selected_spell_tier_,
                    save_data,
                    target_char_cursor_,
                    out_msg
                );
                if (cast_ok) {
                    current_state_ = MenuState::MAGIC_MENU;
                    action = MenuAction::SOUND_CAST;
                } else {
                    action = MenuAction::SOUND_CANCEL;
                }
            } else if (key == InputKey::CANCEL) {
                current_state_ = MenuState::MAGIC_MENU;
                action = MenuAction::SOUND_CANCEL;
            }
            break;
        }

        case MenuState::EQUIP_MENU: {
            if (key == InputKey::LEFT) {
                if (trade_stage_ == 0) {
                    char_cursor_ = (char_cursor_ + 3) % 4;
                    action = MenuAction::SOUND_MOVE;
                }
            } else if (key == InputKey::RIGHT) {
                if (trade_stage_ == 0) {
                    char_cursor_ = (char_cursor_ + 1) % 4;
                    action = MenuAction::SOUND_MOVE;
                }
            } else if (key == InputKey::UP) {
                equip_slot_cursor_ = (equip_slot_cursor_ + 7) % 8;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::DOWN) {
                equip_slot_cursor_ = (equip_slot_cursor_ + 1) % 8;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::SELECT) {
                equip_tab_ = static_cast<EquipTab>((static_cast<uint8_t>(equip_tab_) + 1) % 3);
                trade_stage_ = 0;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                if (equip_tab_ == EquipTab::EQUIP) {
                    toggle_equipment_slot(save_data.party[char_cursor_], equip_slot_cursor_);
                    out_msg = "Equipment adjusted.";
                    action = MenuAction::SOUND_SEL;
                } else if (equip_tab_ == EquipTab::TRADE) {
                    if (trade_stage_ == 0) {
                        trade_src_char_ = char_cursor_;
                        trade_src_slot_ = equip_slot_cursor_;
                        trade_stage_ = 1;
                        out_msg = "Selected slot for trade. Pick destination hero & slot.";
                        action = MenuAction::SOUND_SEL;
                    } else {
                        trade_equipment_slots(
                            save_data.party[trade_src_char_],
                            trade_src_slot_,
                            save_data.party[char_cursor_],
                            equip_slot_cursor_
                        );
                        trade_stage_ = 0;
                        out_msg = "Items traded!";
                        action = MenuAction::SOUND_SEL;
                    }
                } else if (equip_tab_ == EquipTab::DROP) {
                    drop_equipment_slot(save_data.party[char_cursor_], equip_slot_cursor_);
                    out_msg = "Item dropped from inventory.";
                    action = MenuAction::SOUND_SEL;
                }
            } else if (key == InputKey::CANCEL) {
                if (trade_stage_ == 1) {
                    trade_stage_ = 0;
                    out_msg = "Trade cancelled.";
                    action = MenuAction::SOUND_CANCEL;
                } else {
                    current_state_ = MenuState::MAIN_MENU;
                    action = MenuAction::SOUND_CANCEL;
                }
            }
            break;
        }

        case MenuState::STATUS_MENU: {
            if (key == InputKey::LEFT) {
                char_cursor_ = (char_cursor_ + 3) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::RIGHT) {
                char_cursor_ = (char_cursor_ + 1) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM || key == InputKey::CANCEL || key == InputKey::START) {
                current_state_ = MenuState::MAIN_MENU;
                action = MenuAction::SOUND_CANCEL;
            }
            break;
        }

        case MenuState::WORLD_MAP_SCREEN: {
            if (key == InputKey::CONFIRM || key == InputKey::CANCEL || key == InputKey::START) {
                close_menu();
                action = MenuAction::CLOSE_MENU;
            }
            break;
        }

        default:
        case MenuState::SHOP: {
            return handle_shop_input(key, save_data, audio, out_msg);
        }

        default:
            break;
    }

    return action;
}

// Authentic NES Weapon Names (40 items)
static const char* lut_WeaponNames[40] = {
    "NUNCHUCK", "SMALL KNIFE", "WOODEN STAFF", "RAPIER", "IRON HAMMER",
    "SHORT SWORD", "HAND AXE", "SCIMITAR", "IRON STAFF", "LARGE DAGGER",
    "IRON SWORD", "IRON NUNCHUCK", "BROADSWORD", "BATTLE AXE", "LONG SWORD",
    "GREAT AXE", "FALCHION", "SILVER KNIFE", "SILVER SWORD", "SILVER HAMMER",
    "SILVER AXE", "FLAME SWORD", "ICE SWORD", "DRAGON SWORD", "GIANT SWORD",
    "SUN SWORD", "CORAL SWORD", "WERE SWORD", "RUNE SWORD", "POWER STAFF",
    "WIZARD STAFF", "LIGHT AXE", "HEAL STAFF", "MAGE STAFF", "DEFENSE",
    "VORPAL", "CATCLAW", "THOR HAMMER", "BANE SWORD", "EXCALIBUR"
};

// Authentic NES Weapon Base Prices
static const uint32_t lut_WeaponPrices[40] = {
    10, 5, 5, 10, 10,
    550, 550, 200, 200, 175,
    4000, 200, 800, 550, 1500,
    2000, 450, 800, 4000, 2500,
    4500, 10000, 15000, 8000, 8000,
    20000, 8000, 6000, 5000, 12345,
    50000, 10000, 25000, 25000, 40000,
    30000, 65000, 40000, 60000, 65535
};

// Authentic NES Armor Names (40 items)
static const char* lut_ArmorNames[40] = {
    "CLOTH", "WOODEN ARMOR", "CHAIN ARMOR", "IRON ARMOR", "STEEL ARMOR",
    "SILVER ARMOR", "FLAME ARMOR", "ICE ARMOR", "OPAL ARMOR", "DRAGON ARMOR",
    "COPPER GAUNTLET", "IRON GAUNTLET", "SILVER GAUNTLET", "ZEUS GAUNTLET", "POWER GAUNTLET",
    "OPAL GAUNTLET", "HEAL GAUNTLET", "WOODEN SHIELD", "IRON SHIELD", "SILVER SHIELD",
    "FLAME SHIELD", "ICE SHIELD", "OPAL SHIELD", "AEGIS SHIELD", "BUCKLER",
    "PROCAPE", "CAP", "WOODEN HELMET", "IRON HELMET", "SILVER HELMET",
    "OPAL HELMET", "HEAL HELMET", "RIBBON", "PRORING", "WHITE SHIRT",
    "BLACK SHIRT", "GOLDEN ARMOR", "MYSTIC SHIELD", "AEGIS CAPE", "HERO GAUNTLET"
};

// Authentic NES Armor Base Prices
static const uint32_t lut_ArmorPrices[40] = {
    10, 50, 80, 800, 45000,
    7500, 30000, 30000, 60000, 60000,
    200, 1000, 2500, 15000, 10000,
    20000, 25000, 15, 100, 2500,
    10000, 10000, 15000, 40000, 2500,
    20000, 5, 100, 450, 2500,
    10000, 20000, 50000, 20000, 20000,
    20000, 10000, 15000, 20000, 30000
};

// Authentic NES Spell Names (64 spells)
static const char* lut_SpellNames[64] = {
    "CURE", "HARM", "FOG",  "RUSE", "FIRE", "SLEP", "LOCK", "LIT",  // Tier 1
    "LAMP", "MUTE", "ALIT", "INVS", "ICE",  "DARK", "TMPR", "SLOW", // Tier 2
    "CUR2", "HRM2", "AFIR", "HEAL", "FIR2", "HOLD", "LIT2", "LOK2", // Tier 3
    "PURE", "FEAR", "AICE", "AMUT", "SLP2", "FAST", "CONF", "ICE2", // Tier 4
    "CUR3", "LIFE", "HRM3", "HEL2", "FIR3", "BANE", "WARP", "SLO2", // Tier 5
    "SOFT", "EXIT", "FOG2", "INV2", "LIT3", "RUB",  "QAKE", "STUN", // Tier 6
    "CUR4", "HRM4", "ARUB", "HEL3", "ICE3", "BRAK", "SABR", "BLND", // Tier 7
    "LIF2", "FADE", "WALL", "XFER", "NUKE", "STOP", "ZAP",  "XXXX"  // Tier 8
};

// Authentic NES Spell Tier Prices
static const uint32_t lut_SpellTierPrices[8] = {
    100, 400, 1500, 4000, 8000, 20000, 45000, 60000
};

// Authentic Consumable Item Names & Prices
static const char* lut_ConsumableNames[7] = {
    "HEAL", "PURE", "SOFT", "TENT", "CABIN", "HOUSE", "BOTTLE"
};
static const uint32_t lut_ConsumablePrices[7] = {
    60, 75, 800, 75, 250, 1000, 40000
};

std::string MenuEngine::get_item_name(ShopType type, uint8_t item_id) const {
    if (item_id == 0xFF) return "--";
    switch (type) {
        case ShopType::WEAPON:
            return (item_id < 40) ? lut_WeaponNames[item_id] : ("WEAPON #" + std::to_string(item_id));
        case ShopType::ARMOR:
            return (item_id < 40) ? lut_ArmorNames[item_id] : ("ARMOR #" + std::to_string(item_id));
        case ShopType::WHITE_MAGIC:
        case ShopType::BLACK_MAGIC:
            return (item_id < 64) ? lut_SpellNames[item_id] : ("SPELL #" + std::to_string(item_id));
        case ShopType::ITEM:
        case ShopType::CARAVAN:
            return (item_id < 7) ? lut_ConsumableNames[item_id] : ("ITEM #" + std::to_string(item_id));
        default:
            return "ITEM";
    }
}

uint32_t MenuEngine::get_item_price(ShopType type, uint8_t item_id) const {
    if (item_id == 0xFF) return 0;
    switch (type) {
        case ShopType::WEAPON:
            return (item_id < 40) ? lut_WeaponPrices[item_id] : 100;
        case ShopType::ARMOR:
            return (item_id < 40) ? lut_ArmorPrices[item_id] : 100;
        case ShopType::WHITE_MAGIC:
        case ShopType::BLACK_MAGIC: {
            uint8_t tier = item_id / 8;
            return (tier < 8) ? lut_SpellTierPrices[tier] : 100;
        }
        case ShopType::ITEM:
        case ShopType::CARAVAN:
            return (item_id < 7) ? lut_ConsumablePrices[item_id] : 100;
        default:
            return 100;
    }
}

uint32_t MenuEngine::get_sell_price(ShopType type, uint8_t item_id) const {
    // In NES FF1: Sell_Price = Base_Price >> 1 (50% base price)
    return get_item_price(type, item_id) >> 1;
}

void MenuEngine::open_shop(uint8_t shop_id, GameSaveData& save_data) {
    (void)save_data;
    current_state_ = MenuState::SHOP;
    shop_cursor_ = 0;
    shop_sub_cursor_ = 0;
    shop_item_idx_ = 0;
    shop_target_hero_ = 0;

    const auto& all_shops = loader_.get_shops();
    if (shop_id < all_shops.size()) {
        current_shop_ = all_shops[shop_id];
    } else {
        current_shop_.shop_id = shop_id;
        current_shop_.type = ShopType::WEAPON;
        current_shop_.items = {0, 1, 2, 3};
        current_shop_.prices = {10, 5, 5, 10};
    }

    if (current_shop_.type == ShopType::INN) {
        shop_mode_ = ShopMode::INN_PROMPT;
        shop_service_cost_ = (current_shop_.prices[0] > 0) ? current_shop_.prices[0] : 30;
        shop_dialogue_ = "Stay the night for " + std::to_string(shop_service_cost_) + " GP?";
    } else if (current_shop_.type == ShopType::CLINIC) {
        shop_mode_ = ShopMode::CLINIC_SELECT_HERO;
        shop_service_cost_ = (current_shop_.prices[0] > 0) ? current_shop_.prices[0] : 40;
        shop_dialogue_ = "Who shall be revived?";
    } else {
        shop_mode_ = ShopMode::BUY_SELL_EXIT;
        shop_dialogue_ = "Welcome! What would you like to do?";
    }
}

void MenuEngine::open_shop_direct(
    ShopType type,
    const std::array<uint8_t, 4>& items,
    const std::array<uint16_t, 4>& prices,
    uint32_t service_cost,
    GameSaveData& save_data
) {
    (void)save_data;
    current_state_ = MenuState::SHOP;
    shop_cursor_ = 0;
    shop_sub_cursor_ = 0;
    shop_item_idx_ = 0;
    shop_target_hero_ = 0;

    current_shop_.shop_id = 99;
    current_shop_.type = type;
    current_shop_.items = items;
    current_shop_.prices = prices;
    shop_service_cost_ = service_cost;

    if (type == ShopType::INN) {
        shop_mode_ = ShopMode::INN_PROMPT;
        shop_dialogue_ = "Stay the night for " + std::to_string(shop_service_cost_) + " GP?";
    } else if (type == ShopType::CLINIC) {
        shop_mode_ = ShopMode::CLINIC_SELECT_HERO;
        shop_dialogue_ = "Who shall be revived?";
    } else {
        shop_mode_ = ShopMode::BUY_SELL_EXIT;
        shop_dialogue_ = "Welcome! What would you like to do?";
    }
}

MenuAction MenuEngine::handle_shop_input(
    InputKey key,
    GameSaveData& save_data,
    AudioEngine& audio,
    std::string& out_msg
) {
    (void)audio;
    MenuAction action = MenuAction::NONE;

    switch (shop_mode_) {
        case ShopMode::BUY_SELL_EXIT: {
            int max_options = (current_shop_.type == ShopType::WEAPON || current_shop_.type == ShopType::ARMOR) ? 3 : 2;
            if (key == InputKey::UP) {
                shop_cursor_ = (shop_cursor_ + max_options - 1) % max_options;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::DOWN) {
                shop_cursor_ = (shop_cursor_ + 1) % max_options;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                if (shop_cursor_ == 0) { // BUY
                    if (current_shop_.type == ShopType::WHITE_MAGIC || current_shop_.type == ShopType::BLACK_MAGIC) {
                        shop_mode_ = ShopMode::BUY_CHOOSE_HERO;
                        shop_cursor_ = 0;
                        shop_dialogue_ = "Who will learn the spell?";
                    } else {
                        shop_mode_ = ShopMode::BUY_SELECT_ITEM;
                        shop_cursor_ = 0;
                        shop_dialogue_ = "What would you like?";
                    }
                    action = MenuAction::SOUND_SEL;
                } else if (shop_cursor_ == 1 && max_options == 3) { // SELL
                    shop_mode_ = ShopMode::SELL_CHOOSE_HERO;
                    shop_cursor_ = 0;
                    shop_dialogue_ = "Whose item do you want to sell?";
                    action = MenuAction::SOUND_SEL;
                } else { // EXIT
                    close_menu();
                    out_msg = "Please come again!";
                    action = MenuAction::CLOSE_MENU;
                }
            } else if (key == InputKey::CANCEL) {
                close_menu();
                out_msg = "Please come again!";
                action = MenuAction::CLOSE_MENU;
            }
            break;
        }

        case ShopMode::BUY_SELECT_ITEM: {
            if (key == InputKey::UP) {
                shop_cursor_ = (shop_cursor_ + 3) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::DOWN) {
                shop_cursor_ = (shop_cursor_ + 1) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                uint8_t item_id = current_shop_.items[shop_cursor_];
                if (item_id == 0xFF) {
                    shop_dialogue_ = "No item in that slot.";
                    action = MenuAction::SOUND_CANCEL;
                } else {
                    shop_item_idx_ = item_id;
                    if (current_shop_.type == ShopType::WEAPON || current_shop_.type == ShopType::ARMOR) {
                        shop_mode_ = ShopMode::BUY_CHOOSE_HERO;
                        shop_cursor_ = 0;
                        shop_dialogue_ = "Who will take it?";
                        action = MenuAction::SOUND_SEL;
                    } else if (current_shop_.type == ShopType::WHITE_MAGIC || current_shop_.type == ShopType::BLACK_MAGIC) {
                        // Magic purchase confirmation
                        auto& hero = save_data.party[shop_target_hero_];
                        if (!can_class_learn_spell(hero.char_class, item_id)) {
                            shop_dialogue_ = hero.name + " cannot learn this spell!";
                            action = MenuAction::SOUND_CANCEL;
                        } else {
                            uint8_t tier = item_id / 8;
                            bool already_has = false;
                            bool has_slot = false;
                            for (int s = 0; s < 3; ++s) {
                                if (hero.spells[tier][s] == item_id) already_has = true;
                                if (hero.spells[tier][s] == 0xFF) has_slot = true;
                            }
                            if (already_has) {
                                shop_dialogue_ = hero.name + " already knows that spell!";
                                action = MenuAction::SOUND_CANCEL;
                            } else if (!has_slot) {
                                shop_dialogue_ = hero.name + "'s Tier " + std::to_string(tier + 1) + " spellbook is full!";
                                action = MenuAction::SOUND_CANCEL;
                            } else {
                                uint32_t price = get_item_price(current_shop_.type, item_id);
                                shop_mode_ = ShopMode::BUY_CONFIRM;
                                shop_sub_cursor_ = 0;
                                shop_dialogue_ = get_item_name(current_shop_.type, item_id) + " for " + std::to_string(price) + " GP. OK?";
                                action = MenuAction::SOUND_SEL;
                            }
                        }
                    } else { // ITEM or CARAVAN
                        uint32_t price = get_item_price(current_shop_.type, item_id);
                        shop_mode_ = ShopMode::BUY_CONFIRM;
                        shop_sub_cursor_ = 0;
                        shop_dialogue_ = get_item_name(current_shop_.type, item_id) + " for " + std::to_string(price) + " GP. OK?";
                        action = MenuAction::SOUND_SEL;
                    }
                }
            } else if (key == InputKey::CANCEL) {
                if (current_shop_.type == ShopType::WHITE_MAGIC || current_shop_.type == ShopType::BLACK_MAGIC) {
                    shop_mode_ = ShopMode::BUY_CHOOSE_HERO;
                    shop_cursor_ = shop_target_hero_;
                    shop_dialogue_ = "Who will learn the spell?";
                } else {
                    shop_mode_ = ShopMode::BUY_SELL_EXIT;
                    shop_cursor_ = 0;
                    shop_dialogue_ = "What else can I do for you?";
                }
                action = MenuAction::SOUND_CANCEL;
            }
            break;
        }

        case ShopMode::BUY_CHOOSE_HERO: {
            if (key == InputKey::UP) {
                shop_cursor_ = (shop_cursor_ + 3) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::DOWN) {
                shop_cursor_ = (shop_cursor_ + 1) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                shop_target_hero_ = shop_cursor_;
                if (current_shop_.type == ShopType::WHITE_MAGIC || current_shop_.type == ShopType::BLACK_MAGIC) {
                    shop_mode_ = ShopMode::BUY_SELECT_ITEM;
                    shop_cursor_ = 0;
                    shop_dialogue_ = "Which spell shall " + save_data.party[shop_target_hero_].name + " learn?";
                    action = MenuAction::SOUND_SEL;
                } else {
                    // Check capacity
                    auto& hero = save_data.party[shop_target_hero_];
                    bool has_room = false;
                    if (current_shop_.type == ShopType::WEAPON) {
                        for (uint8_t w : hero.weapons) if (w == 0xFF) { has_room = true; break; }
                    } else {
                        for (uint8_t a : hero.armors) if (a == 0xFF) { has_room = true; break; }
                    }

                    if (!has_room) {
                        shop_dialogue_ = hero.name + " cannot carry anymore!";
                        action = MenuAction::SOUND_CANCEL;
                    } else {
                        uint32_t price = get_item_price(current_shop_.type, shop_item_idx_);
                        shop_mode_ = ShopMode::BUY_CONFIRM;
                        shop_sub_cursor_ = 0;
                        shop_dialogue_ = get_item_name(current_shop_.type, shop_item_idx_) + " for " + std::to_string(price) + " GP. OK?";
                        action = MenuAction::SOUND_SEL;
                    }
                }
            } else if (key == InputKey::CANCEL) {
                if (current_shop_.type == ShopType::WHITE_MAGIC || current_shop_.type == ShopType::BLACK_MAGIC) {
                    shop_mode_ = ShopMode::BUY_SELL_EXIT;
                    shop_cursor_ = 0;
                    shop_dialogue_ = "What else can I do for you?";
                } else {
                    shop_mode_ = ShopMode::BUY_SELECT_ITEM;
                    shop_cursor_ = 0;
                    shop_dialogue_ = "What would you like?";
                }
                action = MenuAction::SOUND_CANCEL;
            }
            break;
        }

        case ShopMode::BUY_CONFIRM: {
            if (key == InputKey::LEFT || key == InputKey::RIGHT || key == InputKey::UP || key == InputKey::DOWN) {
                shop_sub_cursor_ ^= 1;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                if (shop_sub_cursor_ == 0) { // YES
                    uint32_t price = get_item_price(current_shop_.type, shop_item_idx_);
                    if (save_data.gold < price) {
                        shop_dialogue_ = "You cannot afford it!";
                        action = MenuAction::SOUND_CANCEL;
                    } else {
                        if (current_shop_.type == ShopType::WEAPON) {
                            auto& hero = save_data.party[shop_target_hero_];
                            for (int i = 0; i < 4; ++i) {
                                if (hero.weapons[i] == 0xFF) {
                                    hero.weapons[i] = shop_item_idx_;
                                    break;
                                }
                            }
                            recalculate_hero_stats(hero);
                            save_data.gold -= price;
                            shop_dialogue_ = "Thank you! What else?";
                            shop_mode_ = ShopMode::BUY_SELECT_ITEM;
                            action = MenuAction::SOUND_SEL;
                        } else if (current_shop_.type == ShopType::ARMOR) {
                            auto& hero = save_data.party[shop_target_hero_];
                            for (int i = 0; i < 4; ++i) {
                                if (hero.armors[i] == 0xFF) {
                                    hero.armors[i] = shop_item_idx_;
                                    break;
                                }
                            }
                            recalculate_hero_stats(hero);
                            save_data.gold -= price;
                            shop_dialogue_ = "Thank you! What else?";
                            shop_mode_ = ShopMode::BUY_SELECT_ITEM;
                            action = MenuAction::SOUND_SEL;
                        } else if (current_shop_.type == ShopType::WHITE_MAGIC || current_shop_.type == ShopType::BLACK_MAGIC) {
                            auto& hero = save_data.party[shop_target_hero_];
                            uint8_t tier = shop_item_idx_ / 8;
                            for (int s = 0; s < 3; ++s) {
                                if (hero.spells[tier][s] == 0xFF) {
                                    hero.spells[tier][s] = shop_item_idx_;
                                    break;
                                }
                            }
                            save_data.gold -= price;
                            shop_dialogue_ = hero.name + " learned " + get_item_name(current_shop_.type, shop_item_idx_) + "!";
                            shop_mode_ = ShopMode::BUY_SELECT_ITEM;
                            action = MenuAction::SOUND_CAST;
                        } else if (current_shop_.type == ShopType::CARAVAN && shop_item_idx_ == 6) { // BOTTLE
                            save_data.gold -= price;
                            save_data.key_items_and_flags[static_cast<size_t>(KeyItem::BOTTLE)] = 1;
                            save_data.key_items_and_flags[QuestFlag::CARAVAN_BOTTLE_BOUGHT] = 1;
                            shop_dialogue_ = "Bought FAIRY BOTTLE! Take care of the fairy!";
                            shop_mode_ = ShopMode::BUY_SELL_EXIT;
                            action = MenuAction::SOUND_SEL;
                        } else { // Consumable items
                            if (shop_item_idx_ == 0 && save_data.consumables.heal_potions < 99) save_data.consumables.heal_potions++;
                            else if (shop_item_idx_ == 1 && save_data.consumables.pure_potions < 99) save_data.consumables.pure_potions++;
                            else if (shop_item_idx_ == 2 && save_data.consumables.soft_potions < 99) save_data.consumables.soft_potions++;
                            else if (shop_item_idx_ == 3 && save_data.consumables.tents < 99) save_data.consumables.tents++;
                            else if (shop_item_idx_ == 4 && save_data.consumables.cabins < 99) save_data.consumables.cabins++;
                            else if (shop_item_idx_ == 5 && save_data.consumables.houses < 99) save_data.consumables.houses++;
                            save_data.gold -= price;
                            shop_dialogue_ = "Thank you! What else?";
                            shop_mode_ = ShopMode::BUY_SELECT_ITEM;
                            action = MenuAction::SOUND_SEL;
                        }
                    }
                } else { // NO
                    shop_dialogue_ = "Too bad... Something else?";
                    shop_mode_ = ShopMode::BUY_SELECT_ITEM;
                    action = MenuAction::SOUND_CANCEL;
                }
            } else if (key == InputKey::CANCEL) {
                shop_dialogue_ = "Too bad... Something else?";
                shop_mode_ = ShopMode::BUY_SELECT_ITEM;
                action = MenuAction::SOUND_CANCEL;
            }
            break;
        }

        case ShopMode::SELL_CHOOSE_HERO: {
            if (key == InputKey::UP) {
                shop_cursor_ = (shop_cursor_ + 3) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::DOWN) {
                shop_cursor_ = (shop_cursor_ + 1) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                shop_target_hero_ = shop_cursor_;
                auto& hero = save_data.party[shop_target_hero_];
                bool has_any = false;
                if (current_shop_.type == ShopType::WEAPON) {
                    for (uint8_t w : hero.weapons) if (w != 0xFF) { has_any = true; break; }
                } else {
                    for (uint8_t a : hero.armors) if (a != 0xFF) { has_any = true; break; }
                }

                if (!has_any) {
                    shop_dialogue_ = hero.name + " has nothing to sell!";
                    action = MenuAction::SOUND_CANCEL;
                } else {
                    shop_mode_ = ShopMode::SELL_SELECT_ITEM;
                    shop_cursor_ = 0;
                    shop_dialogue_ = "Which item to sell?";
                    action = MenuAction::SOUND_SEL;
                }
            } else if (key == InputKey::CANCEL) {
                shop_mode_ = ShopMode::BUY_SELL_EXIT;
                shop_cursor_ = 1;
                shop_dialogue_ = "What else can I do for you?";
                action = MenuAction::SOUND_CANCEL;
            }
            break;
        }

        case ShopMode::SELL_SELECT_ITEM: {
            if (key == InputKey::UP) {
                shop_cursor_ = (shop_cursor_ + 3) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::DOWN) {
                shop_cursor_ = (shop_cursor_ + 1) % 4;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                auto& hero = save_data.party[shop_target_hero_];
                uint8_t item_id = (current_shop_.type == ShopType::WEAPON) ? hero.weapons[shop_cursor_] : hero.armors[shop_cursor_];
                if (item_id == 0xFF) {
                    shop_dialogue_ = "Nothing in that slot!";
                    action = MenuAction::SOUND_CANCEL;
                } else {
                    shop_item_idx_ = item_id;
                    uint32_t sell_price = get_sell_price(current_shop_.type, item_id);
                    shop_mode_ = ShopMode::SELL_CONFIRM;
                    shop_sub_cursor_ = 0;
                    shop_dialogue_ = get_item_name(current_shop_.type, item_id) + " for " + std::to_string(sell_price) + " GP. OK?";
                    action = MenuAction::SOUND_SEL;
                }
            } else if (key == InputKey::CANCEL) {
                shop_mode_ = ShopMode::SELL_CHOOSE_HERO;
                shop_cursor_ = shop_target_hero_;
                shop_dialogue_ = "Whose item do you want to sell?";
                action = MenuAction::SOUND_CANCEL;
            }
            break;
        }

        case ShopMode::SELL_CONFIRM: {
            if (key == InputKey::LEFT || key == InputKey::RIGHT || key == InputKey::UP || key == InputKey::DOWN) {
                shop_sub_cursor_ ^= 1;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                if (shop_sub_cursor_ == 0) { // YES
                    auto& hero = save_data.party[shop_target_hero_];
                    uint32_t sell_price = get_sell_price(current_shop_.type, shop_item_idx_);
                    if (current_shop_.type == ShopType::WEAPON) {
                        hero.weapons[shop_cursor_] = 0xFF;
                    } else {
                        hero.armors[shop_cursor_] = 0xFF;
                    }
                    recalculate_hero_stats(hero);
                    save_data.gold = std::min<uint32_t>(999999, save_data.gold + sell_price);
                    shop_dialogue_ = "Sold for " + std::to_string(sell_price) + " GP! Anything else?";
                    shop_mode_ = ShopMode::SELL_CHOOSE_HERO;
                    action = MenuAction::SOUND_SEL;
                } else { // NO
                    shop_dialogue_ = "Too bad... Something else?";
                    shop_mode_ = ShopMode::SELL_SELECT_ITEM;
                    action = MenuAction::SOUND_CANCEL;
                }
            } else if (key == InputKey::CANCEL) {
                shop_dialogue_ = "Cancelled sale.";
                shop_mode_ = ShopMode::SELL_SELECT_ITEM;
                action = MenuAction::SOUND_CANCEL;
            }
            break;
        }

        case ShopMode::INN_PROMPT: {
            if (key == InputKey::LEFT || key == InputKey::RIGHT || key == InputKey::UP || key == InputKey::DOWN) {
                shop_sub_cursor_ ^= 1;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                if (shop_sub_cursor_ == 0) { // YES
                    if (save_data.gold < shop_service_cost_) {
                        shop_dialogue_ = "You cannot afford to stay here!";
                        action = MenuAction::SOUND_CANCEL;
                    } else {
                        save_data.gold -= shop_service_cost_;
                        for (auto& hero : save_data.party) {
                            if (hero.stats.hp > 0 && !(hero.status_ailments & Status::DEATH)) {
                                hero.stats.hp = hero.stats.max_hp;
                                hero.stats.mp = hero.stats.max_mp;
                                hero.status_ailments &= ~(Status::POISON | Status::BLIND | Status::SILENCE | Status::SLEEP | Status::PARALYSIS);
                            }
                        }
                        SaveSystem::save_game("ff1_save.sav", save_data);
                        shop_mode_ = ShopMode::INN_RESTING;
                        shop_dialogue_ = "Rested and saved! Hold RESET while turning POWER off.";
                        action = MenuAction::SAVE_GAME_TRIGGERED;
                    }
                } else { // NO
                    close_menu();
                    out_msg = "Please come again!";
                    action = MenuAction::CLOSE_MENU;
                }
            } else if (key == InputKey::CANCEL) {
                close_menu();
                out_msg = "Please come again!";
                action = MenuAction::CLOSE_MENU;
            }
            break;
        }

        case ShopMode::INN_RESTING: {
            if (key == InputKey::CONFIRM || key == InputKey::CANCEL || key == InputKey::START) {
                close_menu();
                out_msg = "Good luck on your journey!";
                action = MenuAction::CLOSE_MENU;
            }
            break;
        }

        case ShopMode::CLINIC_SELECT_HERO: {
            // Find dead heroes
            std::vector<uint8_t> dead_indices;
            for (uint8_t i = 0; i < 4; ++i) {
                if (save_data.party[i].stats.hp == 0 || (save_data.party[i].status_ailments & (Status::DEATH | Status::STONE))) {
                    dead_indices.push_back(i);
                }
            }

            if (dead_indices.empty()) {
                shop_dialogue_ = "Nobody is dead! You don't need my services.";
                if (key == InputKey::CONFIRM || key == InputKey::CANCEL) {
                    close_menu();
                    action = MenuAction::CLOSE_MENU;
                }
            } else {
                if (key == InputKey::UP) {
                    shop_cursor_ = (shop_cursor_ + dead_indices.size() - 1) % dead_indices.size();
                    action = MenuAction::SOUND_MOVE;
                } else if (key == InputKey::DOWN) {
                    shop_cursor_ = (shop_cursor_ + 1) % dead_indices.size();
                    action = MenuAction::SOUND_MOVE;
                } else if (key == InputKey::CONFIRM) {
                    shop_target_hero_ = dead_indices[shop_cursor_];
                    shop_mode_ = ShopMode::CLINIC_CONFIRM;
                    shop_sub_cursor_ = 0;
                    shop_dialogue_ = "Revive " + save_data.party[shop_target_hero_].name + " for " + std::to_string(shop_service_cost_) + " GP?";
                    action = MenuAction::SOUND_SEL;
                } else if (key == InputKey::CANCEL) {
                    close_menu();
                    out_msg = "May the Light protect you.";
                    action = MenuAction::CLOSE_MENU;
                }
            }
            break;
        }

        case ShopMode::CLINIC_CONFIRM: {
            if (key == InputKey::LEFT || key == InputKey::RIGHT || key == InputKey::UP || key == InputKey::DOWN) {
                shop_sub_cursor_ ^= 1;
                action = MenuAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                if (shop_sub_cursor_ == 0) { // YES
                    if (save_data.gold < shop_service_cost_) {
                        shop_dialogue_ = "You cannot afford this revival!";
                        action = MenuAction::SOUND_CANCEL;
                    } else {
                        save_data.gold -= shop_service_cost_;
                        auto& hero = save_data.party[shop_target_hero_];
                        hero.stats.hp = 1;
                        hero.status_ailments &= ~(Status::DEATH | Status::STONE);
                        shop_dialogue_ = hero.name + " revived with 1 HP! Who else?";
                        shop_mode_ = ShopMode::CLINIC_SELECT_HERO;
                        shop_cursor_ = 0;
                        action = MenuAction::SOUND_CAST;
                    }
                } else { // NO
                    shop_dialogue_ = "Who else shall be revived?";
                    shop_mode_ = ShopMode::CLINIC_SELECT_HERO;
                    shop_cursor_ = 0;
                    action = MenuAction::SOUND_CANCEL;
                }
            } else if (key == InputKey::CANCEL) {
                shop_mode_ = ShopMode::CLINIC_SELECT_HERO;
                shop_cursor_ = 0;
                action = MenuAction::SOUND_CANCEL;
            }
            break;
        }

        default:
            break;
    }

    return action;
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
    if (!can_class_learn_spell(hero.char_class, spell_id)) return false;

    int level_idx = (spell_id / 8);
    if (level_idx >= 8) return false;

    for (int slot = 0; slot < 3; ++slot) {
        if (hero.spells[level_idx][slot] == spell_id) return false; // Already learned
    }

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
        if (hero.stats.hp > 0 && !(hero.status_ailments & Status::DEATH)) {
            hero.stats.hp = hero.stats.max_hp;
            hero.stats.mp = hero.stats.max_mp;
            hero.status_ailments &= ~(Status::POISON | Status::BLIND | Status::SILENCE | Status::SLEEP | Status::PARALYSIS);
        }
    }

    SaveSystem::save_game("ff1_save.sav", save_data);
    return true;
}

bool MenuEngine::revive_at_clinic(PartyCharacter& hero, GameSaveData& save_data, uint32_t clinic_price) {
    if (save_data.gold < clinic_price) return false;
    if (hero.stats.hp > 0 && !(hero.status_ailments & (Status::DEATH | Status::STONE))) return false;

    save_data.gold -= clinic_price;
    hero.stats.hp = 1;
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
