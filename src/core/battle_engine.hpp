#ifndef BATTLE_ENGINE_HPP
#define BATTLE_ENGINE_HPP

#include "data/game_types.hpp"
#include "data/data_loader.hpp"
#include "engine/rng.hpp"
#include "state/save_system.hpp"
#include <vector>
#include <string>

namespace ff1 {

enum class ActionType {
    ATTACK,
    MAGIC,
    DRINK,
    ITEM,
    RUN
};

struct BattleAction {
    ActionType type = ActionType::ATTACK;
    size_t actor_index = 0;   // 0-3 for party, 4+ for monsters
    bool is_party = true;
    size_t target_index = 0;
    uint8_t spell_or_item_id = 0;
};

struct ActiveMonster {
    uint8_t enemy_id = 0;
    std::string name;
    uint16_t hp = 0;
    uint16_t max_hp = 0;
    uint8_t evade = 0;
    uint8_t absorb = 0;
    uint8_t num_hits = 1;
    uint8_t hit_rate = 0;
    uint8_t damage = 0;
    uint8_t crit_rate = 0;
    uint8_t mag_def = 0;
    uint8_t morale = 0;
    uint8_t ai_id = 0;
    uint8_t category = 0;
    uint8_t elem_weak = 0;
    uint8_t elem_resist = 0;
    uint8_t attack_ailment = 0;
    uint8_t status_ailments = 0;
    bool alive = true;
};

class BattleEngine {
public:
    BattleEngine(const DataLoader& loader, RNG& rng, bool enable_bug_fixes = true);

    void start_battle(GameSaveData& save_data, uint8_t formation_id);

    const std::vector<std::string>& get_log() const { return log_; }
    void clear_log() { log_.clear(); }

    bool is_battle_over() const { return battle_over_; }
    bool is_victory() const { return victory_; }
    bool is_escaped() const { return escaped_; }

    const std::vector<ActiveMonster>& get_monsters() const { return monsters_; }
    const BattleFormation& get_formation() const { return formation_; }

    // Process a full round of turns
    void process_turn(const std::array<BattleAction, 4>& party_actions, GameSaveData& save_data);

private:
    const DataLoader& loader_;
    RNG& rng_;
    bool bug_fixes_enabled_ = true;

    std::vector<ActiveMonster> monsters_;
    BattleFormation formation_;
    std::vector<std::string> log_;

    bool battle_over_ = false;
    bool victory_ = false;
    bool escaped_ = false;

    void log(const std::string& msg) { log_.push_back(msg); }

    void execute_attack(size_t attacker_idx, bool attacker_is_party, size_t target_idx, GameSaveData& save_data);
    void execute_magic(size_t caster_idx, bool caster_is_party, uint8_t spell_id, size_t target_idx, GameSaveData& save_data);
    void execute_item_use(size_t actor_idx, uint8_t item_id, size_t target_idx, GameSaveData& save_data);
    void execute_run(size_t actor_idx, GameSaveData& save_data);

    void process_enemy_ai_turn(size_t monster_idx, GameSaveData& save_data);

    void check_battle_end(GameSaveData& save_data);
    void distribute_rewards(GameSaveData& save_data);
};

} // namespace ff1

#endif // BATTLE_ENGINE_HPP
