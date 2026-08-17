#ifndef MOD_LOADER_HPP
#define MOD_LOADER_HPP

#include <string>
#include <vector>

namespace ff1 {

class ModLoader {
public:
    ModLoader(const std::string& mods_directory = "./mods");

    bool scan_mod_directory();

    bool has_hd_textures() const { return has_hd_textures_; }
    bool has_custom_audio() const { return has_custom_audio_; }

    std::string get_texture_override_path(const std::string& texture_name) const;
    std::string get_audio_override_path(const std::string& track_name) const;

    size_t get_active_mods_count() const { return active_mods_count_; }

private:
    std::string mods_dir_;
    bool has_hd_textures_ = false;
    bool has_custom_audio_ = false;
    size_t active_mods_count_ = 0;
};

} // namespace ff1

#endif // MOD_LOADER_HPP
