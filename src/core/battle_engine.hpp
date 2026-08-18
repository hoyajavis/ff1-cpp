#ifndef BATTLE_ENGINE_HPP
#define BATTLE_ENGINE_HPP

#include "data/game_types.hpp"
#include "data/data_loader.hpp"
#include "engine/rng.hpp"
#include "state/save_system.hpp"
#include "core/intro_engine.hpp" // for InputKey
#include <vector>
#include <string>
#include <array>
#include <cstdint>

namespace ff1 {

class AudioEngine;

enum class BattleState {
    SURPRISE_CHECK,
    HERO_COMMAND_SELECT,
    TARGET_SELECT,
    SUBMENU_MAGIC,
    SUBMENU_ITEM,
    ROUND_EXECUTION,
    ACTION_ANIMATION,
    VICTORY_SUMMARY,
    LEVEL_UP,
    GAME_OVER,
    COMPLETE
};

enum class SurpriseType {
    NORMAL,
    PREEMPTIVE,
    AMBUSH
};

enum class HeroPose {
    STANDING,
    STEP_FORWARD,
    ATTACK_SWING,
    CROUCH
};

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

struct TurnQueueEntry {
    bool is_party = true;
    size_t actor_idx = 0;
    int initiative = 0;
    BattleAction action;
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
    bool fast_buff = false;
    uint8_t absorb_buff = 0;
    uint8_t evade_buff = 0;
    uint8_t damage_buff = 0;
    uint8_t hit_buff = 0;
};

struct LevelUpStatGains {
    bool leveled_up = false;
    uint8_t hero_idx = 0;
    uint8_t new_level = 1;
    uint16_t hp_gain = 0;
    uint8_t str_gain = 0;
    uint8_t agi_gain = 0;
    uint8_t int_gain = 0;
    uint8_t vit_gain = 0;
    uint8_t luck_gain = 0;
    std::vector<std::string> messages;
};

class BattleEngine {
public:
    BattleEngine(const DataLoader& loader, RNG& rng, bool enable_bug_fixes = true);

    void start_battle(GameSaveData& save_data, uint8_t formation_id);

    // Interactive Turn Input Dispatcher
    void handle_battle_input(InputKey key, GameSaveData& save_data, AudioEngine& audio);
    bool step_combat_turn(GameSaveData& save_data, AudioEngine& audio);

    // State Accessors
    BattleState get_state() const { return state_; }
    void set_state(BattleState st) { state_ = st; }
    SurpriseType get_surprise() const { return surprise_; }
    uint8_t get_active_hero() const { return active_hero_; }
    uint8_t get_command_cursor() const { return command_cursor_; }
    uint8_t get_target_cursor() const { return target_cursor_; }
    uint8_t get_magic_tier_cursor() const { return magic_tier_cursor_; }
    uint8_t get_magic_slot_cursor() const { return magic_slot_cursor_; }
    uint8_t get_item_cursor() const { return item_cursor_; }

    HeroPose get_hero_pose(size_t hero_idx, const GameSaveData& save_data) const;
    const LevelUpStatGains& get_current_level_up() const { return current_level_up_; }
    uint32_t get_reward_exp() const { return reward_exp_; }
    uint32_t get_reward_gp() const { return reward_gp_; }
    const std::string& get_current_narrative() const { return current_narrative_; }

    const std::vector<std::string>& get_log() const { return log_; }
    void clear_log() { log_.clear(); }

    bool is_battle_over() const { return battle_over_; }
    bool is_victory() const { return victory_; }
    bool is_escaped() const { return escaped_; }

    const std::vector<ActiveMonster>& get_monsters() const { return monsters_; }
    std::vector<ActiveMonster>& get_monsters_mut() { return monsters_; }
    const BattleFormation& get_formation() const { return formation_; }

    // Legacy full-round process for automated tests
    void process_turn(const std::array<BattleAction, 4>& party_actions, GameSaveData& save_data);

    // Level-Up Stat Calculation
    static LevelUpStatGains check_and_apply_level_up(PartyCharacter& hero, RNG& rng, uint8_t hero_idx = 0);

private:
    const DataLoader& loader_;
    RNG& rng_;
    bool bug_fixes_enabled_ = true;

    BattleState state_ = BattleState::HERO_COMMAND_SELECT;
    SurpriseType surprise_ = SurpriseType::NORMAL;

    std::vector<ActiveMonster> monsters_;
    BattleFormation formation_;
    std::vector<std::string> log_;
    std::string current_narrative_;

    bool battle_over_ = false;
    bool victory_ = false;
    bool escaped_ = false;

    // Interactive input cursors
    uint8_t active_hero_ = 0;
    uint8_t command_cursor_ = 0;    // 0: FIGHT, 1: MAGIC, 2: DRINK, 3: ITEM, 4: RUN
    uint8_t target_cursor_ = 0;     // 0..monsters.size()-1
    uint8_t magic_tier_cursor_ = 0; // 0..7
    uint8_t magic_slot_cursor_ = 0; // 0..2
    uint8_t item_cursor_ = 0;       // 0..3

    std::array<BattleAction, 4> planned_party_actions_;
    std::vector<TurnQueueEntry> turn_queue_;
    size_t current_queue_idx_ = 0;
    bool chaos_cur4_used_ = false;

    // Victory & Rewards
    uint32_t reward_exp_ = 0;
    uint32_t reward_gp_ = 0;
    std::vector<LevelUpStatGains> pending_level_ups_;
    LevelUpStatGains current_level_up_;

    void log_msg(const std::string& msg) {
        log_.push_back(msg);
        current_narrative_ = msg;
    }

    void build_initiative_queue(GameSaveData& save_data);
    void execute_queue_entry(const TurnQueueEntry& entry, GameSaveData& save_data, AudioEngine* audio = nullptr);

    void execute_attack(size_t attacker_idx, bool attacker_is_party, size_t target_idx, GameSaveData& save_data);
    void execute_magic(size_t caster_idx, bool caster_is_party, uint8_t spell_id, size_t target_idx, GameSaveData& save_data);
    void execute_item_use(size_t actor_idx, uint8_t item_id, size_t target_idx, GameSaveData& save_data);
    void execute_run(size_t actor_idx, GameSaveData& save_data);

    void process_enemy_ai_turn(size_t monster_idx, GameSaveData& save_data);
    void check_battle_end(GameSaveData& save_data);
    void distribute_rewards(GameSaveData& save_data);
    void prepare_level_ups(GameSaveData& save_data);
};

} // namespace ff1

#endif // BATTLE_ENGINE_HPP
