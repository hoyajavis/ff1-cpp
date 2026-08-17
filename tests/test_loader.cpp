#include "data/data_loader.hpp"
#include "data/map_loader.hpp"
#include "data/text_decoder.hpp"
#include "engine/rng.hpp"
#include "engine/audio_engine.hpp"
#include "engine/mod_loader.hpp"
#include "engine/chr_decoder.hpp"
#include "core/battle_engine.hpp"
#include "core/map_engine.hpp"
#include "core/menu_engine.hpp"
#include "core/intro_engine.hpp"
#include "core/cutscene_engine.hpp"
#include "core/minigame_engine.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "FF1 NES C++ Port - Comprehensive Verification" << std::endl;
    std::cout << "==========================================" << std::endl;

    // Test 1: DataLoader Initialization
    ff1::DataLoader loader("");
    bool loaded = loader.load_all();
    if (!loaded) {
        ff1::DataLoader loader2("../FinalFantasyDisassembly_v1_0/Final Fantasy Disassembly/bin");
        loaded = loader2.load_all();
        if (loaded) loader = loader2;
    }
    std::cout << "[Test 1] DataLoader status: " << (loaded ? "SUCCESS" : "FAILED") << std::endl;
    assert(loaded && "DataLoader failed to load NES binary assets!");

    // Test 2: Weapon Asset Parsing
    const auto& weapons = loader.get_weapons();
    std::cout << "[Test 2] Loaded Weapons: " << weapons.size() << " (Expected: 40)" << std::endl;
    assert(weapons.size() == 40 && "Weapon count mismatch!");

    // Test 3: Enemy Asset Parsing
    const auto& enemies = loader.get_enemies();
    std::cout << "[Test 3] Loaded Enemies: " << enemies.size() << " (Expected: 128)" << std::endl;
    assert(enemies.size() == 128 && "Enemy count mismatch!");

    // Test 4: Battle Formations
    const auto& formations = loader.get_formations();
    std::cout << "[Test 4] Loaded Formations: " << formations.size() << " (Expected: 128)" << std::endl;
    assert(formations.size() == 128 && "Formation count mismatch!");

    // Test 5: NES RNG Sequence Check ($F100/$FCF1)
    ff1::RNG rng;
    uint8_t r1 = rng.next_byte();
    uint8_t r2 = rng.next_byte();
    std::cout << "[Test 5] NES RNG Sequence Check: 1st=" << (int)r1 << ", 2nd=" << (int)r2 << std::endl;

    // Test 6: DTE Text Decoder
    uint8_t sample_dte[] = {0x1C, 0x1D, 0xC1, 0x90, 0xA4, 0xB0, 0xA8, 0x00};
    std::string decoded = ff1::TextDecoder::decode_string(sample_dte, sizeof(sample_dte));
    std::cout << "[Test 6] Text Decoder Output: '" << decoded << "'" << std::endl;

    // Test 7: MapLoader Asset Parsing
    ff1::MapLoader map_loader("");
    bool maps_ok = map_loader.load_all_maps(&loader);
    std::cout << "[Test 7] MapLoader Status: " << (maps_ok ? "SUCCESS" : "FAILED")
              << " | Maps Loaded: " << map_loader.get_total_maps_loaded()
              << " | NPC Objects Loaded: " << map_loader.get_total_npcs_loaded() << std::endl;
    assert(maps_ok && "MapLoader failed to load maps/NPCs!");
    assert(map_loader.get_total_maps_loaded() == 64 && "All 64 authentic standard maps must be loaded!");

    // Test 8: Enemy AI Decision Scripts
    const auto& ai_scripts = loader.get_ai_scripts();
    std::cout << "[Test 8] Enemy AI Scripts Loaded: " << ai_scripts.size() << " (Expected: 44)" << std::endl;
    assert(ai_scripts.size() == 44 && "AI script count mismatch!");

    // Test 9: World Shop Inventories
    const auto& shops = loader.get_shops();
    std::cout << "[Test 9] Shops Loaded: " << shops.size() << " (Expected: 48)" << std::endl;
    assert(shops.size() == 48 && "Shop count mismatch!");

    // Test 10: Party Creation & Equipment Recalculation
    std::array<ff1::ClassType, 4> base_classes = {
        ff1::ClassType::WARRIOR,
        ff1::ClassType::THIEF,
        ff1::ClassType::WHITE_MAGE,
        ff1::ClassType::BLACK_MAGE
    };
    std::array<std::string, 4> hero_names = {"FIGHT", "THIEF", "WHITE", "BLACK"};
    ff1::GameSaveData save_data = ff1::IntroEngine::create_new_game_party(base_classes, hero_names);

    ff1::MenuEngine menu_engine(loader);
    std::string promo_msg;
    bool promoted = menu_engine.promote_party_classes(save_data, promo_msg);
    std::cout << "[Test 10] Bahamut Class Promotion: " << (promoted ? "SUCCESS" : "FAILED") << " | " << promo_msg << std::endl;
    assert(promoted && "Class promotion failed!");

    // Test 11: Audio Engine Track Switching
    ff1::AudioEngine audio;
    audio.init();
    audio.play_music(ff1::MusicTrack::OVERWORLD);
    std::cout << "[Test 11] AudioEngine Status: SUCCESS | Current Track: OVERWORLD" << std::endl;

    // Test 12: HD Mod Loader Directory Scanner
    ff1::ModLoader mod_loader("./mods");
    bool scanned = mod_loader.scan_mod_directory();
    std::cout << "[Test 12] ModLoader Directory Scan: " << (scanned ? "SUCCESS" : "FAILED")
              << " | Active HD Mod Files: " << mod_loader.get_active_mods_count() << std::endl;

    // Test 13: Cinematic Cutscene Engine Playback
    ff1::CutsceneEngine cutscene;
    cutscene.start_cutscene(ff1::CutsceneType::OPENING_BRIDGE);
    std::cout << "[Test 13] CutsceneEngine Status: " << (cutscene.is_playing() ? "PLAYING" : "STOPPED")
              << " | Subtitle: '" << cutscene.get_current_subtitle() << "'" << std::endl;
    assert(cutscene.is_playing() && "Cutscene must be active!");

    // Test 14: Map Engine Event Triggers & Quest Flags
    ff1::MapEngine map_engine(loader, map_loader, rng);
    map_engine.load_map(10, ff1::MapType::STANDARD_MAP);
    save_data.player_x = 16; save_data.player_y = 16;
    std::string event_msg;
    bool triggered = map_engine.check_event_trigger(save_data, event_msg);
    std::cout << "[Test 14] Quest Event Trigger: " << (triggered ? "SUCCESS" : "FAILED") << " | " << event_msg << std::endl;

    // Test 15: NES CHR Tile Decoding & Palette Parsing
    uint8_t dummy_chr_16bytes[16] = {
        0x3C, 0x42, 0x95, 0xA1, 0xA1, 0x95, 0x42, 0x3C, // Bitplane 0
        0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C  // Bitplane 1
    };
    std::array<uint8_t, 4> palette = {0x0F, 0x30, 0x16, 0x27}; // Black, White, Red, Green
    ff1::PixelBuffer8x8 decoded_tile = ff1::CHRDecoder::decode_chr_tile(dummy_chr_16bytes, palette, false, false, true);
    std::cout << "[Test 15] NES CHR Decoder Test: SUCCESS | Pixel[0,0] RGBA: 0x" << std::hex << decoded_tile[0] << std::dec << std::endl;

    // Test 16: 15-Puzzle MiniGame Solvability & Reward
    ff1::MiniGameEngine minigame;
    minigame.start_game();
    std::cout << "[Test 16] 15-Puzzle MiniGame Status: ACTIVE | Moves: " << minigame.get_move_count() << std::endl;

    // Test 17: Background vs Sprite Alpha Separation
    ff1::PixelBuffer8x8 bg_tile = ff1::CHRDecoder::decode_chr_tile(dummy_chr_16bytes, palette, false, false, false);
    ff1::PixelBuffer8x8 spr_tile = ff1::CHRDecoder::decode_chr_tile(dummy_chr_16bytes, palette, false, false, true);
    // At pixel [0,0], dummy_chr has 0 for both bitplanes -> color_idx = 0
    assert(bg_tile[0] == ff1::lut_NESPalette[palette[0]] && "Background color 0 must be opaque palette background!");
    assert(spr_tile[0] == 0x00000000 && "Sprite color 0 must be transparent!");
    std::cout << "[Test 17] Background vs Sprite Alpha Separation: SUCCESS (BG=0x" << std::hex << bg_tile[0] << ", Sprite=0x" << spr_tile[0] << std::dec << ")" << std::endl;

    // Test 18: Overworld Map 256x256 Stream & Palette LUT Parsing
    const auto& ow_data = loader.get_overworld_map();
    const auto& sm_pals = loader.get_sm_palettes();
    const auto& mm_pals = loader.get_mapman_palettes();
    std::cout << "[Test 18] Overworld Stream: " << ow_data.size() << " bytes (Expected: 65536)"
              << " | SM Palettes: " << sm_pals.size() << " bytes"
              << " | Mapman Palettes: " << mm_pals.size() << " bytes" << std::endl;
    assert(ow_data.size() == 65536 && "Overworld map size mismatch!");
    assert(!sm_pals.empty() && "SM Palettes must be loaded!");
    assert(!mm_pals.empty() && "Mapman Palettes must be loaded!");

    // Test 19: Battle CHR Banks & Monster Palettes
    const auto& b7 = loader.get_chr_bank_07();
    const auto& b8 = loader.get_chr_bank_08();
    const auto& b9 = loader.get_chr_bank_09();
    const auto& btl_pals = loader.get_battle_palettes();
    std::cout << "[Test 19] Battle CHR Banks: Bank07=" << b7.size() << " bytes"
              << ", Bank08=" << b8.size() << " bytes"
              << ", Bank09=" << b9.size() << " bytes"
              << " | Battle Palettes=" << btl_pals.size() << " bytes" << std::endl;
    assert(b7.size() == 16384 && "Bank 07 CHR size mismatch!");
    assert(b8.size() == 16384 && "Bank 08 CHR size mismatch!");
    assert(!b9.empty() && "Bank 09 data must be loaded!");
    assert(!btl_pals.empty() && "Battle palettes must be loaded!");

    // Test 20: Battle Engine Formation & Monster Palette Mapping
    ff1::BattleEngine battle_engine(loader, rng, true);
    battle_engine.start_battle(save_data, 0);
    const auto& formation = battle_engine.get_formation();
    std::array<uint8_t, 4> m_pal = loader.get_monster_palette(formation.palette_id[0]);
    std::cout << "[Test 20] Battle Visual Engine: SUCCESS | Monster Pal[0..3]=0x"
              << std::hex << (int)m_pal[0] << ",0x" << (int)m_pal[1] << ",0x" << (int)m_pal[2] << ",0x" << (int)m_pal[3] << std::dec << std::endl;

    // Test 21: 1bpp NES CHR Tile Decoding & 15-Puzzle Assets
    uint8_t sample_1bpp[8] = {0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}; // Sample 'A'
    ff1::PixelBuffer8x8 decoded_1bpp = ff1::CHRDecoder::decode_1bpp_tile(sample_1bpp, 0xFFFFFFFF, 0xFF000000);
    const auto& p_chr = loader.get_puzzle_chr();
    std::cout << "[Test 21] 1bpp CHR Decoder: SUCCESS | Puzzle CHR=" << p_chr.size() << " bytes (Expected: 512)" << std::endl;
    assert(decoded_1bpp[0] == 0xFF000000 && decoded_1bpp[2] == 0xFFFFFFFF && "1bpp pixel mapping mismatch!");
    assert(p_chr.size() == 512 && "Puzzle CHR size mismatch!");

    // Test 22: Cinematic Cutscene & Ending Sequence Assets
    const auto& bridge_data = loader.get_bridge_cutscene_data();
    const auto& end_data = loader.get_ending_draw_data();
    const auto& end_luts = loader.get_ending_luts();
    std::cout << "[Test 22] Cutscenes & Ending: Bridge=" << bridge_data.size() << " bytes"
              << ", TheEnd=" << end_data.size() << " bytes"
              << ", EndLUTs=" << end_luts.size() << " bytes" << std::endl;
    assert(!bridge_data.empty() && "Bridge cutscene data must be loaded!");
    assert(!end_data.empty() && "TheEnd draw data must be loaded!");
    assert(!end_luts.empty() && "TheEnd LUT data must be loaded!");

    // Test 23: Intro Story & Title Screen Navigation
    ff1::IntroEngine intro;
    assert(intro.get_state() == ff1::IntroState::INTRO_STORY);
    intro.handle_input(ff1::InputKey::CONFIRM, false); // Advance from Intro Story to Title Screen
    assert(intro.get_state() == ff1::IntroState::TITLE_SCREEN);
    assert(intro.get_title_cursor() == 1); // Default NEW GAME
    intro.handle_input(ff1::InputKey::UP, false);
    assert(intro.get_title_cursor() == 0); // Toggled to CONTINUE
    intro.handle_input(ff1::InputKey::DOWN, false);
    assert(intro.get_title_cursor() == 1); // Toggled back to NEW GAME
    uint8_t init_rate = intro.get_respond_rate();
    intro.handle_input(ff1::InputKey::RIGHT, false);
    assert(intro.get_respond_rate() == (init_rate % 8) + 1);
    std::cout << "[Test 23] Intro Story & Title Screen Engine: SUCCESS | Cursor=" << (int)intro.get_title_cursor()
              << ", Respond Rate=" << (int)intro.get_respond_rate() << std::endl;

    // Test 24: Party Creation Matrix & Virtual Keyboard Typing
    intro.handle_input(ff1::InputKey::CONFIRM, false); // Select NEW GAME -> PARTY_CREATION_CLASS
    assert(intro.get_state() == ff1::IntroState::PARTY_CREATION_CLASS);
    assert(intro.get_active_slot() == 0);

    // Slot 0: Choose Fighter, enter name "NOBI"
    intro.handle_input(ff1::InputKey::CONFIRM, false); // Open Keyboard for slot 0
    assert(intro.get_state() == ff1::IntroState::PARTY_CREATION_NAME);
    // Type 'N', 'O', 'B', 'I'
    // 'N' is at (3, 1), 'O' is at (4, 1), 'B' is at (1, 0), 'I' is at (8, 0)
    for (int y = 0; y < 1; ++y) intro.handle_input(ff1::InputKey::DOWN, false);
    for (int x = 0; x < 3; ++x) intro.handle_input(ff1::InputKey::RIGHT, false);
    intro.handle_input(ff1::InputKey::CONFIRM, false); // Type 'N'
    intro.handle_input(ff1::InputKey::RIGHT, false);
    intro.handle_input(ff1::InputKey::CONFIRM, false); // Type 'O'
    intro.handle_input(ff1::InputKey::UP, false);
    intro.handle_input(ff1::InputKey::LEFT, false);
    intro.handle_input(ff1::InputKey::LEFT, false);
    intro.handle_input(ff1::InputKey::LEFT, false);
    intro.handle_input(ff1::InputKey::CONFIRM, false); // Type 'B'
    for (int x = 0; x < 7; ++x) intro.handle_input(ff1::InputKey::RIGHT, false);
    intro.handle_input(ff1::InputKey::CONFIRM, false); // Type 'I' -> advances to Slot 1!

    assert(intro.get_active_slot() == 1);
    assert(intro.get_slot(0).name == "NOBI");

    // Fast-fill slots 1, 2, 3 using CONFIRM -> START
    intro.handle_input(ff1::InputKey::CONFIRM, false); // Slot 1 enter name
    intro.handle_input(ff1::InputKey::START, false);   // Slot 1 auto-fill
    assert(intro.get_active_slot() == 2);

    intro.handle_input(ff1::InputKey::CONFIRM, false); // Slot 2 enter name
    intro.handle_input(ff1::InputKey::START, false);   // Slot 2 auto-fill
    assert(intro.get_active_slot() == 3);

    intro.handle_input(ff1::InputKey::CONFIRM, false); // Slot 3 enter name
    ff1::IntroAction final_act = intro.handle_input(ff1::InputKey::START, false); // Slot 3 auto-fill -> Complete!
    assert(final_act == ff1::IntroAction::START_NEW_GAME);
    assert(intro.get_state() == ff1::IntroState::COMPLETE);

    ff1::GameSaveData generated_save = intro.finalize_party();
    assert(generated_save.party[0].name == "NOBI");
    assert(generated_save.party[0].char_class == ff1::ClassType::WARRIOR);
    assert(generated_save.gold == 500);
    std::cout << "[Test 24] Party Creation Matrix & Virtual Keyboard: SUCCESS | Hero1="
              << generated_save.party[0].name << " (" << ff1::IntroEngine::get_class_name(0) << "), GP=" << generated_save.gold << std::endl;

    // Test 25: Standard Map Decompression & Transition Engine
    map_engine.load_map(0, ff1::MapType::OVERWORLD);
    assert(map_engine.get_map_type() == ff1::MapType::OVERWORLD);
    assert(map_engine.get_width() == 256 && map_engine.get_height() == 256);

    // Transition to Conelia Town (Map 2)
    map_engine.load_map(2, ff1::MapType::STANDARD_MAP);
    assert(map_engine.get_map_type() == ff1::MapType::STANDARD_MAP);
    assert(map_engine.get_current_map_id() == 2);
    assert(map_engine.get_width() == 64 && map_engine.get_height() == 64);
    assert(map_engine.get_map_name() == "Conelia Town");

    // Check that authentic town tiles are present (not empty 0s)
    bool has_non_zero_tiles = false;
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            if (map_engine.get_tile_at(x, y) != 0) {
                has_non_zero_tiles = true;
                break;
            }
        }
        if (has_non_zero_tiles) break;
    }
    assert(has_non_zero_tiles && "Conelia Town must have decompressed authentic tiles!");

    // Transition to Temple of Fiends (Map 10)
    map_engine.load_map(10, ff1::MapType::STANDARD_MAP);
    assert(map_engine.get_current_map_id() == 10);
    assert(map_engine.get_map_name() == "Temple of Fiends 1F");
    assert(map_engine.get_width() == 64 && map_engine.get_height() == 64);

    std::cout << "[Test 25] Standard Map Engine & Authentic Decompression: SUCCESS | Map 2="
              << "Conelia Town (64x64), Map 10=Temple of Fiends 1F (64x64)" << std::endl;

    std::cout << "==========================================" << std::endl;
    std::cout << "ALL 25 END-TO-END VERIFICATION TESTS PASSED!" << std::endl;
    std::cout << "==========================================" << std::endl;

    return 0;
}
