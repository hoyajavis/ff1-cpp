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
                if (battle_engine.get_state() == ff1::BattleState::ROUND_EXECUTION) {
                    static int combat_tick = 0;
                    combat_tick++;
                    if (combat_tick > 20) {
                        battle_engine.step_combat_turn(game_save, audio);
                        combat_tick = 0;
                    }
                } else {
                    static bool battle_key_prev = false;
                    ff1::InputKey bkey = ff1::InputKey::NONE;

                    if (sys.is_key_pressed(SDL_SCANCODE_UP) || sys.is_key_pressed(SDL_SCANCODE_W)) bkey = ff1::InputKey::UP;
                    else if (sys.is_key_pressed(SDL_SCANCODE_DOWN) || sys.is_key_pressed(SDL_SCANCODE_S)) bkey = ff1::InputKey::DOWN;
                    else if (sys.is_key_pressed(SDL_SCANCODE_LEFT) || sys.is_key_pressed(SDL_SCANCODE_A)) bkey = ff1::InputKey::LEFT;
                    else if (sys.is_key_pressed(SDL_SCANCODE_RIGHT) || sys.is_key_pressed(SDL_SCANCODE_D)) bkey = ff1::InputKey::RIGHT;
                    else if (sys.is_key_pressed(SDL_SCANCODE_SPACE) || sys.is_key_pressed(SDL_SCANCODE_Z) || sys.is_key_pressed(SDL_SCANCODE_RETURN)) bkey = ff1::InputKey::CONFIRM;
                    else if (sys.is_key_pressed(SDL_SCANCODE_BACKSPACE) || sys.is_key_pressed(SDL_SCANCODE_X) || sys.is_key_pressed(SDL_SCANCODE_ESCAPE)) bkey = ff1::InputKey::CANCEL;

                    if (bkey != ff1::InputKey::NONE) {
                        if (!battle_key_prev) {
                            battle_engine.handle_battle_input(bkey, game_save, audio);
                            battle_key_prev = true;
                        }
                    } else {
                        battle_key_prev = false;
                    }
                }

                if (battle_engine.get_state() == ff1::BattleState::COMPLETE || battle_engine.get_state() == ff1::BattleState::GAME_OVER) {
                    in_battle = false;
                    if (battle_engine.is_victory() && game_save.cur_map == 60) {
                        game_save.key_items_and_flags[ff1::QuestFlag::CHAOS_DEFEATED] = 1;
                        game_save.key_items_and_flags[ff1::QuestFlag::GAME_COMPLETED] = 1;
                        cutscene.start_cutscene(ff1::CutsceneType::ENDING_CREDITS);
                        audio.play_music(ff1::MusicTrack::FANFARE);
                        dialogue_msg = "The 2000 year time loop is broken! Final Fantasy I Complete!";
                    } else {
                        audio.play_music(ff1::MusicTrack::OVERWORLD);
                        dialogue_msg = battle_engine.is_victory() ? "Battle Won!" : (battle_engine.is_escaped() ? "Escaped from Battle!" : "Party Defeated...");
                    }
                }
            } else if (menu_engine.get_state() != ff1::MenuState::CLOSED) {
                static bool menu_key_prev = false;
                ff1::InputKey mkey = ff1::InputKey::NONE;

                if (sys.is_key_pressed(SDL_SCANCODE_UP) || sys.is_key_pressed(SDL_SCANCODE_W)) mkey = ff1::InputKey::UP;
                else if (sys.is_key_pressed(SDL_SCANCODE_DOWN) || sys.is_key_pressed(SDL_SCANCODE_S)) mkey = ff1::InputKey::DOWN;
                else if (sys.is_key_pressed(SDL_SCANCODE_LEFT) || sys.is_key_pressed(SDL_SCANCODE_A)) mkey = ff1::InputKey::LEFT;
                else if (sys.is_key_pressed(SDL_SCANCODE_RIGHT) || sys.is_key_pressed(SDL_SCANCODE_D)) mkey = ff1::InputKey::RIGHT;
                else if (sys.is_key_pressed(SDL_SCANCODE_SPACE) || sys.is_key_pressed(SDL_SCANCODE_Z) || sys.is_key_pressed(SDL_SCANCODE_RETURN)) mkey = ff1::InputKey::CONFIRM;
                else if (sys.is_key_pressed(SDL_SCANCODE_BACKSPACE) || sys.is_key_pressed(SDL_SCANCODE_X) || sys.is_key_pressed(SDL_SCANCODE_ESCAPE)) mkey = ff1::InputKey::CANCEL;
                else if (sys.is_key_pressed(SDL_SCANCODE_TAB) || sys.is_key_pressed(SDL_SCANCODE_RSHIFT)) mkey = ff1::InputKey::SELECT;
                else if (sys.is_key_pressed(SDL_SCANCODE_M)) mkey = ff1::InputKey::START;

                if (mkey != ff1::InputKey::NONE) {
                    if (!menu_key_prev) {
                        ff1::MenuAction mact = menu_engine.handle_input(mkey, game_save, map_engine, audio, dialogue_msg);
                        if (mact == ff1::MenuAction::SOUND_MOVE) audio.play_sfx(ff1::SoundEffect::CURSOR_MOVE);
                        else if (mact == ff1::MenuAction::SOUND_SEL) audio.play_sfx(ff1::SoundEffect::SELECT);
                        else if (mact == ff1::MenuAction::SOUND_CANCEL) audio.play_sfx(ff1::SoundEffect::CURSOR_MOVE);
                        else if (mact == ff1::MenuAction::SOUND_CAST) audio.play_sfx(ff1::SoundEffect::SPELL_CAST);
                        else if (mact == ff1::MenuAction::SAVE_GAME_TRIGGERED) audio.play_sfx(ff1::SoundEffect::FANFARE);
                        menu_key_prev = true;
                    }
                } else {
                    menu_key_prev = false;
                }
            } else {
                static int move_cooldown = 0;
                if (move_cooldown > 0) move_cooldown--;

                if (move_cooldown == 0) {
                    std::string msg;
                    bool moved = false;
                    int spike_battle = -1;
                    int cooldown_rate = (game_save.vehicle == static_cast<uint8_t>(ff1::VehicleType::AIRSHIP)) ? 4 : 8;
                    if (sys.is_key_pressed(SDL_SCANCODE_UP) || sys.is_key_pressed(SDL_SCANCODE_W)) {
                        moved = map_engine.move_player(ff1::Direction::UP, game_save, msg, spike_battle);
                        move_cooldown = cooldown_rate;
                    } else if (sys.is_key_pressed(SDL_SCANCODE_DOWN) || sys.is_key_pressed(SDL_SCANCODE_S)) {
                        moved = map_engine.move_player(ff1::Direction::DOWN, game_save, msg, spike_battle);
                        move_cooldown = cooldown_rate;
                    } else if (sys.is_key_pressed(SDL_SCANCODE_LEFT) || sys.is_key_pressed(SDL_SCANCODE_A)) {
                        moved = map_engine.move_player(ff1::Direction::LEFT, game_save, msg, spike_battle);
                        move_cooldown = cooldown_rate;
                    } else if (sys.is_key_pressed(SDL_SCANCODE_RIGHT) || sys.is_key_pressed(SDL_SCANCODE_D)) {
                        moved = map_engine.move_player(ff1::Direction::RIGHT, game_save, msg, spike_battle);
                        move_cooldown = cooldown_rate;
                    }

                    if (moved && !msg.empty()) {
                        dialogue_msg = msg;
                    }

                    if (spike_battle >= 0 && !in_battle) {
                        in_battle = true;
                        battle_engine.start_battle(game_save, spike_battle);
                        audio.play_music(ff1::MusicTrack::BATTLE);
                    }
                }

                static bool land_key_down = false;
                if (sys.is_key_pressed(SDL_SCANCODE_L)) {
                    if (!land_key_down && game_save.vehicle == static_cast<uint8_t>(ff1::VehicleType::AIRSHIP)) {
                        std::string land_msg;
                        if (map_engine.land_airship(game_save, land_msg)) {
                            audio.play_sfx(ff1::SoundEffect::SELECT);
                        }
                        dialogue_msg = land_msg;
                        land_key_down = true;
                    }
                } else {
                    land_key_down = false;
                }

                static bool talk_key_down = false;
                if (sys.is_key_pressed(SDL_SCANCODE_SPACE) || sys.is_key_pressed(SDL_SCANCODE_RETURN)) {
                    if (!talk_key_down) {
                        std::string interact_msg;
                        int shop_id = -1;
                        int battle_id = -1;
                        if (map_engine.check_interaction(game_save, interact_msg, shop_id, battle_id)) {
                            dialogue_msg = interact_msg;
                            if (shop_id >= 0) {
                                menu_engine.open_shop(shop_id, game_save);
                                audio.play_sfx(ff1::SoundEffect::SELECT);
                            } else if (battle_id >= 0 && !in_battle) {
                                in_battle = true;
                                battle_engine.start_battle(game_save, battle_id);
                                audio.play_music(ff1::MusicTrack::BATTLE);
                            } else {
                                audio.play_sfx(ff1::SoundEffect::SELECT);
                            }
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

                // Field Menu Toggle (M / TAB)
                static bool m_key_down = false;
                if (sys.is_key_pressed(SDL_SCANCODE_M) || sys.is_key_pressed(SDL_SCANCODE_TAB)) {
                    if (!m_key_down) {
                        menu_engine.open_main_menu();
                        audio.play_sfx(ff1::SoundEffect::SELECT);
                        m_key_down = true;
                    }
                } else {
                    m_key_down = false;
                }

                // World Mini-Map Toggle (N)
                static bool n_key_down = false;
                if (sys.is_key_pressed(SDL_SCANCODE_N)) {
                    if (!n_key_down) {
                        menu_engine.open_world_map();
                        audio.play_sfx(ff1::SoundEffect::SELECT);
                        n_key_down = true;
                    }
                } else {
                    n_key_down = false;
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
        } else if (menu_engine.get_state() == ff1::MenuState::SHOP) {
            renderer.draw_shop(menu_engine, game_save, loader);
        } else if (menu_engine.get_state() == ff1::MenuState::MAIN_MENU || menu_engine.get_state() == ff1::MenuState::LINEUP_SELECT) {
            renderer.draw_main_menu(menu_engine, game_save, loader);
        } else if (menu_engine.get_state() == ff1::MenuState::ITEM_MENU || menu_engine.get_state() == ff1::MenuState::ITEM_TARGET_SELECT || menu_engine.get_state() == ff1::MenuState::CAMPING_SAVE_PROMPT) {
            renderer.draw_item_menu(menu_engine, game_save, loader);
        } else if (menu_engine.get_state() == ff1::MenuState::EQUIP_MENU) {
            renderer.draw_equip_menu(menu_engine, game_save, loader);
        } else if (menu_engine.get_state() == ff1::MenuState::MAGIC_MENU || menu_engine.get_state() == ff1::MenuState::MAGIC_TARGET_SELECT) {
            renderer.draw_magic_menu(menu_engine, game_save, loader);
        } else if (menu_engine.get_state() == ff1::MenuState::STATUS_MENU) {
            renderer.draw_status_menu(menu_engine, game_save, loader);
        } else if (menu_engine.get_state() == ff1::MenuState::WORLD_MAP_SCREEN) {
            renderer.draw_world_map_screen(loader, game_save.player_x, game_save.player_y);
        } else {
            renderer.draw_map(map_engine, loader, game_save.player_x, game_save.player_y);

            // Draw docked ship if visible and player is on foot
            if (map_engine.get_map_type() == ff1::MapType::OVERWORLD && game_save.ship_visible && game_save.vehicle != static_cast<uint8_t>(ff1::VehicleType::SHIP)) {
                renderer.draw_npc(game_save.ship_x, game_save.ship_y, game_save.player_x, game_save.player_y, loader, 0xFF6080D0);
            }

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
        }

        sys.update_texture(renderer.get_buffer());
        sys.render_present();

        SDL_Delay(16); // ~60 FPS
    }

    std::cout << "Exiting Final Fantasy I C++ Port." << std::endl;
    return 0;
}
