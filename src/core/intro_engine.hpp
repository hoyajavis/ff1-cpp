#ifndef INTRO_ENGINE_HPP
#define INTRO_ENGINE_HPP

#include "data/game_types.hpp"
#include "state/save_system.hpp"
#include <array>
#include <string>

namespace ff1 {

enum class IntroState {
    INTRO_STORY,
    TITLE_SCREEN,
    PARTY_CREATION_CLASS,
    PARTY_CREATION_NAME,
    COMPLETE
};

enum class IntroAction {
    NONE,
    START_NEW_GAME,
    CONTINUE_GAME,
    SOUND_SEL,
    SOUND_MOVE
};

struct PartyCreationSlot {
    uint8_t class_id = 0; // 0: Fighter, 1: Thief, 2: Bl.Belt, 3: RedMage, 4: Wh.Mage, 5: Bl.Mage
    std::string name = ""; // Up to 4 characters
};

class IntroEngine {
public:
    IntroEngine();

    // State Accessors
    IntroState get_state() const { return state_; }
    void set_state(IntroState s) { state_ = s; }

    uint8_t get_title_cursor() const { return title_cursor_; }
    uint8_t get_respond_rate() const { return respond_rate_ + 1; } // Returns 1..8
    uint8_t get_respond_rate_raw() const { return respond_rate_; } // Returns 0..7

    uint8_t get_active_slot() const { return active_slot_; }
    const PartyCreationSlot& get_slot(size_t index) const;

    uint8_t get_kb_cursor_x() const { return kb_cursor_x_; }
    uint8_t get_kb_cursor_y() const { return kb_cursor_y_; }

    char get_kb_char_at(uint8_t x, uint8_t y) const;
    static const char* get_class_name(uint8_t class_id);
    static ClassType get_class_type(uint8_t class_id);

    // Input Processing
    IntroAction handle_input(InputKey key, bool has_save_file);
    IntroAction type_char(char ch);

    // Reset / Initialization
    void reset_to_title();
    void reset_party_creation();

    // Finalize Party into GameSaveData
    GameSaveData finalize_party() const;

    // Static helper for direct generation
    static GameSaveData create_new_game_party(
        const std::array<ClassType, 4>& classes,
        const std::array<std::string, 4>& names
    );

private:
    IntroState state_ = IntroState::TITLE_SCREEN;

    // Title Screen State
    uint8_t title_cursor_ = 1; // 0: Continue, 1: New Game
    uint8_t respond_rate_ = 3; // 0..7 (displays 1..8, default 4)

    // Party Creation State
    uint8_t active_slot_ = 0;  // 0..3
    std::array<PartyCreationSlot, 4> slots_;

    // Virtual Keyboard State
    uint8_t kb_cursor_x_ = 0;  // 0..9
    uint8_t kb_cursor_y_ = 0;  // 0..6
};

} // namespace ff1

#endif // INTRO_ENGINE_HPP
