# Final Fantasy I (NES) C++ Port (`ff1_cpp`)

A modern **C++20** port of **Final Fantasy I (NES)** built from scratch using **Disch's 2015 Disassembly** as the authoritative source reference for original game data tables, math formulas, map formats, battle mechanics, and RAM/SRAM state structures.

---

## 🌟 Key Features & Technical Architecture

- **100% NES-Faithful Mechanics**: Authentic implementation of NES combat formulas, turn initiative ordering, physical hit/damage math, magic accuracy, elemental weaknesses/resistances, status ailments, enemy AI decision scripts, and 256-byte RNG sequences ($F100 / $FCF1).
- **Disch 2015 Disassembly Asset Pipeline**: Parses binary asset tables directly from `FinalFantasyDisassembly_v1_0/`:
  - 40 Weapons (`0C_8000_weapondata.bin`)
  - 20 Armors (`0C_8140_armordata.bin`)
  - 64 Magic Spells (`0C_81E0_magicdata.bin`)
  - 128 Enemy Stat Entries (`0C_8520_enemydata.bin` & `0B_94E0_enemynames.bin`)
  - 128 Battle Formations (`0B_8400_battleformations.bin`)
  - 44 Enemy AI Decision Scripts (`0C_9020_aidata.bin`)
  - 48 World Shop Inventories (`0E_8300_shopdata.bin`)
  - 104 NPC Map Objects (`0E_95D5_objectdata.bin`)
  - 64 Decompressed Standard Maps ($64 \times 64$ tiles from `bank_04.dat`, `bank_05.dat`, `bank_06.dat`)
  - Full 256x256 Overworld Map stream (`bank_01.bin` / `bank_01_data.bin`)
  - TSA 16x16 macroblock tile definitions (`lut_SMTilesetTSA` at `$9000` in `bank_00.dat`)
  - Complete Overworld and Dungeon Teleport & Entrance/Exit Matrices
  - DTE (Dual Tile Encoding) text compression tables (`lut_DTE1` / `lut_DTE2`)
- **Authentic Opening Story, Title Screen & Party Creation**:
  - Cold boot Opening Story prologue scroll with blinking confirm prompt.
  - Interactive Title Screen with cursor selection and authentic Message Respond Rate adjustment (1..8).
  - 2x2 Party Creation matrix with 6 starting classes (Fighter, Thief, Black Belt, Red Mage, White Mage, Black Mage).
  - Interactive on-screen Virtual Keyboard alphabet name entry (`A-Z`, Enter confirmation, Backspace letter deletion, Start auto-fill submission).
- **Complete Map & Visual Rendering Pipeline**:
  - `CHRDecoder` decoding 16-byte 2-bit planar NES CHR tile graphics directly from ROM banks (`bank_02.dat`, `bank_03.dat`, `bank_05.dat`, `bank_07.dat`, `bank_08.dat`, `bank_09.dat`).
  - Dynamic Standard Map tileset switching across all 8 tilesets (Town, Castle, Cave/Dungeon, Temple/Fiend Lair, Volcano, Ice Cave, Floating Castle, Sea Shrine) and menu CHR.
  - Smooth animated palette shimmering for authentic ocean waves, rivers, and coastline transitions.
  - Directional 2-frame walk cycle animations for all 12 player classes, 104 NPC map objects, and vehicles (Ship, Canoe, Airship) with transparent background keying.
- **Full Battle Engine & Spell/Item Matrix**:
  - Full 64 White & Black Magic Spells (Healing, Revival, Elemental Damage, Stat Buffs/Debuffs, Instant Death, Status Ailments, and Escape/Warp).
  - Monster AI Decision Script Execution (`process_enemy_ai_turn`).
  - Equipped Item & Weapon Combat Use (`ActionType::ITEM`).
  - Status Ailment Matrix (Death, Stone, Paralysis, Poison, Blindness, Silence, Sleep, Confusion).
