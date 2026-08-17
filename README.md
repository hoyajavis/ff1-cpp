# Final Fantasy I (NES) C++ Port (`ff1_cpp`)

A modern **C++20** port of **Final Fantasy I (NES)** built from scratch using **Disch's 2015 Disassembly** as the authoritative source reference for original game data tables, math formulas, map formats, battle mechanics, and RAM/SRAM state structures.

---

## 🌟 Key Features & Technical Architecture

- **100% NES-Faithful Mechanics**: Authentic implementation of NES combat formulas, turn initiative ordering, physical hit/damage math, magic accuracy, elemental weaknesses/resistances, status ailments, enemy AI decision scripts, and 256-byte RNG sequences.
- **Disch 2015 Disassembly Data & Map Loader**: Parses binary asset tables directly from `FinalFantasyDisassembly_v1_0/`:
  - 40 Weapons (`0C_8000_weapondata.bin`)
  - 20 Armors (`0C_8140_armordata.bin`)
  - 64 Magic Spells (`0C_81E0_magicdata.bin`)
  - 128 Enemy Stat Entries (`0C_8520_enemydata.bin` & `0B_94E0_enemynames.bin`)
  - 128 Battle Formations (`0B_8400_battleformations.bin`)
  - 44 Enemy AI Decision Scripts (`0C_9020_aidata.bin`)
  - 48 World Shop Inventories (`0E_8300_shopdata.bin`)
  - 104 NPC Map Objects (`0E_95D5_objectdata.bin`)
  - Standard maps, TSA 16x16 macroblock tile blocks, locked door requirements, and teleport matrices
  - DTE (Dual Tile Encoding) text compression tables (`lut_DTE1` / `lut_DTE2`)
- **NES CHR Graphics Decoder & 15-Puzzle Mini-Game**:
  - `CHRDecoder` parsing 16-byte 2-bit planar NES CHR tile graphics directly from disassembly CHR ROM banks (`bank_02.dat`, `bank_03.dat`, `bank_05.dat`, `bank_07.dat`, `bank_09.asm`) and mapping to the authentic 64-color NES hardware RGB palette table (`lut_NESPalette`)
  - Interactive 4x4 sliding tile 15-Puzzle Mini-Game engine (`BANK_MINIGAME = $0D`) triggered aboard the Ship (key `P`), with time-scaled GP rewards (100 GP to 10,000 GP)
- **Full Battle Engine & Spell/Item Matrix**:
  - Full 64 White & Black Magic Spells (Healing, Revival, Elemental Damage, Stat Buffs/Debuffs, Instant Death, Status Ailments, and Escape/Warp)
  - Monster AI Decision Script Execution (`process_enemy_ai_turn`)
  - Equipped Item & Weapon Combat Use (`ActionType::ITEM`)
  - Status Ailment Matrix (Death, Stone, Paralysis, Poison, Blindness, Silence, Sleep, Confusion)
- **World Shops, Equipment & Class Promotions**:
  - Weapon, Armor, White/Black Magic Shops, Clinics (Resurrection), and Inns (HP/MP restore & SRAM file saving)
  - Real-time Equipment Manager recalculating Absorb, Evade penalty, HitRate, Damage, and Critical hit rates
  - King Bahamut Class Promotion System transforming base classes (Warrior ➔ Knight, Thief ➔ Ninja, Black Belt ➔ Master, Red Mage ➔ Red Wizard, White Mage ➔ White Wizard, Black Mage ➔ Black Wizard)
- **Audio Engine, HD Modding & Cutscene Engine**:
  - Audio Engine managing BGM tracks (Overworld, Town, Castle, Battle, Fanfare, Dungeon, Airship, Game Over) and SFX signals (Cursor move, Select, Hit, Magic, Door, Chest, Teleport) with soft-synth fallback
  - Native HD Modding System inspecting `./mods/` directory for optional PNG graphic overrides and WAV/OGG audio stream replacements
  - Cinematic Cutscene Engine controlling timing and subtitle scrolling for Opening Conelia Bridge, Airship Rising, and Ending Credits
- **Resolved NES Technical Quirks**:
  - *MMC1 Mapper Flattening*: Unified asset arrays eliminating 16KB PRG-ROM bank latency while retaining cross-bank jump logic.
  - *Hardware Timing & Split Screen*: Decoupled PPU cycle polling (e.g. VBLANK / Sprite 0 Hit) into a clean 60 FPS event loop with layered UI rasterization.
  - *Memory Pointer Emulation*: Encapsulated zero-page RAM structures into strongly-typed C++ structs (`GameSaveData`, `PartyCharacter`).
- **Native Port Advantages**:
  - *Widescreen (16:9) Mode*: Decoupled 256x240 view canvas allowing optional 16:9 widescreen expansion.
  - *Native Modding*: Prepared for PNG sprite/tile asset swapping and WAV/OGG audio stream replacement.
  - *Cross-Platform*: Single C++20 codebase targeting Windows, Linux, macOS, Steam Deck, and mobile devices.
