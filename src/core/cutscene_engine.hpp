#ifndef CUTSCENE_ENGINE_HPP
#define CUTSCENE_ENGINE_HPP

#include <string>
#include <vector>

namespace ff1 {

enum class CutsceneType {
    NONE,
    OPENING_BRIDGE,
    AIRSHIP_RISING,
    ENDING_CREDITS
};

class CutsceneEngine {
public:
    CutsceneEngine();

    void start_cutscene(CutsceneType type);
    void update();
    void end_cutscene();

    bool is_playing() const { return playing_; }
    CutsceneType get_active_type() const { return active_type_; }

    std::string get_current_subtitle() const;
    int get_progress_frame() const { return frame_counter_; }

private:
    bool playing_ = false;
    CutsceneType active_type_ = CutsceneType::NONE;
    int frame_counter_ = 0;
    std::vector<std::string> subtitles_;
};

} // namespace ff1

#endif // CUTSCENE_ENGINE_HPP
