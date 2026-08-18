# Final Fantasy I (NES) C++ Port (`ff1-cpp`)

A complete, modern **C++20** port of **Final Fantasy I (NES)** engineered from the ground up to achieve 100% feature completeness and total parity with **Disch's 2015 Disassembly**. The engine includes the full game storyline, all 64 standard maps + 256x256 Overworld streaming, interactive turn-based combat, 64 spells, vehicle dynamics (Ship, Canoe, Airship), all 4 Elemental Fiends, the 2000-year time loop final dungeon, Chaos final boss, and the complete Epilogue ending sequence.

---

## 🌟 Key Features & Technical Architecture

- **100% NES-Faithful Mechanics & Formulas**:
  - Exact 6502 disassembly parity for physical multi-hit scaling ($1 + \lfloor\frac{\text{Hit\%}}{32}\rfloor$), critical rolls, spell accuracy, elemental weaknesses/resistances ($1.5\times / 0.5\times$), status ailment matrix, and 256-byte RNG sequence (`$F100` / `$FCF1`).
  - Optional authentic bug-fix toggle (`enable_bug_fixes = true`) resolving original NES bugs (e.g. Intelligence stat affecting spell damage, weapon critical rates, and elemental weapon bonuses).
- **Disch 2015 Disassembly Binary Asset Pipeline**:
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
- **Full Visual Rendering Pipeline (256x240 & 16:9 Widescreen)**:
  - `CHRDecoder` decoding 16-byte 2-bit planar NES CHR tile graphics directly from ROM banks (`bank_02.dat` - `bank_09.dat`).
  - Dynamic Standard Map tileset switching across all 8 tilesets (Town, Castle, Cave/Dungeon, Temple/Fiend Lair, Volcano, Ice Cave, Floating Castle, Sea Shrine) and UI CHR.
  - Smooth palette shimmering for animated ocean waves, rivers, and coastlines.
  - Directional 2-frame walk cycle animations for all 12 player classes, 104 NPC map objects, and vehicles (Ship, Canoe, Airship) with alpha transparency keying.
- **Complete World Exploration & Vehicle Mechanics**:
  - **Walking & Port Traversals**: 4-way movement, collision checks, damaging lava tiles, poison step damage, and teleport triggers.
  - **Ship Sailing & Port Docking**: Boarding at parked ship coordinates and docking exclusively at stone ports (`is_port_tile`).
  - **Canoe River Navigation**: Traversal through rivers, lakes, and streams.
  - **Global Airship Flight**: Raising the Airship in Ryukahn Desert using the `FLOATER` (Levistone), 2x movement speed in the air, global terrain bypass (oceans, mountains, forests), shadow projection, and flat grassland landing restrictions.
- **Interactive Battle Engine & Sequential Combat Narrative**:
  - **Step-Forward Command Frame**: Active hero steps forward ($16\text{ px}$ left) with `[FIGHT | MAGIC | DRINK | ITEM | RUN]` and sub-menus.
  - **Dynamic Initiative Resolution**: $\text{Initiative} = \text{AGL} - \text{Rand}[0, 50]$, sorting party actions and monster turns into a priority queue.
  - **Surprise Encounters**: Preemptive Strikes and Monster Ambush support.
  - **Ineffective Redirection**: Attacks on previously slain targets display `"Ineffective!"`.
  - **Hero Battle Poses**: Standing, Step-Forward, Attack swing, and Low-HP / Fallen Crouch.
  - **Level-Up Progression**: HP Max gains, stat boosts (STR, AGI, INT, VIT, LUCK), MP tier expansions, and interactive popups.
