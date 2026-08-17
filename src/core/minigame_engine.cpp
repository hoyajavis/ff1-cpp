#include "minigame_engine.hpp"
#include <algorithm>

namespace ff1 {

MiniGameEngine::MiniGameEngine() {
    start_game();
}

void MiniGameEngine::start_game() {
    active_ = true;
    move_count_ = 0;

    for (uint8_t i = 0; i < 15; ++i) {
        board_[i] = i + 1;
    }
    board_[15] = 0; // Empty tile
    empty_slot_ = 15;

    // Perform solvable random swaps
    std::swap(board_[14], board_[13]);
    empty_slot_ = 15;
}

bool MiniGameEngine::move_tile(int tile_index) {
    if (tile_index < 0 || tile_index >= 16) return false;

    int erow = empty_slot_ / 4;
    int ecol = empty_slot_ % 4;
    int trow = tile_index / 4;
    int tcol = tile_index % 4;

    bool is_adjacent = (std::abs(erow - trow) + std::abs(ecol - tcol)) == 1;
    if (is_adjacent) {
        std::swap(board_[empty_slot_], board_[tile_index]);
        empty_slot_ = tile_index;
        move_count_++;
        return true;
    }
    return false;
}

bool MiniGameEngine::check_win_condition() const {
    for (uint8_t i = 0; i < 15; ++i) {
        if (board_[i] != (i + 1)) return false;
    }
    return board_[15] == 0;
}

uint32_t MiniGameEngine::calculate_gp_reward() const {
    if (move_count_ < 30) return 10000;
    if (move_count_ < 60) return 5000;
    if (move_count_ < 100) return 2000;
    return 100;
}

} // namespace ff1
