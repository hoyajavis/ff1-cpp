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

// Status Ailments
namespace Status {
    constexpr uint8_t NONE     = 0x00;
    constexpr uint8_t DEATH    = 0x01;
    constexpr uint8_t STONE    = 0x02;
    constexpr uint8_t PARALYSIS = 0x04;
    constexpr uint8_t POISON   = 0x08;
    constexpr uint8_t BLIND    = 0x10;
    constexpr uint8_t SILENCE  = 0x20;
    constexpr uint8_t SLEEP    = 0x40;
    constexpr uint8_t CONFUSE  = 0x80;
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
