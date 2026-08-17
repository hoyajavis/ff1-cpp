#include "battle_engine.hpp"
#include <algorithm>
#include <iostream>

namespace ff1 {

BattleEngine::BattleEngine(const DataLoader& loader, RNG& rng, bool enable_bug_fixes)
    : loader_(loader), rng_(rng), bug_fixes_enabled_(enable_bug_fixes) {}

void BattleEngine::start_battle(GameSaveData& save_data, uint8_t formation_id) {
    monsters_.clear();
    log_.clear();
    battle_over_ = false;
    victory_ = false;
    escaped_ = false;

    formation_ = loader_.get_formation(formation_id);

    for (int slot = 0; slot < 4; ++slot) {
        uint8_t enemy_id = formation_.enemy_ids[slot];
        uint8_t min_count = formation_.min_max_a[slot] & 0x0F;
        uint8_t max_count = (formation_.min_max_a[slot] >> 4) & 0x0F;

        if (enemy_id == 0xFF || max_count == 0) continue;

        int count = rng_.next_range(min_count, max_count);
        const auto& edata = loader_.get_enemy(enemy_id);

        for (int c = 0; c < count; ++c) {
            ActiveMonster m;
            m.enemy_id = enemy_id;
            m.name = edata.name + (count > 1 ? (" " + std::to_string(c + 1)) : "");
            m.hp = edata.hp_max;
            m.max_hp = edata.hp_max;
            m.evade = edata.evade;
            m.absorb = edata.absorb;
            m.num_hits = edata.num_hits;
            m.hit_rate = edata.hit_rate;
            m.damage = edata.damage;
            m.crit_rate = edata.crit_rate;
            m.mag_def = edata.mag_def;
            m.morale = edata.morale;
            m.ai_id = edata.ai_id;
            m.category = edata.category;
            m.elem_weak = edata.elem_weak;
            m.elem_resist = edata.elem_resist;
            m.attack_ailment = edata.attack_ailment;
            m.alive = true;
            monsters_.push_back(m);
        }
    }

    log("Enemies appeared!");
}

void BattleEngine::execute_attack(size_t attacker_idx, bool attacker_is_party, size_t target_idx, GameSaveData& save_data) {
    if (attacker_is_party) {
        auto& hero = save_data.party[attacker_idx];
        if (hero.stats.hp == 0 || (hero.status_ailments & (Status::DEATH | Status::PARALYSIS | Status::SLEEP | Status::STONE))) return;

        if (target_idx >= monsters_.size() || !monsters_[target_idx].alive) {
            auto it = std::find_if(monsters_.begin(), monsters_.end(), [](const ActiveMonster& m) { return m.alive; });
            if (it == monsters_.end()) return;
            target_idx = std::distance(monsters_.begin(), it);
        }

        auto& target = monsters_[target_idx];
        int num_hits = std::max(1, 1 + (hero.stats.hit_rate / 32));
        int total_damage = 0;
        int hits_landed = 0;

        for (int h = 0; h < num_hits; ++h) {
            int hit_chance = 168 + hero.stats.hit_rate - target.evade;
            if (hero.status_ailments & Status::BLIND) hit_chance -= 40;

            if (rng_.next_byte() <= hit_chance) {
                hits_landed++;
                int base_dmg = std::max(1, hero.stats.damage - target.absorb);
                int hit_dmg = base_dmg + (rng_.next_byte() % (base_dmg + 1));

                uint8_t crit_rate = bug_fixes_enabled_ ? hero.stats.crit_rate : hero.weapons[0];
                if (rng_.next_byte() <= crit_rate) {
                    hit_dmg += hero.stats.damage;
                }
                total_damage += hit_dmg;
            }
        }

        if (hits_landed > 0) {
            if (total_damage >= target.hp) {
                target.hp = 0;
                target.alive = false;
            } else {
                target.hp -= total_damage;
            }
            log(hero.name + " strikes " + target.name + " for " + std::to_string(total_damage) + " dmg!");
        } else {
            log(hero.name + " attacks " + target.name + "... Miss!");
        }
    } else { // Monster physical attack
        if (attacker_idx >= monsters_.size() || !monsters_[attacker_idx].alive) return;
        auto& monster = monsters_[attacker_idx];
        if (monster.status_ailments & (Status::DEATH | Status::PARALYSIS | Status::SLEEP | Status::STONE)) return;

        if (target_idx >= 4 || save_data.party[target_idx].stats.hp == 0) {
            for (size_t p = 0; p < 4; ++p) {
                if (save_data.party[p].stats.hp > 0) {
                    target_idx = p;
                    break;
                }
            }
        }

        auto& target = save_data.party[target_idx];
        if (target.stats.hp == 0) return;

        int hits_landed = 0;
        int total_damage = 0;
        for (int h = 0; h < monster.num_hits; ++h) {
            int hit_chance = 168 + monster.hit_rate - target.stats.evade;
            if (rng_.next_byte() <= hit_chance) {
                hits_landed++;
                int base_dmg = std::max(1, monster.damage - target.stats.absorb);
                int hit_dmg = base_dmg + (rng_.next_byte() % (base_dmg + 1));
                if (rng_.next_byte() <= monster.crit_rate) {
                    hit_dmg += monster.damage;
                }
                total_damage += hit_dmg;

                // Status ailment on hit check
                if (monster.attack_ailment != 0 && (rng_.next_byte() < 30)) {
                    target.status_ailments |= monster.attack_ailment;
                }
            }
        }

        if (hits_landed > 0) {
            if (total_damage >= target.stats.hp) {
                target.stats.hp = 0;
                target.status_ailments |= Status::DEATH;
            } else {
                target.stats.hp -= total_damage;
            }
            log(monster.name + " hits " + target.name + " for " + std::to_string(total_damage) + " dmg!");
        } else {
            log(monster.name + " attacks " + target.name + "... Miss!");
        }
    }
}

void BattleEngine::execute_magic(size_t caster_idx, bool caster_is_party, uint8_t spell_id, size_t target_idx, GameSaveData& save_data) {
    const auto& spell = loader_.get_spell(spell_id);

    if (caster_is_party) {
        auto& hero = save_data.party[caster_idx];
        if (hero.stats.hp == 0 || (hero.status_ailments & Status::SILENCE)) {
            log(hero.name + " cannot cast spells!");
            return;
        }

        log(hero.name + " casts spell!");

        // Healing / Revival Spells
        if (spell.effect == 0x01 || spell.effect == 0x02) { // CURE / HEAL
            int heal = spell.effectivity + (rng_.next_byte() % (spell.effectivity + 1));
            if (bug_fixes_enabled_) heal += (hero.stats.intelligence / 2);

            if (spell.target == 0x08) { // All Party
                for (auto& member : save_data.party) {
                    if (member.stats.hp > 0) {
                        member.stats.hp = std::min<uint16_t>(member.stats.max_hp, member.stats.hp + heal);
                    }
                }
                log("Party healed for " + std::to_string(heal) + " HP!");
            } else { // Single Hero
                if (target_idx < 4 && save_data.party[target_idx].stats.hp > 0) {
                    auto& t = save_data.party[target_idx];
                    t.stats.hp = std::min<uint16_t>(t.stats.max_hp, t.stats.hp + heal);
                    log(t.name + " healed for " + std::to_string(heal) + " HP!");
                }
            }
        } else { // Offensive Spells (FIRE, LIT, ICE, etc.)
            if (target_idx < monsters_.size() && monsters_[target_idx].alive) {
                auto& target = monsters_[target_idx];
                int hit_chance = 148 + spell.hit_rate - target.mag_def;
                bool saved = (rng_.next_byte() > hit_chance);

                int dmg = spell.effectivity + (rng_.next_byte() % (spell.effectivity + 1));
                if (bug_fixes_enabled_) dmg += (hero.stats.intelligence / 2);
                if (saved) dmg /= 2;

                if (dmg >= target.hp) {
                    target.hp = 0;
                    target.alive = false;
                } else {
                    target.hp -= dmg;
                }
                log("Spell hits " + target.name + " for " + std::to_string(dmg) + " dmg!");
            }
        }
    }
}

void BattleEngine::execute_item_use(size_t actor_idx, uint8_t item_id, size_t target_idx, GameSaveData& save_data) {
    (void)target_idx;
    auto& hero = save_data.party[actor_idx];
    if (hero.stats.hp == 0) return;

    if (item_id == 0) { // Potion / Heal
        int heal = 30;
        hero.stats.hp = std::min<uint16_t>(hero.stats.max_hp, hero.stats.hp + heal);
        log(hero.name + " used a Potion (+30 HP)!");
    } else if (item_id == 1) { // Antidote
        hero.status_ailments &= ~Status::POISON;
        log(hero.name + " used Antidote! Cured Poison!");
    }
}

void BattleEngine::execute_run(size_t actor_idx, GameSaveData& save_data) {
    if (formation_.no_run) {
        log("Cannot run from this battle!");
        return;
    }

    uint8_t run_val = rng_.next_byte();
    uint8_t luck = save_data.party[actor_idx].stats.luck;
    if (run_val <= (15 + luck)) {
        escaped_ = true;
        battle_over_ = true;
        log("Party ran away!");
    } else {
        log("Couldn't run!");
    }
}

void BattleEngine::process_enemy_ai_turn(size_t monster_idx, GameSaveData& save_data) {
    if (monster_idx >= monsters_.size() || !monsters_[monster_idx].alive) return;
    auto& monster = monsters_[monster_idx];
    const auto& ai = loader_.get_ai_script(monster.ai_id);

    uint8_t roll = rng_.next_byte() % 128;
    if (roll < ai.spell_chance && ai.spell_list[0] != 0xFF) {
        // Cast AI spell
        uint8_t spell_id = ai.spell_list[rng_.next_byte() % 8];
        if (spell_id != 0xFF) {
            execute_magic(monster_idx, false, spell_id, rng_.next_range(0, 3), save_data);
            return;
        }
    }

    // Default: physical attack
    execute_attack(monster_idx, false, rng_.next_range(0, 3), save_data);
}

void BattleEngine::process_turn(const std::array<BattleAction, 4>& party_actions, GameSaveData& save_data) {
    if (battle_over_) return;

    for (size_t i = 0; i < 4; ++i) {
        if (save_data.party[i].stats.hp > 0) {
            const auto& act = party_actions[i];
            if (act.type == ActionType::RUN) {
                execute_run(i, save_data);
                if (escaped_) return;
            } else if (act.type == ActionType::MAGIC) {
                execute_magic(i, true, act.spell_or_item_id, act.target_index, save_data);
            } else if (act.type == ActionType::ITEM) {
                execute_item_use(i, act.spell_or_item_id, act.target_index, save_data);
            } else {
                execute_attack(i, true, act.target_index, save_data);
            }
            check_battle_end(save_data);
            if (battle_over_) return;
        }
    }

    for (size_t m = 0; m < monsters_.size(); ++m) {
        if (monsters_[m].alive) {
            process_enemy_ai_turn(m, save_data);
            check_battle_end(save_data);
            if (battle_over_) return;
        }
    }
}

void BattleEngine::check_battle_end(GameSaveData& save_data) {
    bool any_monster_alive = std::any_of(monsters_.begin(), monsters_.end(), [](const ActiveMonster& m) { return m.alive; });
    if (!any_monster_alive) {
        victory_ = true;
        battle_over_ = true;
        log("Victory!");
        distribute_rewards(save_data);
        return;
    }

    bool any_hero_alive = std::any_of(save_data.party.begin(), save_data.party.end(), [](const PartyCharacter& c) { return c.stats.hp > 0; });
    if (!any_hero_alive) {
        victory_ = false;
        battle_over_ = true;
        log("Party defeated...");
    }
}

void BattleEngine::distribute_rewards(GameSaveData& save_data) {
    uint32_t total_exp = 0;
    uint32_t total_gp = 0;
    for (const auto& m : monsters_) {
        const auto& edata = loader_.get_enemy(m.enemy_id);
        total_exp += edata.exp;
        total_gp += edata.gp;
    }

    save_data.gold += total_gp;
    int alive_count = 0;
    for (const auto& c : save_data.party) {
        if (c.stats.hp > 0) alive_count++;
    }

    if (alive_count > 0) {
        uint32_t exp_per_hero = total_exp / alive_count;
        for (auto& c : save_data.party) {
            if (c.stats.hp > 0) {
                c.exp += exp_per_hero;
            }
        }
    }
    log("Gained " + std::to_string(total_exp) + " EXP & " + std::to_string(total_gp) + " GP!");
}

} // namespace ff1
