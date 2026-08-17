# Final Fantasy I (NES) C++ Port - Comprehensive Project Roadmap

This document establishes the master architectural roadmap, data table references, technical specifications, and phased implementation milestones for bringing the **Final Fantasy I (NES) C++ Port (`ff1_cpp`)** to 100% authentic NES parity and modern C++20 excellence based on **Disch's 2015 Disassembly**.

---

## 🏗️ NES Architecture & Native C++ Port Philosophy

Rewriting an 8-bit NES title natively into modern C++20 introduces unique technical challenges compared to emulating 6502 assembly or porting 16/32-bit titles. Our architecture resolves these challenges while unlocking the full benefits of native modern execution:

### ⚡ Technical Challenge Resolutions

1. **Cartridge Mapper Handling (MMC1 / Mapper 1)**
   - *Challenge*: Original NES FF1 uses an MMC1 mapper chip to swap 16 KB PRG-ROM banks (`BANK_00` through `BANK_0F`) into the 6502's `$8000-$BFFF` address space.
   - *Port Solution*: **Flattened Asset Data Abstraction**. Rather than simulating 6502 bank-switching hardware registers (`$9000` bit shifts), all ROM tables, monster stats, spell definitions, and map structures are parsed into unified memory arrays ([`DataLoader`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/data/data_loader.hpp)) accessible instantly by C++ subsystems without bank latency. Cross-bank jump routines (`DoCrossPageJump`, `BankC_CrossBankJumpList`) are replaced with typed C++ method dispatches.

