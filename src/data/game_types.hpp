#ifndef GAME_TYPES_HPP
#define GAME_TYPES_HPP

#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace ff1 {

// Character Classes
enum class ClassType : uint8_t {
    WARRIOR = 0,
    THIEF = 1,
    BLACK_BELT = 2,
    RED_MAGE = 3,
    WHITE_MAGE = 4,
    BLACK_MAGE = 5,
    KNIGHT = 6,
    NINJA = 7,
    MASTER = 8,
    RED_WIZARD = 9,
    WHITE_WIZARD = 10,
    BLACK_WIZARD = 11,
    NONE = 255
};

// Target Categories
namespace Category {
    constexpr uint8_t UNKNOWN  = 0x01;
    constexpr uint8_t DRAGON   = 0x02;
    constexpr uint8_t GIANT    = 0x04;
    constexpr uint8_t UNDEAD   = 0x08;
    constexpr uint8_t WERE     = 0x10;
    constexpr uint8_t WATER    = 0x20;
    constexpr uint8_t MAGE     = 0x40;
    constexpr uint8_t REGEN    = 0x80;
}

// Elements
namespace Element {
    constexpr uint8_t STATUS  = 0x01;
    constexpr uint8_t POISON  = 0x02;
    constexpr uint8_t TIME    = 0x04;
    constexpr uint8_t DEATH   = 0x08;
    constexpr uint8_t FIRE    = 0x10;
    constexpr uint8_t ICE     = 0x20;
    constexpr uint8_t LIGHTNING = 0x40;
    constexpr uint8_t EARTH   = 0x80;
}

// Status Ailments (matching Constants.inc AIL_*)
namespace Status {
    constexpr uint8_t NONE      = 0x00;
    constexpr uint8_t DEATH     = 0x01; // AIL_DEAD
    constexpr uint8_t STONE     = 0x02; // AIL_STONE
    constexpr uint8_t POISON    = 0x04; // AIL_POISON
    constexpr uint8_t BLIND     = 0x08; // AIL_DARK
    constexpr uint8_t PARALYSIS = 0x10; // AIL_STUN
    constexpr uint8_t SLEEP     = 0x20; // AIL_SLEEP
    constexpr uint8_t SILENCE   = 0x40; // AIL_MUTE
    constexpr uint8_t CONFUSE   = 0x80; // AIL_CONF
}

// Vehicles
enum class VehicleType : uint8_t {
    WALK = 1,
    CANOE = 2,
    SHIP = 4,
    AIRSHIP = 8
};

// Shop Types
enum class ShopType : uint8_t {
    WEAPON = 1,
    ARMOR = 2,
    WHITE_MAGIC = 3,
    BLACK_MAGIC = 4,
    ITEM = 5,
    CLINIC = 6,
    INN = 7
};

// Shop Inventory (from 0E_8300_shopdata.bin)
struct ShopInventory {
    uint8_t shop_id = 0;
    ShopType type = ShopType::WEAPON;
    std::array<uint8_t, 4> items = {0xFF, 0xFF, 0xFF, 0xFF};
    std::array<uint16_t, 4> prices = {0, 0, 0, 0};
};

// Enemy AI Script Data (16 bytes per AI ID)
struct EnemyAIData {
    uint8_t spell_chance = 0;
    uint8_t skill_chance = 0;
    std::array<uint8_t, 8> spell_list = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    std::array<uint8_t, 4> skill_list = {0xFF, 0xFF, 0xFF, 0xFF};
};

// Weapon Data
struct WeaponData {
    uint8_t hit_rate = 0;
    uint8_t damage = 0;
    uint8_t crit_rate = 0;
    uint8_t spell_cast = 0;
    uint8_t element = 0;
    uint8_t category = 0;
    uint8_t graphic = 0;
    uint8_t palette = 0;
};

// Armor Data
struct ArmorData {
    uint8_t evade_penalty = 0;
    uint8_t absorb = 0;
    uint8_t element_def = 0;
    uint8_t spell_cast = 0;
};

// Magic Data
struct MagicData {
    uint8_t hit_rate = 0;
    uint8_t effectivity = 0;
    uint8_t element = 0;
    uint8_t target = 0;
    uint8_t effect = 0;
    uint8_t graphic = 0;
    uint8_t palette = 0;
    uint8_t unused = 0;
    uint8_t battle_message_id = 0;
};