- **World Shops, Equipment & Class Promotions**:
  - Weapon, Armor, White/Black Magic Shops, Clinics (Resurrection), and Inns (HP/MP restore & SRAM file saving).
  - Real-time Equipment Manager recalculating Absorb, Evade penalty, HitRate, Damage, and Critical hit rates.
  - King Bahamut Class Promotion System transforming base classes (Warrior ➔ Knight, Thief ➔ Ninja, Black Belt ➔ Master, Red Mage ➔ Red Wizard, White Mage ➔ White Wizard, Black Mage ➔ Black Wizard).
- **Secret 15-Puzzle Mini-Game & Cinematic Cutscenes**:
  - Interactive 4x4 sliding tile 15-Puzzle Mini-Game engine (`BANK_MINIGAME = $0D`) triggered aboard the Ship (key `P`), with 1bpp CHR rendering and time-scaled GP rewards (100 GP to 10,000 GP).
  - Cinematic Cutscene Engine controlling timing and subtitle scrolling for Opening Conelia Bridge, Airship Rising, and Ending Credits.
- **Audio Engine & HD Modding**:
  - Audio Engine managing BGM tracks (Overworld, Town, Castle, Battle, Fanfare, Dungeon, Airship, Game Over) and SFX signals with soft-synth fallback.
  - Native HD Modding System inspecting `./mods/` directory for optional PNG graphic overrides and WAV/OGG audio stream replacements.
- **Modern C++20 Architecture**:
  - *MMC1 Mapper Flattening*: Unified asset arrays eliminating 16KB PRG-ROM bank latency while retaining authentic game logic.
  - *Hardware Timing & Split Screen*: Decoupled PPU cycle polling into a clean 60 FPS event loop with layered UI rasterization.
  - *Strongly-Typed State Encapsulation*: Zero-page RAM structures encapsulated into type-safe C++ structs (`GameSaveData`, `PartyCharacter`).
  - *Widescreen (16:9) Mode*: Decoupled 256x240 view canvas allowing optional 16:9 widescreen expansion.
  - *Cross-Platform*: Single C++20 codebase targeting Windows, Linux, macOS, Steam Deck, and mobile devices.
  - *NES Bug-Fix Toggle*: Option to toggle notorious NES FF1 bugs (e.g. Intelligence stat not affecting spell damage, weapon element/critical rate bugs) or play with authentic NES bug fixes enabled (`enable_bug_fixes = true`).

---

## 📁 Project Architecture

```
ff1_cpp/
├── CMakeLists.txt                 # C++20 build configuration & SDL2 FetchContent setup
├── README.md                      # Comprehensive project documentation
├── ROADMAP.md                     # Phased completion roadmap & technical specifications
├── src/
│   ├── main.cpp                   # Main game entry point & 60 FPS event loop
│   ├── data/
│   │   ├── game_types.hpp         # C++ structs (Party, Character, Weapon, Armor, Spell, Enemy, Formation, EnemyAIData, ShopInventory)
│   │   ├── map_types.hpp          # TSA tiles, Standard Map structures, Teleport & NPC spawn types
│   │   ├── data_loader.hpp / .cpp # Disassembly binary asset parser & 64 standard map decompressor
│   │   ├── map_loader.hpp / .cpp  # Standard map, overworld, NPC object & teleport loader
│   │   └── text_decoder.hpp / .cpp# NES DTE text decoder
│   ├── engine/
│   │   ├── system.hpp / .cpp      # SDL2 windowing, texture updating & input dispatch
│   │   ├── renderer.hpp / .cpp    # 256x240/16:9 PPU tile map, sprite & UI rasterizer
│   │   ├── audio_engine.hpp / .cpp# Music tracks and SFX trigger engine
│   │   ├── mod_loader.hpp / .cpp  # HD PNG graphics & WAV/OGG audio override scanner
│   │   ├── chr_decoder.hpp / .cpp # NES 2-bit CHR tile graphics decoder & NES RGB palette
│   │   └── rng.hpp / .cpp         # NES 256-byte lookup RNG sequence ($F100/$FCF1)
│   ├── state/
│   │   ├── game_state.hpp         # Game RAM/SRAM state representation
│   │   └── save_system.hpp / .cpp # 8KB SRAM save slot file persistence
│   ├── core/
│   │   ├── battle_engine.hpp / .cpp# Turn-based combat system, 64 Spells, AI scripts, math formulas, rewards
│   │   ├── map_engine.hpp / .cpp  # Overworld/dungeon map movement, tile properties, NPCs, chests, teleports, event triggers
│   │   ├── menu_engine.hpp / .cpp # Status, Equipment, Magic, Item, Lineup, Shop transactions & Class Promotions
│   │   ├── intro_engine.hpp / .cpp# Opening story, Title Screen, 2x2 Party Creation & Virtual Keyboard engine
│   │   ├── cutscene_engine.hpp / .cpp# Opening Bridge, Airship Rising & Ending Credits sequence player
│   │   └── minigame_engine.hpp / .cpp# 15-Puzzle Ship mini-game board & rewards
│   └── ui/
│       ├── window_box.hpp / .cpp  # Classic NES border box window renderer
│       └── font.hpp / .cpp        # 8x8 font character drawing
└── tests/
    └── test_loader.cpp            # Comprehensive 25-test automated verification suite
```

