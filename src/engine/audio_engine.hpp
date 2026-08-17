#ifndef AUDIO_ENGINE_HPP
#define AUDIO_ENGINE_HPP

#include <string>
#include <vector>
#include <cstdint>

namespace ff1 {

enum class MusicTrack {
    NONE,
    PRELUDE,
    OVERWORLD,
    TOWN,
    CASTLE,
    BATTLE,
    FANFARE,
    DUNGEON,
    AIRSHIP,
    GAME_OVER
};

enum class SoundEffect {
    CURSOR_MOVE,
    SELECT,
    ATTACK_HIT,
    MAGIC_CAST,
    DOOR_OPEN,
    CHEST_OPEN,
    WARP_TELEPORT
};

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool init();
    void play_music(MusicTrack track);
    void stop_music();
    void play_sfx(SoundEffect sfx);

    MusicTrack get_current_track() const { return current_track_; }

private:
    bool initialized_ = false;
    MusicTrack current_track_ = MusicTrack::NONE;
};

} // namespace ff1

#endif // AUDIO_ENGINE_HPP