// Enemy Data
struct EnemyData {
    uint16_t exp = 0;
    uint16_t gp = 0;
    uint16_t hp_max = 0;
    uint8_t morale = 0;
    uint8_t ai_id = 0;
    uint8_t evade = 0;
    uint8_t absorb = 0;
    uint8_t num_hits = 0;
    uint8_t hit_rate = 0;
    uint8_t damage = 0;
    uint8_t crit_rate = 0;
    uint8_t attack_ailment = 0;
    uint8_t category = 0;
    uint8_t mag_def = 0;
    uint8_t elem_weak = 0;
    uint8_t elem_resist = 0;
    std::string name;
};

// Battle Formation Data
struct BattleFormation {
    uint8_t battle_type = 0;
    uint8_t pattern_sel = 0;
    uint8_t pic_assign = 0;
    std::array<uint8_t, 4> enemy_ids = {0, 0, 0, 0};
    std::array<uint8_t, 4> min_max_a = {0, 0, 0, 0};
    std::array<uint8_t, 2> palette_id = {0, 0};
    uint8_t surprised_rate = 0;
    uint8_t palette_assign = 0;
    bool no_run = false;
    std::array<uint8_t, 2> min_max_b = {0, 0};
};

// Character Stats
struct CharacterStats {
    uint16_t hp = 0;
    uint16_t max_hp = 0;
    std::array<uint8_t, 8> mp = {0, 0, 0, 0, 0, 0, 0, 0};
    std::array<uint8_t, 8> max_mp = {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t strength = 0;
    uint8_t agility = 0;
    uint8_t intelligence = 0;
    uint8_t vitality = 0;
    uint8_t luck = 0;
    uint8_t hit_rate = 0;
    uint8_t evade = 0;
    uint8_t absorb = 0;
    uint8_t damage = 0;
    uint8_t mag_def = 0;
    uint8_t crit_rate = 0;
};

// Character State
struct PartyCharacter {
    std::string name;
    ClassType char_class = ClassType::NONE;
    uint8_t level = 1;
    uint32_t exp = 0;
    CharacterStats stats;
    std::array<uint8_t, 4> weapons = {0xFF, 0xFF, 0xFF, 0xFF};
    std::array<uint8_t, 4> armors = {0xFF, 0xFF, 0xFF, 0xFF};
    std::array<std::array<uint8_t, 3>, 8> spells;
    uint8_t status_ailments = 0;
};

// Consumable Items Inventory
struct Consumables {
    uint8_t heal_potions = 0; // Cure potion (+30 HP)
    uint8_t pure_potions = 0; // Antidote (Cures Poison)
    uint8_t soft_potions = 0; // Gold Needle (Cures Stone)
    uint8_t tents = 0;        // Tent (+30 HP + Save on Overworld)
    uint8_t cabins = 0;       // Cabin (+60 HP + Save on Overworld)
    uint8_t houses = 0;       // House (+120 HP, MP Restore + Save on Overworld)
};

// Key Quest Items
enum class KeyItem : uint8_t {
    LUTE = 0,
    CROWN = 1,
    CRYSTAL = 2,
    HERB = 3,
    MYSTIC_KEY = 4,
    TNT = 5,
    ADAMANT = 6,
    SLAB = 7,
    RUBY = 8,
    ROD = 9,
    FLOATER = 10,
    CHIME = 11,
    TAIL = 12,
    CUBE = 13,
    BOTTLE = 14,
    OXYALE = 15,
    CANOE = 16,
    CANAL = 17,
    COUNT = 18
};

// Major Quest Event Flags
namespace QuestFlag {
    constexpr size_t SARAH_RESCUED         = 0x20; // Defeated Garland, rescued Sarah
    constexpr size_t BRIDGE_BUILT          = 0x21; // King of Conelia built Northern Bridge
    constexpr size_t PIRATES_DEFEATED      = 0x22; // Bikke's pirates defeated -> Ship granted
    constexpr size_t CROWN_RETRIEVED       = 0x23; // Crown looted from Marsh Cave B3
    constexpr size_t ASTOS_DEFEATED        = 0x24; // Astos defeated -> Crystal Eye obtained
    constexpr size_t MATOYA_HERB_TRADED    = 0x25; // Crystal Eye traded for Jolt Tonic / Herb
    constexpr size_t ELF_PRINCE_AWAKE      = 0x26; // Elf Prince awakened -> Mystic Key granted
    constexpr size_t CANAL_DEMOLISHED      = 0x27; // Nerrick used TNT to open canal
    constexpr size_t TITAN_RUBY_FED        = 0x28; // Star Ruby fed to Titan -> tunnel opened
    constexpr size_t SARDA_ROD_OBTAINED    = 0x29; // Sarda gave Earth Rod
    constexpr size_t EARTH_PLATE_SHATTERED = 0x2A; // Earth Cave B3 slab shattered
    constexpr size_t VAMPIRE_DEFEATED      = 0x2B; // Earth Cave Vampire defeated
    constexpr size_t LICH_DEFEATED         = 0x2C; // Lich defeated
    constexpr size_t EARTH_ORB_LIT         = 0x2D; // Earth crystal altar activated
    constexpr size_t CARAVAN_BOTTLE_BOUGHT = 0x2E; // Fairy bottle bought from Caravan
    constexpr size_t FAIRY_RELEASED        = 0x2F; // Fairy released in Gaia -> Oxyale granted
    constexpr size_t AIRSHIP_RAISED        = 0x30; // Floater used to raise Airship in Desert
    constexpr size_t EXCALIBUR_FORGED      = 0x31; // Smyth forged Excalibur from Adamant
    constexpr size_t CLASS_PROMOTED        = 0x32; // Bahamut class promotion granted
    constexpr size_t SLAB_TRANSLATED       = 0x33; // Dr. Unne translated the ancient SLAB
    constexpr size_t CHIME_OBTAINED        = 0x34; // Lufenia gave Chime to unseal Mirage Tower
    constexpr size_t CUBE_OBTAINED         = 0x35; // Lufenia/Waterfall gave Warp Cube for Flying Fortress
    constexpr size_t EVIL_EYE_DEFEATED     = 0x36; // Ice Cave Evil Eye defeated -> Floater obtained
    constexpr size_t KARY_DEFEATED         = 0x37; // Mt. Gurgu Fire Fiend Kary defeated
    constexpr size_t FIRE_ORB_LIT          = 0x38; // Fire crystal altar activated
    constexpr size_t KRAKEN_DEFEATED       = 0x39; // Sunken Shrine Water Fiend Kraken defeated
    constexpr size_t WATER_ORB_LIT         = 0x3A; // Water crystal altar activated
    constexpr size_t TIAMAT_DEFEATED       = 0x3B; // Flying Fortress Wind Fiend Tiamat defeated
    constexpr size_t WIND_ORB_LIT          = 0x3C; // Wind crystal altar activated
    constexpr size_t FOUR_ORBS_LIT         = 0x3D; // All 4 Orbs restored, Black Crystal unsealed
    constexpr size_t TIME_WARP_UNSEALED    = 0x3E; // Lute played at Black Crystal -> ToF Past opened
    constexpr size_t LICH2_DEFEATED        = 0x3F; // ToF Past 1F Lich 2 / Phantom defeated
    constexpr size_t KARY2_DEFEATED        = 0x40; // ToF Past 2F Kary 2 defeated
    constexpr size_t KRAKEN2_DEFEATED      = 0x41; // ToF Past 3F Kraken 2 defeated
    constexpr size_t TIAMAT2_DEFEATED      = 0x42; // ToF Past 4F Tiamat 2 defeated
    constexpr size_t MASAMUNE_OBTAINED     = 0x43; // Legendary Masamune sword looted
    constexpr size_t CHAOS_DEFEATED        = 0x44; // Ultimate Final Boss Chaos defeated
    constexpr size_t GAME_COMPLETED        = 0x45; // Epilogue ending complete
}

struct KeyItemInfo {
    std::string name;
    std::string description;
};

inline KeyItemInfo get_key_item_info(KeyItem item) {
    switch (item) {
        case KeyItem::LUTE: return {"LUTE", "The legendary musical instrument of Sarah."};
        case KeyItem::CROWN: return {"CROWN", "The stolen royal CROWN of Northwest Castle."};
        case KeyItem::CRYSTAL: return {"CRYSTAL", "The eye crystal stolen from Witch Matoya."};
        case KeyItem::HERB: return {"HERB", "Herb that can awaken the sleeping Elf Prince."};
        case KeyItem::MYSTIC_KEY: return {"MYSTIC KEY", "Key forged to unlock locked treasure vaults."};
        case KeyItem::TNT: return {"TNT", "High explosive capable of blasting open the canal."};
        case KeyItem::ADAMANT: return {"ADAMANT", "Legendary metal needed by Smyth to forge Excalibur."};
        case KeyItem::SLAB: return {"SLAB", "Ancient tablet inscribed with Lufenian language."};
        case KeyItem::RUBY: return {"STAR RUBY", "Giant ruby coveted by the hungry Titan."};
        case KeyItem::ROD: return {"EARTH ROD", "Rod blessed by Sarda to remove the earth plate."};
        case KeyItem::FLOATER: return {"FLOATER", "Levistone stone that can raise the ancient Airship."};
        case KeyItem::CHIME: return {"CHIME", "Chime required to enter the Mirage Tower."};
        case KeyItem::TAIL: return {"RAT TAIL", "Proof of courage for King Bahamut's trial."};
        case KeyItem::CUBE: return {"WARP CUBE", "Cube needed to access the Floating Castle."};
        case KeyItem::BOTTLE: return {"BOTTLE", "Bottle containing a trapped fairy from Gaia."};
        case KeyItem::OXYALE: return {"OXYALE", "Mystic liquid that allows breathing underwater."};
        case KeyItem::CANOE: return {"CANOE", "Light boat for traveling along rivers and streams."};
        case KeyItem::CANAL: return {"CANAL", "Canal path connected to the outer sea."};
        default: return {"ITEM", "Key quest artifact."};
    }
}

// Check authentic NES class spell permissions (0..63)
inline bool can_class_learn_spell(ClassType ctype, uint8_t spell_id) {
    uint8_t tier = (spell_id / 8) + 1; // 1..8
    bool is_white = ((spell_id % 8) < 4);

    switch (ctype) {
        case ClassType::WARRIOR:
            return false;
        case ClassType::KNIGHT:
            return is_white && (tier <= 3) && (spell_id != 1 && spell_id != 17); // Level 1-3 White (excluding HARM series)
        case ClassType::THIEF:
            return false;
        case ClassType::NINJA:
            return !is_white && (tier <= 4); // Level 1-4 Black
        case ClassType::BLACK_BELT:
        case ClassType::MASTER:
            return false;
        case ClassType::RED_MAGE:
            if (tier > 4) return false;
            // Level 1-4 White/Black except special spells
            return (spell_id != 1 && spell_id != 17 && spell_id != 25);
        case ClassType::RED_WIZARD:
            if (tier > 7) return false;
            // Up to Level 7 selected spells
            if (tier <= 4) return (spell_id != 1 && spell_id != 17 && spell_id != 25);
            if (tier == 5) return (spell_id == 32 || spell_id == 33 || spell_id == 36 || spell_id == 38 || spell_id == 39); // CUR3, LIFE, FIR3, WARP, SLO2
            if (tier == 6) return (spell_id == 42 || spell_id == 43 || spell_id == 44); // FOG2, INV2, LIT3
            if (tier == 7) return (spell_id == 50 || spell_id == 52); // ARUB, ICE3
            return false;
        case ClassType::WHITE_MAGE:
            return is_white && (tier <= 7);
        case ClassType::WHITE_WIZARD:
            return is_white; // All White magic tiers 1-8
        case ClassType::BLACK_MAGE:
            return !is_white && (tier <= 7);
        case ClassType::BLACK_WIZARD:
            return !is_white; // All Black magic tiers 1-8
        default:
            return false;
    }
}

// Elemental Orbs / Crystals
enum class OrbType : uint8_t {
    EARTH = 0,
    FIRE = 1,
    WATER = 2,
    WIND = 3
};

// Input Keys for Menu & UI navigation
enum class InputKey {
    NONE,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    CONFIRM,
    CANCEL,
    START,
    SELECT
};

} // namespace ff1

#endif // GAME_TYPES_HPP
