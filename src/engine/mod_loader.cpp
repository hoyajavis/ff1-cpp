#include "mod_loader.hpp"
#include <filesystem>
#include <iostream>

namespace ff1 {

ModLoader::ModLoader(const std::string& mods_directory) : mods_dir_(mods_directory) {}

bool ModLoader::scan_mod_directory() {
    active_mods_count_ = 0;
    has_hd_textures_ = false;
    has_custom_audio_ = false;

    try {
        if (std::filesystem::exists(mods_dir_) && std::filesystem::is_directory(mods_dir_)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(mods_dir_)) {
                if (entry.is_regular_file()) {
                    auto ext = entry.path().extension().string();
                    if (ext == ".png") {
                        has_hd_textures_ = true;
                        active_mods_count_++;
                    } else if (ext == ".wav" || ext == ".ogg") {
                        has_custom_audio_ = true;
                        active_mods_count_++;
                    }
                }
            }
        }
    } catch (...) {
        std::cerr << "ModLoader: Error scanning mods directory " << mods_dir_ << std::endl;
    }

    std::cout << "ModLoader: Scanned " << active_mods_count_ << " active HD mod override files in '" << mods_dir_ << "'." << std::endl;
    return true;
}

std::string ModLoader::get_texture_override_path(const std::string& texture_name) const {
    std::string candidate = mods_dir_ + "/textures/" + texture_name + ".png";
    if (std::filesystem::exists(candidate)) {
        return candidate;
    }
    return "";
}

std::string ModLoader::get_audio_override_path(const std::string& track_name) const {
    std::string candidate_wav = mods_dir_ + "/audio/" + track_name + ".wav";
    if (std::filesystem::exists(candidate_wav)) {
        return candidate_wav;
    }
    std::string candidate_ogg = mods_dir_ + "/audio/" + track_name + ".ogg";
    if (std::filesystem::exists(candidate_ogg)) {
        return candidate_ogg;
    }
    return "";
}

} // namespace ff1