---

## 🎮 Controls

| Key | Action | Context |
| :--- | :--- | :--- |
| **Arrow Keys / WASD** | Walk Player / Navigate Menus | Field / Combat / Menus |
| **SPACE / RETURN / Z** | Confirm / Talk to NPC / Open Chest / Select Letter / Attack | General |
| **BACKSPACE / DELETE / X** | Cancel / Delete Name Letter | Party Creation / Menus |
| **TAB / RSHIFT** | Auto-Fill Name & Advance / Submit / Status Menu Overlay | Party Creation / Field |
| **M** | Toggle Field Menu (Items, Equipment, Magic, Status) | Overworld & Towns |
| **B** | Trigger Area Monster Battle Simulation | Overworld & Dungeons |
| **C** | Trigger Opening Conelia Bridge Cutscene | Field |
| **P** | Trigger Secret 15-Puzzle Mini-Game | Aboard Ship |
| **T** | Warm Reset to Title Screen | General |
| **ESC** | Exit Application | Global |

---

## 🛠️ Building & Running

### Prerequisites
- **CMake** 3.20 or newer
- **C++20 Compatible Compiler** (MSVC 2022/2026, GCC 11+, or Clang 13+)

### 📦 Obtaining the Disassembly Data

The engine parses binary tables and CHR graphics directly from Disch's Final Fantasy I disassembly. Clone or download [Entroper/FF1Disassembly](https://github.com/Entroper/FF1Disassembly) adjacent to `ff1_cpp` (or inside `ff1_cpp`):

```bash
# In the parent directory containing ff1_cpp:
git clone https://github.com/Entroper/FF1Disassembly.git
```

Expected directory layout:
```text
Coding/
├── ff1_cpp/
│   ├── CMakeLists.txt
│   ├── src/
│   └── ...
└── FF1Disassembly/ (or FinalFantasyDisassembly_v1_0/)
    └── Final Fantasy Disassembly/
        ├── bin/
        │   ├── 0C_8000_weapondata.bin
        │   ├── 0C_8140_armordata.bin
        │   └── ...
        ├── bank_00.dat .. bank_09_data.bin
        └── ...
```

### Build Instructions

1. Navigate to the `ff1_cpp` directory:
   ```cmd
   cd ff1_cpp
   ```

2. Configure CMake:
   ```cmd
   cmake -B build -S . -DFF1_ENABLE_TESTS=ON
   ```

3. Build the project:
   ```cmd
   cmake --build build --config Release
   ```

### Running Executables

- **Launch the Game Engine**:
  ```cmd
  .\build\ff1_cpp.exe
  ```

- **Run Automated Verification Tests (25 Tests)**:
  ```cmd
  .\build\test_loader.exe
  ```

---

## 📜 License & Credits

- **Engine Source Code**: Licensed under the [MIT License](LICENSE).
- **Reference Disassembly**: [Disch (2015) Final Fantasy NES Disassembly](https://github.com/Entroper/FF1Disassembly).
- **Original Game & Assets**: Final Fantasy (NES) © Square Enix Co., Ltd. This project is an educational, non-commercial fan port for research and preservation purposes.

