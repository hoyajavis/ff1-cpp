#ifndef MAP_LOADER_HPP
#define MAP_LOADER_HPP

#include "map_types.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace ff1 {

class DataLoader;

class MapLoader {
public:
    MapLoader(const std::string& base_path = "");

    bool load_all_maps(const DataLoader* loader = nullptr);

    const StandardMapData& get_standard_map(uint8_t map_id) const;
    const std::vector<NPCObjectData>& get_npcs_for_map(uint8_t map_id) const;
    const std::vector<TeleportEntry>& get_teleports() const { return teleports_; }

    size_t get_total_maps_loaded() const { return maps_.size(); }
    size_t get_total_npcs_loaded() const { return all_npcs_.size(); }

private:
    std::string base_path_;

    std::unordered_map<uint8_t, StandardMapData> maps_;
    std::unordered_map<uint8_t, std::vector<NPCObjectData>> map_npcs_;
    std::vector<NPCObjectData> all_npcs_;
    std::vector<TeleportEntry> teleports_;

    bool load_npc_objects();
    bool load_teleport_matrix(const DataLoader* loader = nullptr);
    bool build_standard_maps(const DataLoader* loader = nullptr);

    std::vector<uint8_t> read_binary_file(const std::string& filename);
};

} // namespace ff1

#endif // MAP_LOADER_HPP