- **NES Bug-Fix Toggle**: Option to toggle notorious NES FF1 bugs (e.g. Intelligence stat not affecting spell damage, weapon element/critical rate bugs) or play with authentic NES bug fixes enabled (`enable_bug_fixes = true`).

---

## 🐛 Known Graphical Issues & Pipeline Status

A detailed comparison between the C++ port build and the authentic NES hardware version identified the following remaining graphics pipeline tasks:

1. **Town & Dungeon Map CHR Tileset Pipeline**:
   - **Symptom**: Entering towns (such as `CONELIA TOWN`) renders overworld CHR tiles and red/blue braided tile patterns.
   - **Technical Cause**: `Renderer::draw_map` currently forces Overworld CHR (`bank_02.dat`) and Overworld TSA macroblock tables (`bank_00.dat`) across all map types.
   - **Fix**: Standard maps (`MapType::STANDARD_MAP`) must load town/dungeon tileset CHR from `bank_03.dat` / `bank_05.dat` and resolve town TSA macroblock tables from `bank_04.dat` / `bank_05.dat`.

2. **Overworld Map Tile Byte Masking & Ocean Waves**:
   - **Symptom**: Overworld map displays repeating leaf pattern tiles and checkerboard blocks instead of smooth grass, trees, and animated ocean waves.
   - **Technical Cause**: Overworld 256x256 map layout bytes from `bank_01_data.bin` require correct TSA macroblock index masking (0..127) and CHR bank alignment.

3. **Player & NPC Sprite CHR Alignment**:
   - **Symptom**: Character and NPC sprites display shifted sub-tile indices and background block elements.
   - **Technical Cause**: Player sprites (`LoadPlayerMapmanCHR`) require exact 16-tile bank offset calculations (`0x1000 + class_id * 0x100` in `bank_02.dat`) and OAM 2x2 sub-tile ordering with transparent background color keying (`color_idx == 0`).

---

## 📁 Project Architecture

```
ff1_cpp/
├── CMakeLists.txt                 # C++20 build configuration & SDL2 FetchContent setup
├── README.md                      # Project documentation
├── ROADMAP.md                     # Phased completion roadmap & technical specifications
├── src/
│   ├── main.cpp                   # Main game entry point & 60 FPS event loop
│   ├── data/
│   │   ├── game_types.hpp         # C++ structs (Party, Character, Weapon, Armor, Spell, Enemy, Formation, EnemyAIData, ShopInventory)
│   │   ├── map_types.hpp          # TSA tiles, Standard Map structures, Teleport & NPC spawn types
│   │   ├── data_loader.hpp / .cpp # Disassembly binary asset parser
│   │   ├── map_loader.hpp / .cpp  # Standard map, overworld, NPC object & teleport loader
│   │   └── text_decoder.hpp / .cpp# NES DTE text decoder
│   ├── engine/
│   │   ├── system.hpp / .cpp      # SDL2 windowing, texture updating & input dispatch
│   │   ├── renderer.hpp / .cpp    # 256x240/16:9 PPU tile map & entity rasterizer
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
│   │   ├── intro_engine.hpp / .cpp# New Game party creation & starting stats setup
│   │   ├── cutscene_engine.hpp / .cpp# Opening Bridge, Airship Rising & Ending Credits sequence player
│   │   └── minigame_engine.hpp / .cpp# 15-Puzzle Ship mini-game board & rewards
│   └── ui/
│       ├── window_box.hpp / .cpp  # Classic NES border box window renderer
│       └── font.hpp / .cpp        # 8x8 font character drawing
└── tests/
    └── test_loader.cpp            # Comprehensive 16-test automated verification suite
```

---

## 🎮 Controls

| Key | Action |
| :--- | :--- |
| **Arrow Keys / WASD** | Walk Player (Up, Down, Left, Right) |
| **SPACE / RETURN** | Action (Talk to NPC / Open Treasure Chest / Unlock Door / Confirm in Combat) |
| **M / TAB** | Toggle Party Status & Equipment Menu Overlay |
| **B** | Trigger Area Monster Battle Simulation |
| **C** | Trigger Opening Conelia Bridge Cutscene |
| **P** | Trigger Secret 15-Puzzle Mini-Game |
| **ESC** | Exit Application |

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
        ├── bank_02.dat
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

- **Run Automated Verification Tests**:
  ```cmd
  .\build\test_loader.exe
  ```

---

## 📜 License & Credits

- **Engine Source Code**: Licensed under the [MIT License](LICENSE).
- **Reference Disassembly**: [Disch (2015) Final Fantasy NES Disassembly](https://github.com/Entroper/FF1Disassembly).
- **Original Game & Assets**: Final Fantasy (NES) © Square Enix Co., Ltd. This project is an educational, non-commercial fan port for research and preservation purposes.

