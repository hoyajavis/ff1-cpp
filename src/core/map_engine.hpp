#ifndef MAP_ENGINE_HPP
#define MAP_ENGINE_HPP

#include "data/game_types.hpp"
#include "data/data_loader.hpp"
#include "data/map_loader.hpp"
#include "engine/rng.hpp"
#include "state/save_system.hpp"
#include <vector>
#include <string>
#include <cstdint>

namespace ff1 {

enum class MapType {
    OVERWORLD,
    STANDARD_MAP
};

enum class Direction : uint8_t {
    RIGHT = 1,
    LEFT = 2,
    DOWN = 4,
    UP = 8
};

struct MapNPC {
    uint8_t id = 0;
    int x = 0;
    int y = 0;
    Direction facing = Direction::DOWN;
    uint8_t graphic_id = 0;
    std::string dialogue;
    bool active = true;
};

struct MapChest {
    int x = 0;
    int y = 0;
    uint8_t chest_id = 0;
    uint8_t item_or_gp = 0;
    uint16_t value = 0;
    bool opened = false;
};

class MapEngine {
public:
    MapEngine(const DataLoader& loader, const MapLoader& map_loader, RNG& rng);

    void load_map(uint8_t map_id, MapType type);

    bool can_move_to(int x, int y, VehicleType vehicle) const;
    bool move_player(Direction dir, GameSaveData& save_data, std::string& out_message);

    bool check_interaction(GameSaveData& save_data, std::string& out_message);
    bool check_door_unlock(int target_x, int target_y, GameSaveData& save_data, std::string& out_message);
    bool check_event_trigger(GameSaveData& save_data, std::string& out_message);

    bool check_encounter(VehicleType vehicle);

    uint8_t get_current_map_id() const { return current_map_id_; }
    MapType get_map_type() const { return map_type_; }
    Direction get_player_facing() const { return player_facing_; }

    const std::vector<MapNPC>& get_npcs() const { return npcs_; }
    const std::vector<MapChest>& get_chests() const { return chests_; }

    uint8_t get_tile_at(int x, int y) const;
    int get_width() const { return width_; }
    int get_height() const { return height_; }
    std::string get_map_name() const { return current_map_name_; }

private:
    const DataLoader& loader_;
    const MapLoader& map_loader_;
    RNG& rng_;

    uint8_t current_map_id_ = 0;
    std::string current_map_name_ = "Overworld";
    MapType map_type_ = MapType::OVERWORLD;
    Direction player_facing_ = Direction::DOWN;

    int width_ = 256;
    int height_ = 256;

    std::vector<uint8_t> tile_data_;
    std::vector<MapNPC> npcs_;
    std::vector<MapChest> chests_;
    std::vector<TeleportEntry> teleports_;

    void setup_map_npcs(uint8_t map_id);
};

} // namespace ff1

#endif // MAP_ENGINE_HPP