- **Full Story Progression, Quest State Machines & Bosses**:
  - Rescue Princess Sarah from Garland in the Temple of Fiends $\rightarrow$ Northern Bridge built by the King of Conelia.
  - Defeat Bikke's Pirates in Pravoka $\rightarrow$ Ship granted.
  - Retrieve the Crown from Marsh Cave $\rightarrow$ Defeat King Astos $\rightarrow$ Matoya's Crystal Eye $\rightarrow$ Jolt Tonic / Herb $\rightarrow$ Awaken Elf Prince $\rightarrow$ Mystic Key.
  - Blast open the Canal with Dwarf Nerrick using TNT.
  - Feed Star Ruby to Giant Titan $\rightarrow$ Sarda's Earth Rod $\rightarrow$ Unseal Earth Cave B3.
  - **The Four Elemental Fiends**:
    - **Earth Fiend Lich** (Earth Cave B5) $\rightarrow$ Earth Orb restored.
    - **Fire Fiend Kary / Marilith** (Mt. Gurgu B5) $\rightarrow$ Fire Orb restored.
    - **Water Fiend Kraken** (Sunken Shrine 5F, diving with Oxyale) $\rightarrow$ Water Orb restored.
    - **Wind Fiend Tiamat** (Mirage Tower & Flying Fortress 5F, unsealed with Chime & Cube) $\rightarrow$ Wind Orb restored.
  - Rosetta Stone deciphering by Dr. Unne in Melmond $\rightarrow$ Lufenia Sky Civilization language translated.
  - Bahamut's Trial of Courage $\rightarrow$ Class Promotions (Warrior ➔ Knight, Thief ➔ Ninja, Black Belt ➔ Master, Red Mage ➔ Red Wizard, White Mage ➔ White Wizard, Black Mage ➔ Black Wizard).
  - Smyth forgives Excalibur from Adamantite in Dwarven Cave.
  - **The 2000-Year Time Loop Final Arc**:
    - 4 Orbs restore the Black Crystal in Temple of Fiends 1F.
    - Sarah's LUTE shatters the seal and opens the 2000-year time warp into the past.
    - **Temple of Fiends (Past) 5-Floor Labyrinth**: Rematches against **Lich 2**, **Kary 2**, **Kraken 2**, and **Tiamat 2**.
    - Legendary **Masamune** sword treasure chest.
    - **Final Boss CHAOS**: 2000 HP, 8 hits, elemental AoE spells (`CRACK`, `INFERNO`, `TSUNAMI`, `CYCLONE`, `NUKE`), and emergency full recovery via `CUR4`.
    - **Authentic Epilogue & Ending Sequence**: 4 Light Warriors on the cliff, scrolling story narrative text, developer credits, and final "THE END" screen.
- **Secret 15-Puzzle Mini-Game & Audio Engine**:
  - Interactive 4x4 sliding tile 15-Puzzle Mini-Game triggered aboard the Ship (key `P`), rewarding up to 10,000 GP.
  - Synthesized BGM tracks (Overworld, Town, Castle, Battle, Fanfare, Dungeon, Airship, Game Over) and SFX signals.
  - Native HD Modding directory `./mods/` scanning for optional PNG graphic overrides and WAV/OGG audio streams.

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
    └── test_loader.cpp            # Comprehensive 56-test automated verification suite
