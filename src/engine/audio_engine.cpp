#include "audio_engine.hpp"
#include <iostream>

namespace ff1 {

AudioEngine::AudioEngine() {}

AudioEngine::~AudioEngine() {
    stop_music();
}

bool AudioEngine::init() {
    initialized_ = true;
    std::cout << "AudioEngine: Subsystem initialized (with soft-synth audio triggers)." << std::endl;
    return true;
}

void AudioEngine::play_music(MusicTrack track) {
    if (!initialized_) return;
    if (current_track_ == track) return;

    current_track_ = track;
    std::string track_name = "NONE";
    switch (track) {
        case MusicTrack::OVERWORLD: track_name = "Main Theme (Overworld)"; break;
        case MusicTrack::TOWN: track_name = "Town Theme"; break;
        case MusicTrack::CASTLE: track_name = "Conelia Castle Theme"; break;
        case MusicTrack::BATTLE: track_name = "Battle Theme"; break;
        case MusicTrack::FANFARE: track_name = "Victory Fanfare"; break;
        case MusicTrack::DUNGEON: track_name = "Dungeon Theme"; break;
        case MusicTrack::AIRSHIP: track_name = "Airship Theme"; break;
        case MusicTrack::GAME_OVER: track_name = "Game Over Theme"; break;
        default: break;
    }
    if (track != MusicTrack::NONE) {
        std::cout << "AudioEngine BGM: Now playing [" << track_name << "]" << std::endl;
    }
}

void AudioEngine::stop_music() {
    if (current_track_ != MusicTrack::NONE) {
        current_track_ = MusicTrack::NONE;
    }
}

void AudioEngine::play_sfx(SoundEffect sfx) {
    if (!initialized_) return;
    std::string sfx_name = "SFX";
    switch (sfx) {
        case SoundEffect::CURSOR_MOVE: sfx_name = "Cursor Move"; break;
        case SoundEffect::SELECT: sfx_name = "Select"; break;
        case SoundEffect::ATTACK_HIT: sfx_name = "Attack Hit"; break;
        case SoundEffect::MAGIC_CAST: sfx_name = "Magic Cast"; break;
        case SoundEffect::DOOR_OPEN: sfx_name = "Door Open"; break;
        case SoundEffect::CHEST_OPEN: sfx_name = "Chest Open"; break;
        case SoundEffect::WARP_TELEPORT: sfx_name = "Warp Teleport"; break;
    }
    (void)sfx_name;
}

} // namespace ff1
