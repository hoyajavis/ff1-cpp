#include "intro_engine.hpp"

namespace ff1 {

// 7 rows x 10 cols matching authentic NES lut_NameInput in bank_0E.asm
static const char lut_NameInputGrid[7][10] = {
    {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'},
    {'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T'},
    {'U', 'V', 'W', 'X', 'Y', 'Z', ',', '.', ' ', ' '},
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'},
    {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'},
    {'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't'},
    {'u', 'v', 'w', 'x', 'y', 'z', '-', '\'', '!', '?'}
};

static const char* lut_ClassNames[6] = {
    "FIGHTER",
    "THIEF",
    "Bl.BELT",
    "RedMAGE",
    "Wh.MAGE",
    "Bl.MAGE"
};

static const ClassType lut_ClassTypes[6] = {
    ClassType::WARRIOR,
    ClassType::THIEF,
    ClassType::BLACK_BELT,
    ClassType::RED_MAGE,
    ClassType::WHITE_MAGE,
    ClassType::BLACK_MAGE
};

IntroEngine::IntroEngine() {
    state_ = IntroState::INTRO_STORY;
    title_cursor_ = 1;
    reset_party_creation();
}

void IntroEngine::reset_to_title() {
    state_ = IntroState::TITLE_SCREEN;
    title_cursor_ = 1; // Default to NEW GAME
    reset_party_creation();
}

void IntroEngine::reset_party_creation() {
    active_slot_ = 0;
    // Default starting class presets: Fighter, Thief, Wh.Mage, Bl.Mage
    slots_[0] = {0, ""};
    slots_[1] = {1, ""};
    slots_[2] = {4, ""};
    slots_[3] = {5, ""};
    kb_cursor_x_ = 0;
    kb_cursor_y_ = 0;
}

const PartyCreationSlot& IntroEngine::get_slot(size_t index) const {
    static PartyCreationSlot dummy;
    if (index < slots_.size()) return slots_[index];
    return dummy;
}

char IntroEngine::get_kb_char_at(uint8_t x, uint8_t y) const {
    if (x < 10 && y < 7) {
        return lut_NameInputGrid[y][x];
    }
    return ' ';
}

const char* IntroEngine::get_class_name(uint8_t class_id) {
    if (class_id < 6) return lut_ClassNames[class_id];
    return "UNKNOWN";
}

ClassType IntroEngine::get_class_type(uint8_t class_id) {
    if (class_id < 6) return lut_ClassTypes[class_id];
    return ClassType::WARRIOR;
}

IntroAction IntroEngine::handle_input(InputKey key, bool has_save_file) {
    IntroAction action = IntroAction::NONE;

    switch (state_) {
        case IntroState::INTRO_STORY: {
            if (key == InputKey::CONFIRM || key == InputKey::START || key == InputKey::CANCEL) {
                state_ = IntroState::TITLE_SCREEN;
                action = IntroAction::SOUND_SEL;
            }
            break;
        }

        case IntroState::TITLE_SCREEN: {
            if (key == InputKey::UP || key == InputKey::DOWN) {
                title_cursor_ ^= 1;
                action = IntroAction::SOUND_SEL;
            } else if (key == InputKey::LEFT) {
                respond_rate_ = (respond_rate_ + 7) & 7; // Wrap 0..7
                action = IntroAction::SOUND_MOVE;
            } else if (key == InputKey::RIGHT) {
                respond_rate_ = (respond_rate_ + 1) & 7;
                action = IntroAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM || key == InputKey::START) {
                if (title_cursor_ == 0) { // CONTINUE
                    if (has_save_file) {
                        state_ = IntroState::COMPLETE;
                        action = IntroAction::CONTINUE_GAME;
                    }
                } else { // NEW GAME
                    state_ = IntroState::PARTY_CREATION_CLASS;
                    reset_party_creation();
                    action = IntroAction::SOUND_SEL;
                }
            }
            break;
        }

        case IntroState::PARTY_CREATION_CLASS: {
            if (key == InputKey::UP || key == InputKey::LEFT) {
                slots_[active_slot_].class_id = (slots_[active_slot_].class_id + 5) % 6;
                action = IntroAction::SOUND_MOVE;
            } else if (key == InputKey::DOWN || key == InputKey::RIGHT) {
                slots_[active_slot_].class_id = (slots_[active_slot_].class_id + 1) % 6;
                action = IntroAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM || key == InputKey::START) {
                state_ = IntroState::PARTY_CREATION_NAME;
                slots_[active_slot_].name.clear();
                kb_cursor_x_ = 0;
                kb_cursor_y_ = 0;
                action = IntroAction::SOUND_SEL;
            } else if (key == InputKey::CANCEL) {
                if (active_slot_ > 0) {
                    active_slot_--;
                    action = IntroAction::SOUND_MOVE;
                } else {
                    state_ = IntroState::TITLE_SCREEN;
                    action = IntroAction::SOUND_MOVE;
                }
            }
            break;
        }

        case IntroState::PARTY_CREATION_NAME: {
            if (key == InputKey::UP) {
                kb_cursor_y_ = (kb_cursor_y_ + 6) % 7;
                action = IntroAction::SOUND_MOVE;
            } else if (key == InputKey::DOWN) {
                kb_cursor_y_ = (kb_cursor_y_ + 1) % 7;
                action = IntroAction::SOUND_MOVE;
            } else if (key == InputKey::LEFT) {
                kb_cursor_x_ = (kb_cursor_x_ + 9) % 10;
                action = IntroAction::SOUND_MOVE;
            } else if (key == InputKey::RIGHT) {
                kb_cursor_x_ = (kb_cursor_x_ + 1) % 10;
                action = IntroAction::SOUND_MOVE;
            } else if (key == InputKey::CONFIRM) {
                char ch = get_kb_char_at(kb_cursor_x_, kb_cursor_y_);
                if (slots_[active_slot_].name.length() < 4) {
                    slots_[active_slot_].name.push_back(ch);
                    action = IntroAction::SOUND_SEL;
                }
                
                // If 4 letters have been entered, advance
                if (slots_[active_slot_].name.length() >= 4) {
                    if (active_slot_ < 3) {
                        active_slot_++;
                        state_ = IntroState::PARTY_CREATION_CLASS;
                    } else {
                        state_ = IntroState::COMPLETE;
                        action = IntroAction::START_NEW_GAME;
                    }
                }
            } else if (key == InputKey::START) {
                // Pressing START pads name with spaces if non-empty, or uses default
                if (slots_[active_slot_].name.empty()) {
                    slots_[active_slot_].name = get_class_name(slots_[active_slot_].class_id);
                    if (slots_[active_slot_].name.length() > 4) {
                        slots_[active_slot_].name = slots_[active_slot_].name.substr(0, 4);
                    }
                }
                while (slots_[active_slot_].name.length() < 4) {
                    slots_[active_slot_].name.push_back(' ');
                }
                if (active_slot_ < 3) {
                    active_slot_++;
                    state_ = IntroState::PARTY_CREATION_CLASS;
                    action = IntroAction::SOUND_SEL;
                } else {
                    state_ = IntroState::COMPLETE;
                    action = IntroAction::START_NEW_GAME;
                }
            } else if (key == InputKey::CANCEL) {
                if (!slots_[active_slot_].name.empty()) {
                    slots_[active_slot_].name.pop_back();
                    action = IntroAction::SOUND_MOVE;
                } else {
                    state_ = IntroState::PARTY_CREATION_CLASS;
                    action = IntroAction::SOUND_MOVE;
                }
            }
            break;
        }

        case IntroState::COMPLETE:
            break;
    }

    return action;
}

IntroAction IntroEngine::type_char(char ch) {
    if (state_ != IntroState::PARTY_CREATION_NAME) return IntroAction::NONE;
    if (slots_[active_slot_].name.length() >= 4) return IntroAction::NONE;

    // Search for character in the authentic on-screen 7x10 grid
    for (uint8_t r = 0; r < 7; ++r) {
        for (uint8_t c = 0; c < 10; ++c) {
            if (lut_NameInputGrid[r][c] == ch) {
                kb_cursor_x_ = c;
                kb_cursor_y_ = r;
                slots_[active_slot_].name.push_back(ch);
                
                if (slots_[active_slot_].name.length() >= 4) {
                    if (active_slot_ < 3) {
                        active_slot_++;
                        state_ = IntroState::PARTY_CREATION_CLASS;
                    } else {
                        state_ = IntroState::COMPLETE;
                        return IntroAction::START_NEW_GAME;
                    }
                }
                return IntroAction::SOUND_SEL;
            }
        }
    }
    return IntroAction::NONE;
}

GameSaveData IntroEngine::finalize_party() const {
    std::array<ClassType, 4> classes;
    std::array<std::string, 4> names;

    for (int i = 0; i < 4; ++i) {
        classes[i] = get_class_type(slots_[i].class_id);
        std::string n = slots_[i].name;
        while (!n.empty() && n.back() == ' ') n.pop_back();
        if (n.empty()) n = get_class_name(slots_[i].class_id);
        if (n.length() > 4) n = n.substr(0, 4);
        names[i] = n;
    }

    return create_new_game_party(classes, names);
}

GameSaveData IntroEngine::create_new_game_party(
    const std::array<ClassType, 4>& classes,
    const std::array<std::string, 4>& names
) {
    GameSaveData save;
    save.gold = 500; // Starting GP in NES FF1
    save.vehicle = static_cast<uint8_t>(VehicleType::WALK);
    save.cur_map = 0; // Overworld
    save.player_x = 153; // Authentic starting position between Conelia Castle and Conelia Town
    save.player_y = 160;
    save.valid = true;

    for (int i = 0; i < 4; ++i) {
        PartyCharacter& hero = save.party[i];
        hero.name = names[i].empty() ? ("HERO" + std::to_string(i + 1)) : names[i];
        hero.char_class = classes[i];
        hero.level = 1;
        hero.exp = 0;
        hero.weapons = {0xFF, 0xFF, 0xFF, 0xFF};
        hero.armors  = {0xFF, 0xFF, 0xFF, 0xFF};
        for (auto& lvl : hero.spells) lvl = {0xFF, 0xFF, 0xFF};

        // Base NES starting stats per class from Disassembly
        switch (classes[i]) {
            case ClassType::WARRIOR:
                hero.stats = {35, 35, {0}, {0}, 10, 8, 1, 10, 5, 10, 48, 0, 10, 3, 3};
                break;
            case ClassType::THIEF:
                hero.stats = {30, 30, {0}, {0}, 5, 10, 5, 5, 15, 5, 50, 0, 5, 3, 3};
                break;
            case ClassType::BLACK_BELT:
                hero.stats = {33, 33, {0}, {0}, 5, 5, 5, 20, 5, 5, 45, 2, 2, 3, 3};
                break;
            case ClassType::RED_MAGE:
                hero.stats = {30, 30, {0}, {0}, 8, 7, 7, 6, 6, 7, 47, 0, 8, 3, 3};
                break;
            case ClassType::WHITE_MAGE:
                hero.stats = {28, 28, {0}, {0}, 5, 5, 15, 5, 5, 5, 45, 0, 5, 3, 3};
                hero.stats.max_mp[0] = 2;
                hero.stats.mp[0] = 2;
                break;
            case ClassType::BLACK_MAGE:
                hero.stats = {25, 25, {0}, {0}, 1, 7, 20, 1, 10, 5, 47, 0, 1, 3, 3};
                hero.stats.max_mp[0] = 2;
                hero.stats.mp[0] = 2;
                break;
            default:
                hero.stats = {30, 30, {0}, {0}, 5, 5, 5, 5, 5, 5, 45, 0, 5, 3, 3};
                break;
        }
    }

    return save;
}

} // namespace ff1