```

---

## 🎯 Master Roadmap Milestones (All 20 Phases Complete ✅)

| Phase | Milestone Name | Focus Area | Status |
| :---: | :--- | :--- | :---: |
| **1** | **Foundation & Data Architecture** | Binary loaders, DTE text decoding, PRNG, SRAM persistence | ✅ Completed |
| **2** | **Core Map & Exploration** | 256x240 renderer, 4-way movement, NPC talk, teleports | ✅ Completed |
| **3** | **World Map & Asset Integration** | TSA 16x16 macroblock parsing, NPC object matrix, doors | ✅ Completed |
| **4** | **Battle Engine & Spell Matrix** | 64 spells, combat formulas, status matrix, enemy AI | ✅ Completed |
| **5** | **Menu Engine & Class Upgrade** | Shops, equipment manager, Bahamut class promotions | ✅ Completed |
| **6** | **Audio Engine & HD Modding** | BGM synth/SFX, HD mod directory scanner, cutscene timer | ✅ Completed |
| **7** | **Multi-Platform Verification** | 16-test verification suite, multi-platform CMake builds | ✅ Completed |
| **8** | **Secret Mini-Game & Quests** | 15-Puzzle board logic, Excalibur quest, special events | ✅ Completed |
| **9** | **Core CHR Decoder & Palettes** | Universal BG/Sprite alpha separation, authentic ROM LUTs | ✅ Completed |
| **10** | **Complete Map Visual Pipeline** | 256x256 OW stream, 64 standard maps, sub-pixel camera | ✅ Completed |
| **11** | **Sprite & Entity Animation** | 2-frame walk cycles, 104 NPC CHRs, vehicle sprites | ✅ Completed |
| **12** | **Full Battle Visual Engine** | Monster CHR/TSA, backdrops, 12 hero poses, slash/spell FX | ✅ Completed |
| **13** | **Authentic UI & CHR Font** | 8x8 NES CHR font, authentic border boxes, cursor engine | ✅ Completed |
| **14** | **Cinematics & HD Widescreen** | Bridge cutscene nametable, 1bpp puzzle CHR, 16:9 canvas | ✅ Completed |
| **15** | **Title Screen & Party Creation** | 2x2 character matrix, job sprite walk cycles, virtual keyboard | ✅ Completed |
| **16** | **Field Menu & Sub-Screens** | 4-Orb HUD, 8-tier magic matrix, EQUIP/TRADE/DROP, STATUS | ✅ Completed |
| **17** | **Shop & Town Service Flows** | Weapon/Armor/Magic shops, counter barriers, Inn/Clinic save | ✅ Completed |
| **18** | **Battle Turn & Combat Narrative** | Step-forward lineup, multi-hit narrative log, level-up popup | ✅ Completed |
| **19** | **Airship Flight & Four Fiends** | Floater raising, global flight, Kary/Kraken/Tiamat, 4 Orbs | ✅ Completed |
| **20** | **Temple of Fiends Past & Chaos** | 2000-year time warp, 4 Fiends rematch, Chaos boss, Epilogue | ✅ Completed |

---

## 🎮 Controls

| Key | Action | Context |
| :--- | :--- | :--- |
| **Arrow Keys / WASD** | Walk Player / Navigate Menus & Combat Cursors | Field / Menus / Combat |
| **SPACE / RETURN / Z** | Confirm / Talk / Loot Chest / Select Spell / Attack | General / Menus / Combat |
| **BACKSPACE / ESC / X** | Cancel / Backtrack Party Member / Exit Sub-Menu | Menus / Combat |
| **L** | Land Airship (Flat Grasslands Only) | Aboard Airship on Overworld |
| **M / TAB** | Toggle Field Menu (Items, Equipment, Magic, Status) | Overworld & Towns |
| **P** | Trigger Secret 15-Puzzle Mini-Game | Aboard Ship |
| **B** | Trigger Area Monster Battle Simulation | Overworld & Dungeons |
| **C** | Trigger Opening Conelia Bridge Cutscene | Field |
| **T** | Warm Reset to Title Screen | General |
| **ESC** | Exit Application | Global |

---

## 🛠️ Building & Running

### Prerequisites
- **CMake** 3.20 or newer
- **C++20 Compatible Compiler** (MSVC 2022/2026, GCC 11+, or Clang 13+)

### 📦 Obtaining the Disassembly Data

The engine parses binary tables and CHR graphics directly from Disch's Final Fantasy I disassembly. Clone or download [Entroper/FF1Disassembly](https://github.com/Entroper/FF1Disassembly) adjacent to `ff1-cpp` (or inside `ff1-cpp`):

```bash
# In the parent directory containing ff1-cpp:
git clone https://github.com/Entroper/FF1Disassembly.git
```

Expected directory layout:
```text
Coding/
├── ff1-cpp/
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

1. Navigate to the `ff1-cpp` directory:
   ```cmd
   cd ff1-cpp
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

- **Run Automated Verification Tests (56 Tests)**:
  ```cmd
  .\build\test_loader.exe
  ```

---

## 📜 License & Credits

- **Engine Source Code**: Licensed under the [MIT License](LICENSE).
- **Reference Disassembly**: [Disch (2015) Final Fantasy NES Disassembly](https://github.com/Entroper/FF1Disassembly).
- **Original Game & Assets**: Final Fantasy (NES) © Square Enix Co., Ltd. This project is an educational, non-commercial fan port for research, archival, and preservation purposes.