2. **Hardware-Driven Logic & Cycle Accuracy**
   - *Challenge*: NES games tied frame updates to PPU/VBLANK cycles (1.79 MHz NTSC) and hardware status polling (e.g. Sprite 0 Hit for status bar screen splits).
   - *Port Solution*: **Fixed-Timestep Loop & Layered Rasterizer**. Hardware cycle polling is replaced with a clean 60 FPS event loop ([`System::poll_events`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/engine/system.cpp#L72) and frame pacing). Screen splits (combat text boxes, dialogue windows, HUD bars) are rendered as decoupled UI composite layers over the tile camera canvas.

3. **Memory Pointer Emulation & Zero-Page RAM Mapping**
   - *Challenge*: NES assembly relied heavily on zero-page pointers (`$00-$FF` RAM), indirect Y-indexed addressing `(tmp), Y`, and runtime memory mutation.
   - *Port Solution*: **Strongly-Typed State Encapsulation**. Zero-page variables listed in `variables.inc` (`ch_stats`, `cur_map`, `vehicle`, `party_gold`) are encapsulated within `GameSaveData` and `PartyCharacter` structs, preserving original NES RAM layouts while providing type safety and memory stability.

---

### 🚀 Key Advantages of Native C++ Porting

1. **Widescreen (16:9) & High-Resolution Viewports**
   - Decouples tile rendering from the fixed 256x240 NES pixel budget, enabling optional 16:9 widescreen expansion (e.g. 40x15 visible tile grid) without sprite distortion.
2. **High Framerate Motion Interpolation**
   - Supports 60 FPS+ unlocked frame rates with sub-pixel camera scrolling and smooth sprite movement.
3. **Native HD Modding & Asset Swapping**
   - Allows drop-in replacement of 2-bit CHR tiles and APU audio streams with standard PNG images, custom HD sprites, and WAV/OGG high-fidelity audio tracks.
4. **Cross-Platform Portability**
   - Single C++20 codebase compiles natively across Windows (MSVC/Clang), Linux, macOS, Steam Deck, Android, iOS, and handheld devices via CMake.

---

## 🎯 Master Progress Overview

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
| **16** | **Field Menu & Sub-Screens** | 4-Orb HUD, 8-tier magic matrix, EQUIP/TRADE/DROP, STATUS | 🔲 Planned |
| **17** | **Shop & Town Service Flows** | Weapon/Armor/Magic shops, counter barriers, Inn/Clinic save | 🔲 Planned |
| **18** | **Battle Turn & Combat Narrative** | Step-forward lineup, multi-hit narrative log, level-up popup | 🔲 Planned |

---

## 📋 Detailed Milestone Roadmap

### Phase 1: Foundation & Data Architecture (Completed ✅)
- [x] **C++20 & Build Pipeline**: Set up `CMakeLists.txt` with SDL2 `FetchContent` for hardware-accelerated 256x240 pixel canvas rendering and cross-platform compilation.
- [x] **Binary Data Loader**: Implemented `data_loader.cpp` to parse disassembly binary tables (`0C_8000_weapondata.bin`, `0C_8140_armordata.bin`, `0C_81E0_magicdata.bin`, `0C_8520_enemydata.bin`, `0B_8400_battleformations.bin`).
- [x] **DTE Text Decoder**: Implemented `text_decoder.cpp` matching NES DTE (Dual Tile Encoding) compressed string tables.
- [x] **NES PRNG**: Implemented `rng.cpp` matching the authentic 256-byte lookup sequence at `$F100` / `$FCF1`.
- [x] **SRAM Save System**: Implemented `save_system.cpp` for 8KB save file persistence.
- [x] **Automated Test Suite**: Created `test_loader.cpp` unit test verification suite.

---

### Phase 2: Core Map & Exploration (Completed ✅)
- [x] **256x240 Tile Renderer**: Implemented `renderer.cpp` with camera viewport centering on player tile coordinates.
- [x] **4-Directional Movement**: Implemented directional walking (`UP`, `DOWN`, `LEFT`, `RIGHT`) with mountain/wall collision checks in `map_engine.cpp`.
- [x] **NPC Talk & Chest System**: Added interactive NPC dialog popups and treasure chest GP/item looting.
- [x] **Map Transitions**: Implemented teleport transitions between Overworld map and Conelia Town/Dungeons.
- [x] **Turn-Based Combat Simulation**: Implemented physical attack turn processing, damage/hit math, and battle log UI.
- [x] **Status Menu Overlay**: Implemented Party Status & Gold display overlay.

---

### Phase 3: World Map & Asset Integration (Completed ✅)
- [x] **TSA Tile Block Decoder**: Parsed 16x16 macroblock TSA tables from disassembly `bank_00.dat` - `bank_0A.dat` and `bank_01_data.bin` for exact NES graphics rendering.
- [x] **NPC Object Matrix**: Parsed 104 NPC objects from `0E_95D5_objectdata.bin` to populate every town and castle NPC entity across the world.
- [x] **Door & Key Requirement Matrix**: Implemented locked door unlocking logic using key items (LUTE, CROWN, Mystic KEY, CUBE, ROD, and the 4 ORBS).
- [x] **Damaging & Special Tiles**: Implemented Lava / Poison swamp damage tiles (-1 HP per step) and Teleport Warp tiles.

---

### Phase 4: Complete Battle Engine & Spell/Item Matrix (Completed ✅)
- [x] **Full 64 Spells Logic**: Implemented exact battle & out-of-battle effects for all 64 White and Black magic spells.
- [x] **Status Ailment Matrix**: Full persistence and recovery logic for Death, Stone, Paralysis, Poison, Blindness, Silence, Sleep, and Confusion.
- [x] **Enemy AI Script Engine**: Parsed and executed monster AI routines from `0C_9020_aidata.bin` (44 AI decision scripts).
- [x] **Equipment Item Battle Use**: Implemented using equipped weapons/armors in combat to trigger free spell casts (`ActionType::ITEM`).

---

### Phase 5: Full Menu Engine, Shops & Class Upgrade (Completed ✅)
- [x] **In-Depth World Shops**:
  - **Weapon & Armor Shops**: Buying, selling, class equipment restrictions, GP balance checks (`0E_8300_shopdata.bin`).
  - **White & Black Magic Shops**: Purchasing level 1-8 spells per character class permissions.
  - **Clinics & Inns**: Resurrecting fallen party members, HP/MP restoration, and SRAM save persistence.
- [x] **Equipment & Stat Manager**: Full equipment assignment, evade penalty math, absorb calculation, and category bonuses.
- [x] **Bahamut Class Upgrade System**: Transformed base classes upon presenting the TAIL to Bahamut.

---

### Phase 6: Audio Engine, HD Modding & Cinematic Cutscenes (Completed ✅)
- [x] **Audio Engine Core**: Implemented BGM music tracks and SFX signals with soft-synth fallback.
- [x] **HD Asset Swapping & Modding Engine**: Scanned `./mods/` directory for optional HD PNG graphics and WAV/OGG audio replacements.
- [x] **Cinematic Cutscenes & Story Events**: Implemented subtitle timing and frame sequence player for Opening Bridge, Airship Rising, and Ending Credits.

---

### Phase 7: Multi-Platform Builds & Playthrough Verification (Completed ✅)
- [x] **Multi-Platform Targets**: Built & verified native packages for Windows, Linux, macOS, and handheld devices.
- [x] **End-to-End Test Suite**: Comprehensive 16-test automated verification suite (`test_loader.exe`) — **100% PASSED**.

---

### Phase 8: Secret Mini-Game, Special Event Quests & Combat Nuances (Completed ✅)
- [x] **Secret 15-Puzzle Mini-Game (`BANK_MINIGAME = $0D`)**: Interactive sliding 15-puzzle minigame board with solvable permutation checks and time-scaled GP rewards.
- [x] **Smyth Dwarf Smith & Excalibur Forging (`WPNID_XCALBUR = $27`)**: Mt. Duergar Adamant quest handler.
- [x] **Oasis Caravan & Fairy Bottle Quest**: Caravan shop logic, Fairy release in Gaia, and Oxyale acquisition.
- [x] **Fixed Tile Monster Encounters**: Standard map tile encoding `ssss = 0101` triggering forced non-random battles.

---

### Phase 9: Core CHR Decoder & Palette Modernization (Completed ✅)
- [x] **RGBA32 Bitplane Decoder Engine**:
  - Convert 2bpp NES planar tile pairs (16 bytes = Plane 0 + Plane 1) into indexed 8x8 pixel matrices (`PixelBuffer8x8`).
  - Map 2-bit color indices (0..3) to 32-bit RGBA pixels using `lut_NESPalette[64]`.
- [x] **Background vs. Sprite Alpha Separation**:
  - **Background Tiles**: Color 0 maps to `lut_NESPalette[palette[0]]` (universal background color `$3F00`), preventing background transparency bleeding.
  - **Sprite Tiles**: Color 0 maps to `0x00000000` (transparent) for authentic OAM sprite keying.
- [x] **ROM Palette Lookup Tables**:
  - Overworld Palettes: 4 palette sets mapped from `bank_00.dat`.
  - Standard Map Palettes: 48 bytes per map loaded from `lut_SMPalettes` (`bank_00.dat` offset `0x2000`).
  - Hero Class Palettes: 16 bytes per class loaded from `lut_MapmanPalettes` (`bank_00.dat` offset `0x03A0`).
  - Monster / Battle Palettes: 4-byte palettes loaded from `0C_8F20_battlepalettes.bin`.
- [x] **Tile-Level Mirroring (H-Flip / V-Flip)**:
  - Implement horizontal/vertical pixel mirroring using NES OAM attribute flags (`bit 6` = H-flip, `bit 7` = V-flip).

---

### Phase 10: World Map & Standard Map Visual Pipeline (Completed ✅)
- [x] **256x256 Authentic Overworld Map Stream**:
  - Load and decompress `bank_01_data.bin` (65,536 bytes) using `lut_OWPtrTbl` (`$8000`), replacing procedural mockups.
  - Decode TSA macroblocks (0..127) using `bank_00.dat` offset `0x0000`.
- [x] **Dynamic Tileset & TSA Switcher**:
  - Map each `map_id` to its `tileset_id` via `lut_Tilesets` (`bank_00.dat` offset `0x2CC0`).
  - Read tileset CHR from `bank_03.dat` (Tilesets 0-1) or `bank_05.dat` (Tilesets 2-3) and TSA from `lut_SMTilesetTSA` (`bank_00.dat` offset `0x1000`).
- [x] **Animated Ocean Waves**:
  - Implement 4-frame water tile cycling on overworld water tiles.

---

### Phase 11: Sprite, Entity, Vehicle & OAM Animation Engine (Completed ✅)
- [x] **2-Frame Hero Walk Cycle State Machine**:
  - Alternate between Frame 0 and Frame 1 using `lut_PlayerMapmanSprTbl` (`bank_0F.asm:8751`) based on movement frame ticks.
- [x] **12 Hero Class Mapman Sprites & Promoted Class Palettes**:
  - Load base and upgraded class sprites from `bank_02.dat` offset `0x1000 + (class_id * 0x100)`.
  - Map class palettes accurately via `lut_MapmanPalettes`.
- [x] **104 Dynamic NPC Object Renderers**:
  - Load 16x16 sprite tiles from `lut_MapObjCHR` (`bank_02.dat` offset `0x1C00` / `0x2200`).
- [x] **Vehicle Sprite Rendering**:
  - Ship: Boarding replaces player with Ship sprite (`bank_02.dat` offset `0x1C00`).
  - Airship: Renders Airship sprite (`bank_02.dat` offset `0x1D00`).

---

### Phase 12: Full Battle Visual & Combat Animation Engine (Completed ✅)
- [x] **Battle Backdrop Arena Rasterizer**:
  - Render battle background arena in upper screen half with horizon divider.
- [x] **Monster Graphics & Formation Assembler**:
  - Render monsters in formation slots using `bank_07.dat` / `bank_08.dat`.
  - Apply authentic monster palette tables from `0C_8F20_battlepalettes.bin`.
- [x] **4-Player Flank Battle Party Sprites**:
  - Load battle party sprites with class palettes.
  - Implement battle poses: Standing, Attack, and Low-HP / Fallen Crouch.
- [x] **Authentic Battle HUD & Command Windows**:
  - Render classic split-screen command boxes: `[FIGHT | MAGIC | DRINK | ITEM | RUN]`.
  - Display party member HP status panels with authentic borders and battle combat banner.

---

### Phase 13: Authentic NES UI, CHR Font & Menu Windows (Completed ✅)
- [x] **Classic Double-Line NES Window Frames**:
  - Implement double-line beveled window box renderer with solid black interior fill and outer rounded corner strokes.
- [x] **Interactive Menu & HUD Overlays**:
  - Integrated Header HUD, Dialogue boxes, Party status menus, and Battle command frames.
  - Implement shop transaction interfaces (Weapon, Armor, Magic, Inn, Clinic) with cursor selection.

---

### Phase 14: Cinematic Sequences, 15-Puzzle & HD Modding Pipeline (Completed ✅)
- [x] **Opening Conelia Bridge Cutscene**:
  - Load nametable tile map and CHR from `0B_A800_endingbridge_chrnt.bin`.
  - Render the 4 Light Warriors overlooking Castle Conelia on the bridge with narrative prologue subtitles.
- [x] **15-Puzzle 1bpp Graphics**:
  - Implement `CHRDecoder::decode_1bpp_tile` to decode 1bpp number tiles from `0D_9E00_puzzle_1bpp.chr`.
  - Integrated 4x4 sliding puzzle rasterization with move tracker in `Renderer::draw_puzzle`.
- [x] **Ending Credits & "The End" Sequence**:
  - Load ending scene assets `0D_A000_theenddrawdata.bin` and `0D_A681_theendluts.bin`.

---

### Phase 15: Title Screen, Party Creation & Virtual Keyboard Engine (Completed ✅)
- [x] **Title Screen Flow**:
  - Implement centered 3 vertical bordered boxes on black backdrop: `[CONTINUE | NEW GAME | RESPOND RATE 1-8]`.
  - Display authentic bottom copyright banner: `© 1987 SQUARE / © 1990 NINTENDO`.
  - Configurable cursor message speed (RESPOND RATE 1-8).
- [x] **2x2 Party Creation Grid**:
  - 4-slot blue window box matrix for Party Slots 1 to 4 (Top-Left, Top-Right, Bottom-Left, Bottom-Right).
  - Job class cycling (FIGHTER, THIEF, Bl.BELT, RedMAGE, Wh.MAGE, Bl.MAGE) with animated 2-frame walk cycle sprite preview.
- [x] **Alphanumeric Virtual Keyboard Box**:
  - 4-row character matrix (`A-Z`, `0-9`, `a-z`, symbols `! ? ' , . -`).
  - 4-character naming lock advancing sequentially to slots 1 through 4.

---

### Phase 16: Comprehensive Field Menu, Equipment Sub-Screens & Global Mini-Map (Planned 🔲)
- [ ] **Main Field Pause Menu (`M` / `Tab` / `Start`)**:
  - **4-Orb Status Box**: Top-left gem status (Earth, Fire, Water, Wind) reflecting active/restored crystal states (`orbs_lit[4]`), switching from dark circles (Palette 3) to glowing radiant orbs (Palette 0).
  - **Gold Box**: Real-time GP counter display (`[Gold] G`, capped at 999,999 GP).
  - **Command Menu Box**: 5 vertical action entries: `[ITEM | MAGIC | WEAPON | ARMOR | STATUS]`.
  - **Party Member Cards**: Character name, level (`L 1`), animated class sprite, HP (`curr/max`), status flag overlay, and 8-tier magic slot availability matrix (`2/0/0/0/0/0/0/0`).
- [ ] **Party Lineup Reordering UX Flow**:
  - Triggered by pressing `SELECT` on overworld or moving cursor to blank character header in main menu.
  - Selecting Slot $A$ and Slot $B$ swaps full character records.
  - Overworld/town leader avatar dynamically updates to match the class of whoever occupies Slot 1.
  - Dynamically updates enemy target priority weighting: $\text{Slot 1} = 50\%$, $\text{Slot 2} = 25\%$, $\text{Slot 3} = 12.5\%$, $\text{Slot 4} = 12.5\%$.
- [ ] **Full-World Mini-Map Mode (`B + SELECT` / dedicated hotkey)**:
  - Decompresses and downsamples the $256 \times 256$ Overworld map into a $128 \times 128$ pixel miniature display in PPU Pattern Table 0.
  - Frames the canvas with 4 ornate corner dragon/gargoyle border glyphs.
  - Overlays a blinking `+` crosshair tracking the party's current global $(X, Y)$ coordinates in real-time.
  - Plays Prelude / Crystal Theme track ($41$).
- [ ] **Consumable Item Menu & Camping Pipeline (`ITEM`)**:
  - **Potion Quantity Counters**: Displays item counts (`HEAL *99`, `PURE *58`, `SOFT *12`).
  - **Rapid HEAL Potion Spamming**: Prompt `"Who needs to recover HP?"` restores 30 HP with **cursor retention** in party selector for fast consecutive uses.
  - **Overworld Camping Items (TENT, CABIN, HOUSE)**:
    - Usable only on Overworld (rejected with `"You cannot use it here!"` inside dungeons).
    - TENT: $+30\text{ HP}$ to conscious members + interactive `SAVE? Push A....YES / Push B....NO` dialog.
    - CABIN: $+60\text{ HP}$ + save confirmation.
    - HOUSE: $+120\text{ HP}$ + full MP restoration + save confirmation.
    - Interactive SRAM write saving binary state and displaying `"Now saving....!"`.
  - **Key Item Inspection Descriptions**: Highlighting key items displays descriptions (e.g. `The stolen CROWN.`, `The ROD to remove the plate from the earth.`).
- [ ] **Equipment Sub-Screen (`WEAPON` / `ARMOR`)**:
  - 3 Header tabs: `[EQUIP | TRADE | DROP]`.
  - **`EQUIP`**: 4-slot per-character inventory with `E-` equipped prefix toggle. Equipping/unequipping recalculates Damage, Hit %, Absorb, and Evade % live.
  - **`TRADE`**: Cursor selects an item in Character $A$'s bag, then switches to swap with a slot in Character $B$'s bag, resetting `E-` flags and updating both characters' stats.
  - **`DROP`**: Prompts confirmation to permanently discard an item, freeing space within the strict 4-item per-character cap.
- [ ] **Magic Sub-Screen (`MAGIC`)**:
  - 8-tier spellbook matrix with 3 slots per tier and charges (`2/2`).
  - Field spell casting prompt (e.g., `CURE`, `HEAL`, `LIFE`, `PURE`, `SOFT`) with 4-member party targeting.
- [ ] **Status Sub-Screen (`STATUS`)**:
  - Header: Character Name, Class Sprite/Title, and Level (`Nobi FIGHTER LEV 3`).
  - EXP Box: `EXP. POINTS` and `FOR LEV UP`.
  - Base Stats: `STR.`, `AGI.`, `INT.`, `VIT.`, `LUCK`.
  - Derived Parameters: `DAMAGE`, `HIT %`, `ABSORB`, `EVADE %`.
- [ ] **Overworld Poison Step Ticking**:
  - Walking on foot with poisoned party members deals $1\text{ HP}$ damage per step (stopping at minimum $1\text{ HP}$).
  - Plays down-sweeping harsh pulse SFX and flashes the screen red on damage ticks.

---

### Phase 17: Shop Transaction Systems, Quest State Machines & Dungeon Events (Planned 🔲)
- [ ] **Vehicle Navigation & Port Docking System (The Ship)**:
  - Walking from land onto the docked ship transforms the player sprite into the ship and toggles water-only movement physics.
  - Strict docking constraint: Ship can only disembark onto designated stone dock/port tiles.
  - Aquatic battle encounter tables & split-horizon ocean battle backdrops.
- [ ] **Dedicated Weapon & Armor Shops**:
  - 4-part split layout with shopkeeper counter barrier and customer sprite.
  - Menu options: `[Buy | Sell | Exit]` with current GP.
  - Item catalog with weapon/armor icons and prices.
  - `Who will take it?` party recipient selector with 4-slot capacity validation.
  - **`Sell` Sub-Menu**: Quotes $\mathbf{50\%}$ of base purchase price (`Sell_Price = Base_Price >> 1`) with `[Price] Gold OK? (Yes / No)` confirmation.
- [ ] **White & Black Magic Shops**:
  - `Who will learn the spell?` caster selector.
  - 4 available tier spells catalog with class compatibility and 3-spell tier capacity validation.
- [ ] **Town Services (Inn & Clinic)**:
  - **Inn**: `[Cost] Gold OK?` prompt, screen night fade, resting fanfare, HP/MP restoration, SRAM save, and `"Hold RESET while turning POWER off"` notice.
  - **Clinic**: `"Who shall be revived ...."` selector, town-scaled revival pricing, resuscitating fallen heroes with **exactly $1\text{ HP}$**.
  - **Ruined Town State (Melmond)**: Broken perimeter walls, gravestones, and Tier 5 Magic shops (`CUR3`, `HRM3`, `FIR3`, `BANE`, `WARP`).
- [ ] **Story Progression & Quest State Machines**:
  - **Garland & Princess Sarah**: Rescuing Sarah sets `SARAH_RESCUED` $\rightarrow$ King of Conelia builds Northern Bridge cutscene $\rightarrow$ bridge passage unlocked.
  - **Pirates & Ship**: Defeating Bikke's 9 Pirates awards the Ship (`ship_vis = 1`).
  - **Marsh Cave & Crown Quest**: Marsh Cave B3 spike tile $\rightarrow$ defeat Piscodemons/Wizards $\rightarrow$ obtain `CROWN`.
  - **King Astos Reveal**: Presenting `CROWN` reveals King as Astos $\rightarrow$ defeat Astos $\rightarrow$ acquire `CRYSTAL` ($750\text{ EXP}$, $2,000\text{ GP}$, `OBJID_ASTOS` hidden).
  - **Matoya's Herb Trade**: Presenting `CRYSTAL` to Witch Matoya trades for `HERB`.
  - **Elf Prince Awakening**: Giving `HERB` wakes the sleeping prince $\rightarrow$ awards permanent `MYSTIC KEY`.
  - **Mystic Key Door Unlocking**: Interacting with stone vault doors checks `has_mystic_key`, plays door open chime, and mutates tile to open doorway.
  - **Treasure Vault Looting & Capacity Constraints**: Gold chests increment GP; equipment chests reject looting with `"Can't carry any more equipment."` if character bags are full.
  - **Canal Demolition**: Delivering `TNT` to Nerrick plays demolition sequence, sets `canal_vis = 0`, and permanently converts overworld canal barrier into passable sea water.
  - **Titan's Tunnel & Sarda's Cave**: Feeding `RUBY` to the giant Titan opens western passage $\rightarrow$ Sage Sarda grants the `ROD`.
  - **Earth Cave B3 Stone Plate Unsealing**: Using `ROD` on stone slab displays `"The plate shatters, revealing a stairway!"` and mutates slab into downward staircase to B4.
  - **Fiend Altar Sequence**: Stepping on altar behind defeated Fiend plays beam raster effect, sets `orbs_lit[EARTH] = true`, updates 4-Orb HUD, and triggers instant exit warp to overworld.
- [ ] **Dungeon Coordinate Spike Encounters**:
  - Hardcoded tile triggers (`tileprop+1 & 0x80 == 0`) executing mandatory $100\%$ forced battles (Piscodemons, Earth Elementals, Gargoyles).

---

### Phase 18: Battle Turn Step-Forward & Sequential Combat Narrative (Planned 🔲)
- [ ] **Turn Command Step-Forward Flow**:
  - Sequential active hero step-forward animation ($16\text{ px}$ left) with command frame: `[FIGHT | MAGIC | DRINK | ITEM | RUN]`.
  - Target selection cursor with B-button cancel-back to previous party member.
  - Hero battle poses: Standing, Attack swing, and Low-HP / Fallen Crouch.
- [ ] **Combat Initiative & Surprise Rounds**:
  - Initiative resolution per round: $\text{Initiative} = \text{AGL} - \text{Rand}[0, 50]$.
  - Preemptive Strike: Displays `"Chance to strike first!"` before Round 1; party acts with full free turn.
  - Ambush: Displays `"Monsters strike first!"`; enemies execute full attack turn before player menu inputs open.
- [ ] **Sequential Combat Narrative Engine**:
  - Actor & Target banner box: `[Actor Name] -> [Target Name]`.
  - Multi-Hit Scaling: $\text{Hits} = 1 + \lfloor\frac{\text{Hit\%}}{32}\rfloor$, outputting `X Hits!`, `X DMG`.
  - RNG Criticals: Weapon-specific crit check outputting `Critical hit!!` with screen flash.
  - Ineffective Targeting: If target dies before strike executes, displays `Ineffective` and clears combat boxes.
  - Status messages: `Poisoned`, `Paralyzed`, `Muted!`, `Missed!`, `Awoke!`, `Cured!`, `Neutralized`.
- [ ] **Status Ailment & Combat Math Pipeline**:
  - Affliction priority: $\mathbf{Dead (0\text{ HP})} > \mathbf{Stone} > \mathbf{Poison (PO)} > \mathbf{Paralysis (PA/ST)}$.
  - Darkness: $-40\text{ Hit Rate}$ penalty.
  - Silence: Spell lock.
  - Paralysis: Turn skip + per-round recovery resistance roll.
  - Sleep: Turn skip + physical hit wakeup roll ($\text{MaxHP} > \text{Rand}[0, 80]$).
  - Permanent Stone: Terminal petrification removing character from turn queue; concurrent 4-member Dead/Stone triggers immediate `Party Perished` game over.
  - Undead Vulnerabilities: `HARM` series (`HRM2`, `HRM3`) and `FIRE` series (`FIR2`, `FIR3`) deal amplified damage against `Category::UNDEAD`.
  - Buff Stacking: `FAST` ($\times 2\text{ Hits}$), `FOG` ($+8\text{ Absorb}$), `RUSE` ($+80\text{ Evade}$), `SABR` ($+16\text{ Dmg}, +40\text{ Hit}$).
  - Weapon Family / Elemental Bonus: $+4\text{ Damage}$ and $+40\text{ Hit Rate}$ when `enable_bug_fixes == true`.
- [ ] **Boss AI Engines**:
  - **King Astos**: `RUB -> SLO2 -> FAST -> FIR2` ($750\text{ EXP}$, $2,000\text{ GP}$).
  - **Lich (Fiend of Earth)**: `ICE2 -> SLP2 -> FAST -> LIT2 -> FIR3 -> HOLD/SLEP` ($550\text{ EXP}$, $3,000\text{ GP}$).
  - **Un-Escapable Formations**: Bosses and spike guardians have `unrunnable` flag set, causing `RUN` attempts to output `"Can't run"` sequentially.
- [ ] **Escape Mechanics**:
  - Escapable check: $\text{Luck} > \text{Rand}[0, \text{Level} + 15]$.
  - Failed attempt: outputs `"Can't run"`.
  - Successful escape: outputs `"Close call....."` and terminates combat.
- [ ] **Victory & Level-Up Sequence**:
  - `Monsters perished` victory banner + reward summary (`EXP UP [X]P`, `GOLD [X]G`).
  - Level-Up Popups: `Lev. up! [Name] L2`, `HP max [X]pts.`, stat gain readouts (`Str. up`, `Agi. up`, `Vit. up`, `Luck up`, `Int. up`).

## 🛡️ Risk Mitigation & Architectural Safeguards

```
                      RISK MITIGATION ARCHITECTURE
┌───────────────────────────────────┬───────────────────────────────────┐
│              RISK                 │            MITIGATION             │
├───────────────────────────────────┼───────────────────────────────────┤
│ 1. Asset File Path Portability    │ Multi-tier fallback path searcher │
│    (Running from diff CWDs)       │ + embedded ROM fallback buffers   │
├───────────────────────────────────┼───────────────────────────────────┤
│ 2. CHR Decoding Performance       │ Pre-decode and cache tile buffers │
│    (Per-frame 2bpp decoding overhead) │ (RGBA Tile Atlas) at startup │
├───────────────────────────────────┼───────────────────────────────────┤
│ 3. Gameplay Logic Regressions     │ Strict decoupling of MapEngine /  │
│    (Altering collision/events)    │ BattleEngine from Renderer view   │
├───────────────────────────────────┼───────────────────────────────────┤
│ 4. Visual Verification / Testing  │ Automated frame dumper + 16       │
│                                   │ automated loader verification tests│
└───────────────────────────────────┴───────────────────────────────────┘
```

1. **Asset Resolution & Graceful Fallback**:
   - *Strategy*: Maintain the recursive upward path search in [`DataLoader::resolve_asset_path`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/data/data_loader.cpp#L10) and retain programmatic fallback geometry/colors if files are missing.
2. **Tile Atlas Caching (Zero Runtime Decoding Lag)**:
   - *Strategy*: Pre-decode active CHR banks into 32-bit RGBA pixel tile atlases at map/battle initialization. Runtime rendering operates via direct memory copies / blits.
3. **Strict Model-View Separation**:
   - *Strategy*: Ensure [`BattleEngine`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/core/battle_engine.hpp) and [`MapEngine`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/core/map_engine.hpp) remain pure data models. [`Renderer`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/engine/renderer.hpp) acts strictly as a read-only observer consuming state structs.
4. **Visual Regression Suite**:
   - *Strategy*: Implement an automated headless frame capture test utility (`dump_frame`) to verify rendered frames against authentic NES baseline screenshots.

---

## 🛠️ Architecture Component Map

| Component | Files | Primary Responsibility |
| :--- | :--- | :--- |
| **Data Architecture** | [`game_types.hpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/data/game_types.hpp), [`data_loader.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/data/data_loader.cpp), [`text_decoder.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/data/text_decoder.cpp) | Binary asset parsing, DTE decoding, typed game structs |
| **System & Renderer** | [`system.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/engine/system.cpp), [`renderer.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/engine/renderer.cpp), [`chr_decoder.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/engine/chr_decoder.cpp), [`rng.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/engine/rng.cpp) | SDL2 window, 256x240/16:9 tile rasterizer, 2bpp CHR decoder, NES PRNG |
| **UI & Fonts** | [`window_box.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/ui/window_box.cpp), [`font.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/ui/font.cpp) | 8x8 NES CHR font rendering, authentic border box drawing |
| **Game State & Save** | [`save_system.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/state/save_system.cpp) | Party state, gold, inventory, SRAM file read/write |
| **Battle Engine** | [`battle_engine.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/core/battle_engine.cpp) | Initiative, physical/spell math, enemy AI, monster TSA rasterizer |
| **Map Engine** | [`map_engine.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/core/map_engine.cpp), [`map_loader.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/data/map_loader.cpp) | Overworld/dungeon map tiles, collision, NPCs, chests, teleports |
| **Menu Engine** | [`menu_engine.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/core/menu_engine.cpp) | Party status, equipment manager, magic lists, shops |
| **Intro & Events** | [`intro_engine.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/core/intro_engine.cpp), [`cutscene_engine.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/core/cutscene_engine.cpp), [`minigame_engine.cpp`](file:///d:/hoyaj/Coding/NES/ff1_cpp/src/core/minigame_engine.cpp) | Party creation, Conelia Bridge scene, 15-Puzzle engine |
