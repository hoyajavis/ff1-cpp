#include "engine/system.hpp"
#include "engine/renderer.hpp"
#include "engine/rng.hpp"
#include "engine/audio_engine.hpp"
#include "engine/mod_loader.hpp"
#include "engine/chr_decoder.hpp"
#include "data/data_loader.hpp"
#include "data/map_loader.hpp"
#include "data/text_decoder.hpp"
#include "state/save_system.hpp"
#include "core/battle_engine.hpp"
#include "core/map_engine.hpp"
#include "core/menu_engine.hpp"
#include "core/intro_engine.hpp"
#include "core/cutscene_engine.hpp"
#include "core/minigame_engine.hpp"
#include "ui/window_box.hpp"
#include "ui/font.hpp"

#include <iostream>
#include <array>
#include <string>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::cout << "Launching Final Fantasy I (NES C++ Port Phase 8)..." << std::endl;

    // 1. Data & Mod Loaders
    ff1::DataLoader loader("");
    if (!loader.load_all()) {
        std::cerr << "Notice: Binary files loaded with standard fallback defaults." << std::endl;
    }

    ff1::MapLoader map_loader("");
    map_loader.load_all_maps(&loader);

    ff1::ModLoader mod_loader("./mods");
    mod_loader.scan_mod_directory();

    // 2. Engine Services
    ff1::RNG rng;
    ff1::AudioEngine audio;
    audio.init();

    ff1::CutsceneEngine cutscene;
    ff1::MiniGameEngine minigame;
    minigame.close_game();

    ff1::System sys(3); // 3x scale window (768x720)
    if (!sys.init("Final Fantasy I - NES C++ Port (Phase 8)")) {
        return 1;
    }

    ff1::Renderer renderer(256, 240);
    ff1::MapEngine map_engine(loader, map_loader, rng);
    ff1::BattleEngine battle_engine(loader, rng, true);
    ff1::MenuEngine menu_engine(loader);

    // 3. Game State & Party Initialization
    ff1::IntroEngine intro_engine;
    ff1::SaveSystem save_system;

    std::array<ff1::ClassType, 4> default_classes = {
        ff1::ClassType::WARRIOR,
        ff1::ClassType::THIEF,
        ff1::ClassType::WHITE_MAGE,
        ff1::ClassType::BLACK_MAGE
    };
    std::array<std::string, 4> default_names = {"WAR1", "THI2", "WHT3", "BLK4"};
    ff1::GameSaveData game_save = ff1::IntroEngine::create_new_game_party(default_classes, default_names);
    game_save.key_items_and_flags[static_cast<size_t>(ff1::KeyItem::MYSTIC_KEY)] = 1;

    bool has_save = save_system.load_game("ff1_save.sav", game_save);

    map_engine.load_map(0, ff1::MapType::OVERWORLD);
    audio.play_music(ff1::MusicTrack::PRELUDE);

    std::string dialogue_msg = "WASD: move, SPACE: act, B: battle, C: cutscene, P: puzzle, M: menu, T: title";
    bool in_battle = false;
    bool in_menu = false;

    std::cout << "Phase 15 Title Screen, Party Creation & Virtual Keyboard active." << std::endl;

    bool running = true;
    while (running) {
        running = sys.poll_events();

        if (sys.is_key_pressed(SDL_SCANCODE_ESCAPE)) {
            running = false;
        }

        // Title Screen & Party Creation Input Handling
        if (intro_engine.get_state() != ff1::IntroState::COMPLETE) {
            static bool intro_key_prev = false;
            ff1::InputKey ikey = ff1::InputKey::NONE;

            if (sys.is_key_pressed(SDL_SCANCODE_UP) || sys.is_key_pressed(SDL_SCANCODE_W)) ikey = ff1::InputKey::UP;
            else if (sys.is_key_pressed(SDL_SCANCODE_DOWN) || sys.is_key_pressed(SDL_SCANCODE_S)) ikey = ff1::InputKey::DOWN;
            else if (sys.is_key_pressed(SDL_SCANCODE_LEFT) || sys.is_key_pressed(SDL_SCANCODE_A)) ikey = ff1::InputKey::LEFT;
            else if (sys.is_key_pressed(SDL_SCANCODE_RIGHT) || sys.is_key_pressed(SDL_SCANCODE_D)) ikey = ff1::InputKey::RIGHT;
            else if (sys.is_key_pressed(SDL_SCANCODE_SPACE) || sys.is_key_pressed(SDL_SCANCODE_Z) || sys.is_key_pressed(SDL_SCANCODE_RETURN)) ikey = ff1::InputKey::CONFIRM;
            else if (sys.is_key_pressed(SDL_SCANCODE_TAB) || sys.is_key_pressed(SDL_SCANCODE_RSHIFT)) ikey = ff1::InputKey::START;
            else if (sys.is_key_pressed(SDL_SCANCODE_BACKSPACE) || sys.is_key_pressed(SDL_SCANCODE_X) || sys.is_key_pressed(SDL_SCANCODE_DELETE)) ikey = ff1::InputKey::CANCEL;

            if (ikey != ff1::InputKey::NONE) {
                if (!intro_key_prev) {
                    ff1::IntroAction act = intro_engine.handle_input(ikey, has_save);
                    if (act == ff1::IntroAction::START_NEW_GAME) {
                        game_save = intro_engine.finalize_party();
                        map_engine.load_map(0, ff1::MapType::OVERWORLD);
                        audio.play_music(ff1::MusicTrack::OVERWORLD);
                        audio.play_sfx(ff1::SoundEffect::SELECT);
                    } else if (act == ff1::IntroAction::CONTINUE_GAME) {
                        map_engine.load_map(game_save.cur_map, game_save.cur_map == 0 ? ff1::MapType::OVERWORLD : ff1::MapType::STANDARD_MAP);
                        audio.play_music(ff1::MusicTrack::OVERWORLD);
                        audio.play_sfx(ff1::SoundEffect::SELECT);
                    } else if (act == ff1::IntroAction::SOUND_SEL) {
                        audio.play_sfx(ff1::SoundEffect::SELECT);
                    } else if (act == ff1::IntroAction::SOUND_MOVE) {
                        audio.play_sfx(ff1::SoundEffect::CURSOR_MOVE);
                    }
                    intro_key_prev = true;
                }
            } else {
                intro_key_prev = false;
            }
        } else {
            // Field / In-Game Hotkeys

            // Return to Title Screen (T)
            static bool t_key_down = false;
            if (sys.is_key_pressed(SDL_SCANCODE_T)) {
                if (!t_key_down) {
                    intro_engine.reset_to_title();
                    audio.play_music(ff1::MusicTrack::PRELUDE);
                    t_key_down = true;
                }
            } else {
                t_key_down = false;
            }

            // Trigger 15-Puzzle Mini-Game (P)
            static bool p_key_down = false;
            if (sys.is_key_pressed(SDL_SCANCODE_P)) {
                if (!p_key_down) {
                    if (minigame.is_active()) {
                        minigame.close_game();
                    } else {
                        minigame.start_game();
                    }
                    p_key_down = true;
                }
            } else {
                p_key_down = false;
            }

            // Trigger Opening Cutscene (C)
            static bool c_key_down = false;
            if (sys.is_key_pressed(SDL_SCANCODE_C)) {
                if (!c_key_down && !cutscene.is_playing() && !minigame.is_active()) {
                    cutscene.start_cutscene(ff1::CutsceneType::OPENING_BRIDGE);
                    c_key_down = true;
                }
            } else {
                c_key_down = false;
            }

            if (minigame.is_active()) {
                // Puzzle controls
                static bool arrow_down = false;
                if (sys.is_key_pressed(SDL_SCANCODE_SPACE) || sys.is_key_pressed(SDL_SCANCODE_RETURN)) {
                    if (!arrow_down) {
                        minigame.move_tile(14); // Perform slide move
                        arrow_down = true;
                    }
                } else {
                    arrow_down = false;
                }

                if (minigame.check_win_condition()) {
                    uint32_t reward = minigame.calculate_gp_reward();
                    game_save.gold += reward;
                    dialogue_msg = "15-Puzzle Solved! Won " + std::to_string(reward) + " GP!";
                    minigame.close_game();
                }
            } else if (cutscene.is_playing()) {
                cutscene.update();
            } else if (in_battle) {
                static bool action_key_down = false;
                if (sys.is_key_pressed(SDL_SCANCODE_SPACE) || sys.is_key_pressed(SDL_SCANCODE_RETURN)) {
                    if (!action_key_down) {
                        std::array<ff1::BattleAction, 4> actions;
                        for (size_t i = 0; i < 4; ++i) {
                            actions[i].type = ff1::ActionType::ATTACK;
                            actions[i].actor_index = i;
                            actions[i].target_index = 0;
                        }
                        battle_engine.process_turn(actions, game_save);
                        action_key_down = true;
                    }
                } else {
                    action_key_down = false;
                }

                if (battle_engine.is_battle_over()) {
                    static int exit_delay = 0;
                    exit_delay++;
                    if (exit_delay > 60) {
                        in_battle = false;
                        exit_delay = 0;
                        audio.play_music(ff1::MusicTrack::OVERWORLD);
                        dialogue_msg = battle_engine.is_victory() ? "Battle Won!" : "Party Defeated...";
                    }
                }
            } else if (!in_menu) {
                static int move_cooldown = 0;
                if (move_cooldown > 0) move_cooldown--;

                if (move_cooldown == 0) {
                    std::string msg;
                    bool moved = false;
                    if (sys.is_key_pressed(SDL_SCANCODE_UP) || sys.is_key_pressed(SDL_SCANCODE_W)) {
                        moved = map_engine.move_player(ff1::Direction::UP, game_save, msg);
                        move_cooldown = 8;
                    } else if (sys.is_key_pressed(SDL_SCANCODE_DOWN) || sys.is_key_pressed(SDL_SCANCODE_S)) {
                        moved = map_engine.move_player(ff1::Direction::DOWN, game_save, msg);
                        move_cooldown = 8;
                    } else if (sys.is_key_pressed(SDL_SCANCODE_LEFT) || sys.is_key_pressed(SDL_SCANCODE_A)) {
                        moved = map_engine.move_player(ff1::Direction::LEFT, game_save, msg);
                        move_cooldown = 8;
                    } else if (sys.is_key_pressed(SDL_SCANCODE_RIGHT) || sys.is_key_pressed(SDL_SCANCODE_D)) {
                        moved = map_engine.move_player(ff1::Direction::RIGHT, game_save, msg);
                        move_cooldown = 8;
                    }

                    if (moved && !msg.empty()) {
                        dialogue_msg = msg;
                    }
                }

                static bool talk_key_down = false;
                if (sys.is_key_pressed(SDL_SCANCODE_SPACE) || sys.is_key_pressed(SDL_SCANCODE_RETURN)) {
                    if (!talk_key_down) {
                        std::string interact_msg;
                        if (map_engine.check_interaction(game_save, interact_msg)) {
                            dialogue_msg = interact_msg;
                            audio.play_sfx(ff1::SoundEffect::SELECT);
                        }
                        talk_key_down = true;
                    }
                } else {
                    talk_key_down = false;
                }

                static bool b_key_down = false;
                if (sys.is_key_pressed(SDL_SCANCODE_B)) {
                    if (!b_key_down && !in_battle) {
                        in_battle = true;
                        battle_engine.start_battle(game_save, 0);
                        audio.play_music(ff1::MusicTrack::BATTLE);
                        b_key_down = true;
                    }
                } else {
                    b_key_down = false;
                }

                static bool m_key_down = false;
                if (sys.is_key_pressed(SDL_SCANCODE_M) || sys.is_key_pressed(SDL_SCANCODE_TAB)) {
                    if (!m_key_down) {
                        in_menu = !in_menu;
                        audio.play_sfx(ff1::SoundEffect::CURSOR_MOVE);
                        m_key_down = true;
                    }
                } else {
                    m_key_down = false;
                }
            }
        }

        // Render Frame
        renderer.clear(0xFF102040);

        if (intro_engine.get_state() == ff1::IntroState::INTRO_STORY) {
            renderer.draw_intro_story(intro_engine);
        } else if (intro_engine.get_state() == ff1::IntroState::TITLE_SCREEN) {
            renderer.draw_title_screen(intro_engine, has_save);
        } else if (intro_engine.get_state() == ff1::IntroState::PARTY_CREATION_NAME) {
            renderer.draw_name_input_screen(intro_engine);
        } else if (intro_engine.get_state() != ff1::IntroState::COMPLETE) {
            renderer.draw_party_creation(intro_engine, loader);
        } else if (minigame.is_active()) {
            renderer.draw_puzzle(minigame, loader);
        } else if (cutscene.is_playing()) {
            renderer.draw_cutscene(cutscene, loader);
        } else if (in_battle) {
            renderer.draw_battle(battle_engine, game_save, loader);
        } else {
            renderer.draw_map(map_engine, loader, game_save.player_x, game_save.player_y);

            for (const auto& npc : map_engine.get_npcs()) {
                if (npc.active) {
                    renderer.draw_npc(npc.x, npc.y, game_save.player_x, game_save.player_y, loader);
                }
            }

            renderer.draw_player(game_save.party[0], loader, map_engine.get_player_facing(), static_cast<ff1::VehicleType>(game_save.vehicle));

            // Header HUD
            ff1::WindowBox::draw_box(renderer.get_buffer(), 256, 0, 0, 32, 3);
            ff1::Font::draw_string(renderer.get_buffer(), 256, 1, 1, map_engine.get_map_name() + " (" + std::to_string(game_save.player_x) + "," + std::to_string(game_save.player_y) + ") GP:" + std::to_string(game_save.gold));

            // Dialogue Box at bottom
            ff1::WindowBox::draw_box(renderer.get_buffer(), 256, 0, 22, 32, 7);
            ff1::Font::draw_string(renderer.get_buffer(), 256, 1, 23, dialogue_msg);

            // Overlay Main Menu if active
            if (in_menu) {
                ff1::WindowBox::draw_box(renderer.get_buffer(), 256, 1, 1, 30, 20);
                ff1::Font::draw_string(renderer.get_buffer(), 256, 2, 2, "MAIN MENU - PARTY STATUS");
                ff1::Font::draw_string(renderer.get_buffer(), 256, 2, 4, "GOLD: " + std::to_string(game_save.gold) + " GP");

                for (size_t i = 0; i < 4; ++i) {
                    const auto& hero = game_save.party[i];
                    std::string hline = hero.name + " L" + std::to_string(hero.level) + " HP:" + std::to_string(hero.stats.hp) + "/" + std::to_string(hero.stats.max_hp) + " STR:" + std::to_string(hero.stats.strength) + " AGI:" + std::to_string(hero.stats.agility);
                    ff1::Font::draw_string(renderer.get_buffer(), 256, 2, 6 + (i * 3), hline);
                }
                ff1::Font::draw_string(renderer.get_buffer(), 256, 2, 19, "Press [M] or [TAB] to Close Menu");
            }
        }

        sys.update_texture(renderer.get_buffer());
        sys.render_present();

        SDL_Delay(16); // ~60 FPS
    }

    std::cout << "Exiting Final Fantasy I C++ Port." << std::endl;
    return 0;
}
