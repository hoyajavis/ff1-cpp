#ifndef MINIGAME_ENGINE_HPP
#define MINIGAME_ENGINE_HPP

#include <array>
#include <cstdint>

namespace ff1 {

class MiniGameEngine {
public:
    MiniGameEngine();

    void start_game();
    bool move_tile(int tile_index);
    bool check_win_condition() const;

    uint32_t calculate_gp_reward() const;

    bool is_active() const { return active_; }
    void close_game() { active_ = false; }

    const std::array<uint8_t, 16>& get_board() const { return board_; }
    uint32_t get_move_count() const { return move_count_; }

private:
    std::array<uint8_t, 16> board_; // 1..15, 0 = empty slot
    int empty_slot_ = 15;
    uint32_t move_count_ = 0;
    bool active_ = false;
};

} // namespace ff1

#endif // MINIGAME_ENGINE_HPP
