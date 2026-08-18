#include "battle_engine.hpp"
#include "core/menu_engine.hpp"
#include "engine/audio_engine.hpp"
#include <algorithm>
#include <iostream>

namespace ff1 {

BattleEngine::BattleEngine(const DataLoader& loader, RNG& rng, bool enable_bug_fixes)
    : loader_(loader), rng_(rng), bug_fixes_enabled_(enable_bug_fixes) {}

void BattleEngine::start_battle(GameSaveData& save_data, uint8_t formation_id) {
    monsters_.clear();
    log_.clear();
    current_narrative_.clear();
    turn_queue_.clear();
    pending_level_ups_.clear();
    current_level_up_ = LevelUpStatGains{};
    current_queue_idx_ = 0;
    battle_over_ = false;
    victory_ = false;
    escaped_ = false;
    chaos_cur4_used_ = false;

    formation_ = loader_.get_formation(formation_id);
    if (formation_id >= 0x70) {
        formation_.no_run = true;
    }

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

    // Surprise round determination
    uint8_t roll = rng_.next_byte();
    if (roll < 8) {
        surprise_ = SurpriseType::AMBUSH;
        log_msg("Monsters strike first!");
    } else if (roll < 20) {
        surprise_ = SurpriseType::PREEMPTIVE;
        log_msg("Chance to strike first!");
    } else {
        surprise_ = SurpriseType::NORMAL;
        log_msg("Enemies appeared!");
    }

    // Find first active hero
    active_hero_ = 0;
    while (active_hero_ < 4 && save_data.party[active_hero_].stats.hp == 0) {
        active_hero_++;
    }

    command_cursor_ = 0;
    target_cursor_ = 0;
    magic_tier_cursor_ = 0;
    magic_slot_cursor_ = 0;
    item_cursor_ = 0;

    for (size_t i = 0; i < 4; ++i) {
        planned_party_actions_[i] = BattleAction{ActionType::ATTACK, i, true, 0, 0};
    }

    state_ = BattleState::HERO_COMMAND_SELECT;
}

HeroPose BattleEngine::get_hero_pose(size_t hero_idx, const GameSaveData& save_data) const {
    if (hero_idx >= 4) return HeroPose::STANDING;
    const auto& hero = save_data.party[hero_idx];

    if (hero.stats.hp == 0 || (hero.status_ailments & (Status::DEATH | Status::STONE))) {
        return HeroPose::CROUCH;
    }
    if (hero.stats.hp <= hero.stats.max_hp / 4) {
        return HeroPose::CROUCH;
    }
    if (state_ == BattleState::HERO_COMMAND_SELECT || state_ == BattleState::TARGET_SELECT ||
        state_ == BattleState::SUBMENU_MAGIC || state_ == BattleState::SUBMENU_ITEM) {
        if (hero_idx == active_hero_) {
            return HeroPose::STEP_FORWARD;
        }
    }
    if (state_ == BattleState::ACTION_ANIMATION && current_queue_idx_ < turn_queue_.size()) {
        const auto& act = turn_queue_[current_queue_idx_];
        if (act.is_party && act.actor_idx == hero_idx) {
            return HeroPose::ATTACK_SWING;
        }
    }
    return HeroPose::STANDING;
}

void BattleEngine::handle_battle_input(InputKey key, GameSaveData& save_data, AudioEngine& audio) {
    if (state_ == BattleState::HERO_COMMAND_SELECT) {
        if (key == InputKey::UP) {
            if (command_cursor_ > 0) {
                command_cursor_--;
                audio.play_sfx(SoundEffect::SELECT);
            }
        } else if (key == InputKey::DOWN) {
            if (command_cursor_ < 4) {
                command_cursor_++;
                audio.play_sfx(SoundEffect::SELECT);
            }
        } else if (key == InputKey::CANCEL) {
            // Step back to previous living hero
            if (active_hero_ > 0) {
                int prev = active_hero_ - 1;
                while (prev >= 0 && save_data.party[prev].stats.hp == 0) {
                    prev--;
                }
                if (prev >= 0) {
                    active_hero_ = prev;
                    audio.play_sfx(SoundEffect::SELECT);
                }
            }
        } else if (key == InputKey::CONFIRM) {
            audio.play_sfx(SoundEffect::SELECT);
            if (command_cursor_ == 0) { // FIGHT
                state_ = BattleState::TARGET_SELECT;
                target_cursor_ = 0;
                while (target_cursor_ < monsters_.size() && !monsters_[target_cursor_].alive) {
                    target_cursor_++;
                }
            } else if (command_cursor_ == 1) { // MAGIC
                state_ = BattleState::SUBMENU_MAGIC;
                magic_tier_cursor_ = 0;
                magic_slot_cursor_ = 0;
            } else if (command_cursor_ == 2 || command_cursor_ == 3) { // DRINK / ITEM
                state_ = BattleState::SUBMENU_ITEM;
                item_cursor_ = 0;
            } else if (command_cursor_ == 4) { // RUN
                planned_party_actions_[active_hero_] = BattleAction{ActionType::RUN, active_hero_, true, 0, 0};
                // Advance to next living hero
                active_hero_++;
                while (active_hero_ < 4 && save_data.party[active_hero_].stats.hp == 0) {
                    active_hero_++;
                }
                if (active_hero_ >= 4) {
                    build_initiative_queue(save_data);
                    state_ = BattleState::ROUND_EXECUTION;
                }
            }
        }
    } else if (state_ == BattleState::TARGET_SELECT) {
        if (key == InputKey::UP || key == InputKey::LEFT) {
            if (target_cursor_ > 0) {
                target_cursor_--;
                while (target_cursor_ > 0 && !monsters_[target_cursor_].alive) {
                    target_cursor_--;
                }
                audio.play_sfx(SoundEffect::SELECT);
            }
        } else if (key == InputKey::DOWN || key == InputKey::RIGHT) {
            if (target_cursor_ + 1 < monsters_.size()) {
                target_cursor_++;
                while (target_cursor_ + 1 < monsters_.size() && !monsters_[target_cursor_].alive) {
                    target_cursor_++;
                }
                audio.play_sfx(SoundEffect::SELECT);
            }
        } else if (key == InputKey::CANCEL) {
            state_ = BattleState::HERO_COMMAND_SELECT;
            audio.play_sfx(SoundEffect::SELECT);
        } else if (key == InputKey::CONFIRM) {
            audio.play_sfx(SoundEffect::SELECT);
            planned_party_actions_[active_hero_] = BattleAction{ActionType::ATTACK, active_hero_, true, target_cursor_, 0};

            // Advance to next living hero
            active_hero_++;
            while (active_hero_ < 4 && save_data.party[active_hero_].stats.hp == 0) {
                active_hero_++;
            }
            if (active_hero_ >= 4) {
                build_initiative_queue(save_data);
                state_ = BattleState::ROUND_EXECUTION;
            } else {
                state_ = BattleState::HERO_COMMAND_SELECT;
                command_cursor_ = 0;
            }
        }
    } else if (state_ == BattleState::SUBMENU_MAGIC) {
        if (key == InputKey::UP) {
            if (magic_tier_cursor_ > 0) magic_tier_cursor_--;
        } else if (key == InputKey::DOWN) {
            if (magic_tier_cursor_ < 7) magic_tier_cursor_++;
        } else if (key == InputKey::LEFT) {
            if (magic_slot_cursor_ > 0) magic_slot_cursor_--;
        } else if (key == InputKey::RIGHT) {
            if (magic_slot_cursor_ < 2) magic_slot_cursor_++;
        } else if (key == InputKey::CANCEL) {
            state_ = BattleState::HERO_COMMAND_SELECT;
            audio.play_sfx(SoundEffect::SELECT);
        } else if (key == InputKey::CONFIRM) {
            auto& hero = save_data.party[active_hero_];
            uint8_t spell_id = hero.spells[magic_tier_cursor_][magic_slot_cursor_];
            if (spell_id != 0xFF && hero.stats.mp[magic_tier_cursor_] > 0) {
                audio.play_sfx(SoundEffect::SELECT);
                planned_party_actions_[active_hero_] = BattleAction{ActionType::MAGIC, active_hero_, true, target_cursor_, spell_id};
                state_ = BattleState::TARGET_SELECT;
            }
        }
    } else if (state_ == BattleState::SUBMENU_ITEM) {
        if (key == InputKey::UP) {
            if (item_cursor_ > 0) item_cursor_--;
        } else if (key == InputKey::DOWN) {
            if (item_cursor_ < 3) item_cursor_++;
        } else if (key == InputKey::CANCEL) {
            state_ = BattleState::HERO_COMMAND_SELECT;
            audio.play_sfx(SoundEffect::SELECT);
        } else if (key == InputKey::CONFIRM) {
            audio.play_sfx(SoundEffect::SELECT);
            planned_party_actions_[active_hero_] = BattleAction{ActionType::ITEM, active_hero_, true, target_cursor_, item_cursor_};
            active_hero_++;
            while (active_hero_ < 4 && save_data.party[active_hero_].stats.hp == 0) {
                active_hero_++;
            }
            if (active_hero_ >= 4) {
                build_initiative_queue(save_data);
                state_ = BattleState::ROUND_EXECUTION;
            } else {
                state_ = BattleState::HERO_COMMAND_SELECT;
                command_cursor_ = 0;
            }
        }
    } else if (state_ == BattleState::VICTORY_SUMMARY) {
        if (key == InputKey::CONFIRM) {
            if (!pending_level_ups_.empty()) {
                current_level_up_ = pending_level_ups_.front();
                pending_level_ups_.erase(pending_level_ups_.begin());
                state_ = BattleState::LEVEL_UP;
                audio.play_sfx(SoundEffect::LEVEL_UP);
            } else {
                state_ = BattleState::COMPLETE;
                battle_over_ = true;
            }
        }
    } else if (state_ == BattleState::LEVEL_UP) {
        if (key == InputKey::CONFIRM) {
            if (!pending_level_ups_.empty()) {
                current_level_up_ = pending_level_ups_.front();
                pending_level_ups_.erase(pending_level_ups_.begin());
                audio.play_sfx(SoundEffect::LEVEL_UP);
            } else {
                state_ = BattleState::COMPLETE;
                battle_over_ = true;
            }
        }
    }
}

void BattleEngine::build_initiative_queue(GameSaveData& save_data) {
    turn_queue_.clear();
    current_queue_idx_ = 0;

    // Party Members Initiative
    for (size_t i = 0; i < 4; ++i) {
        if (save_data.party[i].stats.hp > 0) {
            int init = static_cast<int>(save_data.party[i].stats.agility) - (rng_.next_byte() % 51);
            turn_queue_.push_back(TurnQueueEntry{true, i, init, planned_party_actions_[i]});
        }
    }

    // Monsters Initiative (if not preemptive round)
    if (surprise_ != SurpriseType::PREEMPTIVE) {
        for (size_t m = 0; m < monsters_.size(); ++m) {
            if (monsters_[m].alive) {
                int init = static_cast<int>(monsters_[m].evade) - (rng_.next_byte() % 51);
                turn_queue_.push_back(TurnQueueEntry{false, m, init, BattleAction{ActionType::ATTACK, m, false, 0, 0}});
            }
        }
    }

    // Sort Queue by Initiative Descending
    std::sort(turn_queue_.begin(), turn_queue_.end(), [](const TurnQueueEntry& a, const TurnQueueEntry& b) {
        return a.initiative > b.initiative;
    });
}

bool BattleEngine::step_combat_turn(GameSaveData& save_data, AudioEngine& audio) {
    if (battle_over_ || state_ == BattleState::VICTORY_SUMMARY || state_ == BattleState::LEVEL_UP || state_ == BattleState::COMPLETE) {
        return false;
    }

    if (current_queue_idx_ >= turn_queue_.size()) {
        // Round Complete -> Reset for Next Round
        check_battle_end(save_data);
        if (battle_over_) return false;

        active_hero_ = 0;
        while (active_hero_ < 4 && save_data.party[active_hero_].stats.hp == 0) {
            active_hero_++;
        }
        command_cursor_ = 0;
        target_cursor_ = 0;
        state_ = BattleState::HERO_COMMAND_SELECT;
        return false;
    }

    const auto& entry = turn_queue_[current_queue_idx_++];
    execute_queue_entry(entry, save_data, &audio);
    check_battle_end(save_data);
    return true;
}

void BattleEngine::execute_queue_entry(const TurnQueueEntry& entry, GameSaveData& save_data, AudioEngine* audio) {
    (void)audio;
    if (entry.is_party) {
        if (entry.action.type == ActionType::RUN) {
            execute_run(entry.actor_idx, save_data);
        } else if (entry.action.type == ActionType::MAGIC) {
            execute_magic(entry.actor_idx, true, entry.action.spell_or_item_id, entry.action.target_index, save_data);
        } else if (entry.action.type == ActionType::ITEM) {
            execute_item_use(entry.actor_idx, entry.action.spell_or_item_id, entry.action.target_index, save_data);
        } else {
            execute_attack(entry.actor_idx, true, entry.action.target_index, save_data);
        }
    } else {
        process_enemy_ai_turn(entry.actor_idx, save_data);
    }
}

void BattleEngine::execute_attack(size_t attacker_idx, bool attacker_is_party, size_t target_idx, GameSaveData& save_data) {
    if (attacker_is_party) {
        auto& hero = save_data.party[attacker_idx];
        if (hero.stats.hp == 0 || (hero.status_ailments & (Status::DEATH | Status::STONE))) return;

        // Paralysis check
        if (hero.status_ailments & Status::PARALYSIS) {
            if (rng_.next_byte() < 64) {
                hero.status_ailments &= ~Status::PARALYSIS;
                log_msg(hero.name + " overcame paralysis!");
            } else {
                log_msg(hero.name + " is paralyzed and cannot move!");
                return;
            }
        }

        // Sleep check
        if (hero.status_ailments & Status::SLEEP) {
            log_msg(hero.name + " is asleep!");
            return;
        }

        // Ineffective Target Check
        if (target_idx >= monsters_.size() || !monsters_[target_idx].alive) {
            log_msg(hero.name + " attacks... Ineffective!");
            return;
        }

        auto& target = monsters_[target_idx];
        int num_hits = std::max(1, 1 + (hero.stats.hit_rate / 32));
        if (target.fast_buff) num_hits *= 2;

        int total_damage = 0;
        int hits_landed = 0;
        bool is_crit = false;

        for (int h = 0; h < num_hits; ++h) {
            int hit_chance = 168 + hero.stats.hit_rate - target.evade;
            if (hero.status_ailments & Status::BLIND) hit_chance -= 40;

            if (rng_.next_byte() <= hit_chance) {
                hits_landed++;
                int base_dmg = std::max(1, hero.stats.damage - (target.absorb + target.absorb_buff));
                int hit_dmg = base_dmg + (rng_.next_byte() % (base_dmg + 1));

                uint8_t crit_rate = bug_fixes_enabled_ ? hero.stats.crit_rate : hero.weapons[0];
                if (rng_.next_byte() <= crit_rate) {
                    hit_dmg += hero.stats.damage;
                    is_crit = true;
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
            std::string crit_str = is_crit ? " Critical hit!! " : " ";
            log_msg(hero.name + " strikes " + target.name + "!" + crit_str + std::to_string(total_damage) + " dmg (" + std::to_string(hits_landed) + " Hits)");
        } else {
            log_msg(hero.name + " attacks " + target.name + "... Miss!");
        }
    } else { // Monster physical attack
        if (attacker_idx >= monsters_.size() || !monsters_[attacker_idx].alive) return;
        auto& monster = monsters_[attacker_idx];
        if (monster.status_ailments & (Status::DEATH | Status::STONE)) return;

        if (monster.status_ailments & Status::PARALYSIS) {
            if (rng_.next_byte() < 64) {
                monster.status_ailments &= ~Status::PARALYSIS;
                log_msg(monster.name + " overcame paralysis!");
            } else {
                log_msg(monster.name + " is paralyzed!");
                return;
            }
        }

        if (monster.status_ailments & Status::SLEEP) {
            log_msg(monster.name + " is asleep!");
            return;
        }

        // Chaos CUR4 emergency recovery trigger
        if ((monster.name.find("CHAOS") != std::string::npos || monster.name.find("Chaos") != std::string::npos || monster.enemy_id == 0x7F) && monster.hp <= 500 && !chaos_cur4_used_) {
            monster.hp = 2000;
            chaos_cur4_used_ = true;
            log_msg("Chaos casts CUR4! HP fully restored!");
            return;
        }

        // Target active hero
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
            log_msg(monster.name + " hits " + target.name + " for " + std::to_string(total_damage) + " dmg!");
        } else {
            log_msg(monster.name + " attacks " + target.name + "... Miss!");
        }
    }
}

void BattleEngine::execute_magic(size_t caster_idx, bool caster_is_party, uint8_t spell_id, size_t target_idx, GameSaveData& save_data) {
    const auto& spell = loader_.get_spell(spell_id);

    if (caster_is_party) {
        auto& hero = save_data.party[caster_idx];
        if (hero.stats.hp == 0 || (hero.status_ailments & Status::SILENCE)) {
            log_msg(hero.name + " is muted and cannot cast spells!");
            return;
        }

        // Deduct Tier MP
        uint8_t tier = magic_tier_cursor_;
        if (hero.stats.mp[tier] > 0) {
            hero.stats.mp[tier] -= 1;
        }

        // 1. Healing / Recovery
        if (spell.effect == 0x01 || spell.effect == 0x02) { // CURE / HEAL
            int heal = spell.effectivity + (rng_.next_byte() % (spell.effectivity + 1));
            if (bug_fixes_enabled_) heal += (hero.stats.intelligence / 2);

            if (spell.target == 0x08) { // All Party
                for (auto& member : save_data.party) {
                    if (member.stats.hp > 0) {
                        member.stats.hp = std::min<uint16_t>(member.stats.max_hp, member.stats.hp + heal);
                    }
                }
                log_msg(hero.name + " casts HEAL! Party recovered " + std::to_string(heal) + " HP!");
            } else { // Single Hero
                if (target_idx < 4 && save_data.party[target_idx].stats.hp > 0) {
                    auto& t = save_data.party[target_idx];
                    t.stats.hp = std::min<uint16_t>(t.stats.max_hp, t.stats.hp + heal);
                    log_msg(hero.name + " casts CURE! " + t.name + " recovered " + std::to_string(heal) + " HP!");
                }
            }
        } else if (spell.effect == 0x07) { // FAST
            if (target_idx < monsters_.size() && monsters_[target_idx].alive) {
                monsters_[target_idx].fast_buff = true;
                log_msg(hero.name + " casts FAST!");
            }
        } else if (spell.effect == 0x05) { // FOG (+8 Absorb)
            if (target_idx < 4) {
                save_data.party[target_idx].stats.absorb += 8;
                log_msg(hero.name + " casts FOG! Absorb increased.");
            }
        } else if (spell.effect == 0x06) { // RUSE (+80 Evade)
            hero.stats.evade += 80;
            log_msg(hero.name + " casts RUSE! Evade sharply boosted.");
        } else { // Offensive Spells (FIRE, LIT, ICE, HARM)
            if (target_idx < monsters_.size() && monsters_[target_idx].alive) {
                auto& target = monsters_[target_idx];

                // Check Undead Vulnerability for HARM
                if (spell.element & 0x04) { // Dia/Harm series
                    if (!(target.category & 0x01)) { // Not undead
                        log_msg(hero.name + " casts HARM... Ineffective!");
                        return;
                    }
                }

                int hit_chance = 148 + spell.hit_rate - target.mag_def;
                bool saved = (rng_.next_byte() > hit_chance);

                int dmg = spell.effectivity + (rng_.next_byte() % (spell.effectivity + 1));
                if (bug_fixes_enabled_) dmg += (hero.stats.intelligence / 2);

                // Element weakness amplify (1.5x)
                if (target.elem_weak & spell.element) dmg = (dmg * 3) / 2;
                if (target.elem_resist & spell.element) dmg /= 2;

                if (saved) dmg /= 2;

                if (dmg >= target.hp) {
                    target.hp = 0;
                    target.alive = false;
                } else {
                    target.hp -= dmg;
                }
                log_msg(hero.name + " casts spell on " + target.name + " for " + std::to_string(dmg) + " dmg!");
            } else {
                log_msg(hero.name + " casts spell... Ineffective!");
            }
        }
    } else { // Monster casting spell
        if (caster_idx >= monsters_.size() || !monsters_[caster_idx].alive) return;
        auto& monster = monsters_[caster_idx];

        if (target_idx >= 4 || save_data.party[target_idx].stats.hp == 0) {
            for (size_t p = 0; p < 4; ++p) {
                if (save_data.party[p].stats.hp > 0) {
                    target_idx = p;
                    break;
                }
            }
        }
        auto& target = save_data.party[target_idx];
        int dmg = spell.effectivity + (rng_.next_byte() % (spell.effectivity + 1));

        if (dmg >= target.stats.hp) {
            target.stats.hp = 0;
            target.status_ailments |= Status::DEATH;
        } else {
            target.stats.hp -= dmg;
        }
        log_msg(monster.name + " casts " + spell.name + " on " + target.name + " for " + std::to_string(dmg) + " dmg!");
    }
}

void BattleEngine::execute_item_use(size_t actor_idx, uint8_t item_id, size_t target_idx, GameSaveData& save_data) {
    (void)target_idx;
    auto& hero = save_data.party[actor_idx];
    if (hero.stats.hp == 0) return;

    if (item_id == 0) { // Heal Potion
        if (save_data.consumables.heal_potions > 0) {
            save_data.consumables.heal_potions--;
            int heal = 30;
            hero.stats.hp = std::min<uint16_t>(hero.stats.max_hp, hero.stats.hp + heal);
            log_msg(hero.name + " drank a Potion (+30 HP)!");
        }
    } else if (item_id == 1) { // Pure Potion
        if (save_data.consumables.pure_potions > 0) {
            save_data.consumables.pure_potions--;
            hero.status_ailments &= ~Status::POISON;
            log_msg(hero.name + " drank Pure Potion! Poison neutralized.");
        }
    }
}

void BattleEngine::execute_run(size_t actor_idx, GameSaveData& save_data) {
    if (formation_.no_run) {
        log_msg("Can't run from this encounter!");
        return;
    }

    uint8_t run_val = rng_.next_byte();
    uint8_t luck = save_data.party[actor_idx].stats.luck;
    uint8_t level = save_data.party[actor_idx].level;

    if (luck > (run_val % (level + 15 + 1))) {
        escaped_ = true;
        battle_over_ = true;
        state_ = BattleState::COMPLETE;
        log_msg("Close call..... Party fled!");
    } else {
        log_msg(save_data.party[actor_idx].name + " tried to flee... Can't run!");
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

void BattleEngine::check_battle_end(GameSaveData& save_data) {
    bool any_monster_alive = std::any_of(monsters_.begin(), monsters_.end(), [](const ActiveMonster& m) { return m.alive; });
    if (!any_monster_alive) {
        victory_ = true;
        battle_over_ = true;
        state_ = BattleState::VICTORY_SUMMARY;
        log_msg("Monsters perished! Victory!");
        distribute_rewards(save_data);
        prepare_level_ups(save_data);
        return;
    }

    bool any_hero_alive = std::any_of(save_data.party.begin(), save_data.party.end(), [](const PartyCharacter& c) { return c.stats.hp > 0; });
    if (!any_hero_alive) {
        victory_ = false;
        battle_over_ = true;
        state_ = BattleState::GAME_OVER;
        log_msg("Party perished in battle...");
    }
}

void BattleEngine::distribute_rewards(GameSaveData& save_data) {
    reward_exp_ = 0;
    reward_gp_ = 0;
    for (const auto& m : monsters_) {
        const auto& edata = loader_.get_enemy(m.enemy_id);
        reward_exp_ += edata.exp;
        reward_gp_ += edata.gp;
    }

    save_data.gold += reward_gp_;
    int alive_count = 0;
    for (const auto& c : save_data.party) {
        if (c.stats.hp > 0) alive_count++;
    }

    if (alive_count > 0) {
        uint32_t exp_per_hero = reward_exp_ / alive_count;
        for (auto& c : save_data.party) {
            if (c.stats.hp > 0) {
                c.exp += exp_per_hero;
            }
        }
    }
}

LevelUpStatGains BattleEngine::check_and_apply_level_up(PartyCharacter& hero, RNG& rng, uint8_t hero_idx) {
    LevelUpStatGains gains;
    gains.hero_idx = hero_idx;

    uint32_t next_req = MenuEngine::get_exp_for_level(hero.level + 1);
    if (hero.exp >= next_req && hero.level < 50) {
        gains.leveled_up = true;
        hero.level++;
        gains.new_level = hero.level;
        gains.messages.push_back("Lev. up! " + hero.name + " L" + std::to_string(hero.level));

        // HP Gain Calculation
        uint16_t hp_boost = 0;
        if (hero.class_type == ClassType::WARRIOR || hero.class_type == ClassType::KNIGHT ||
            hero.class_type == ClassType::MONK || hero.class_type == ClassType::MASTER) {
            hp_boost = 20 + (rng.next_byte() % 7); // 20-26 HP
        } else {
            hp_boost = 10 + (rng.next_byte() % 6); // 10-15 HP
        }
        hero.stats.max_hp += hp_boost;
        hero.stats.hp += hp_boost;
        gains.hp_gain = hp_boost;
        gains.messages.push_back("HP max +" + std::to_string(hp_boost) + "pts.");

        // Stat Growth
        if (rng.next_byte() < 180) { hero.stats.strength += 1; gains.str_gain = 1; gains.messages.push_back("Str. up"); }
        if (rng.next_byte() < 180) { hero.stats.agility += 1; gains.agi_gain = 1; gains.messages.push_back("Agi. up"); }
        if (rng.next_byte() < 180) { hero.stats.intelligence += 1; gains.int_gain = 1; gains.messages.push_back("Int. up"); }
        if (rng.next_byte() < 180) { hero.stats.vitality += 1; gains.vit_gain = 1; gains.messages.push_back("Vit. up"); }
        if (rng.next_byte() < 180) { hero.stats.luck += 1; gains.luck_gain = 1; gains.messages.push_back("Luck up"); }

        // MP Tier Growth for Mages
        if (hero.class_type == ClassType::WHITE_MAGE || hero.class_type == ClassType::WHITE_WIZARD ||
            hero.class_type == ClassType::BLACK_MAGE || hero.class_type == ClassType::BLACK_WIZARD ||
            hero.class_type == ClassType::RED_MAGE || hero.class_type == ClassType::RED_WIZARD) {
            size_t tier = (hero.level - 1) / 3;
            if (tier < 8 && hero.stats.max_mp[tier] < 9) {
                hero.stats.max_mp[tier] += 1;
                hero.stats.mp[tier] += 1;
            }
        }
    }
    return gains;
}

void BattleEngine::prepare_level_ups(GameSaveData& save_data) {
    pending_level_ups_.clear();
    for (size_t i = 0; i < 4; ++i) {
        if (save_data.party[i].stats.hp > 0) {
            LevelUpStatGains gains = check_and_apply_level_up(save_data.party[i], rng_, static_cast<uint8_t>(i));
            if (gains.leveled_up) {
                pending_level_ups_.push_back(gains);
            }
        }
    }
}

void BattleEngine::process_turn(const std::array<BattleAction, 4>& party_actions, GameSaveData& save_data) {
    if (battle_over_) return;
    planned_party_actions_ = party_actions;
    build_initiative_queue(save_data);

    for (const auto& entry : turn_queue_) {
        execute_queue_entry(entry, save_data);
        check_battle_end(save_data);
        if (battle_over_) return;
    }
}

} // namespace ff1
