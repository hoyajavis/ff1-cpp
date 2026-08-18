#include "cutscene_engine.hpp"

namespace ff1 {

CutsceneEngine::CutsceneEngine() {}

void CutsceneEngine::start_cutscene(CutsceneType type) {
    active_type_ = type;
    playing_ = true;
    frame_counter_ = 0;
    subtitles_.clear();

    if (type == CutsceneType::OPENING_BRIDGE) {
        subtitles_ = {
            "And so their journey begins...",
            "The 4 Light Warriors cross the Conelia Bridge.",
            "Restoring peace to the world."
        };
    } else if (type == CutsceneType::AIRSHIP_RISING) {
        subtitles_ = {
            "The Levestone reacts with the desert sands!",
            "The ancient Airship rises into the skies!",
            "Freedom to explore the world!"
        };
    } else if (type == CutsceneType::ENDING_CREDITS) {
        subtitles_ = {
            "Chaos has been vanquished in the past!",
            "The 2000 year time loop is broken.",
            "The 4 Light Warriors return to their own time.",
            "Sarah, the King, and Matoya are waiting.",
            "The four crystals shine with eternal hope.",
            "CREDITS: PROGRAMMED BY SQUARE & ANTIGRAVITY",
            "THE END"
        };
    }
}

void CutsceneEngine::update() {
    if (!playing_) return;
    frame_counter_++;
    int max_frames = (active_type_ == CutsceneType::ENDING_CREDITS) ? 700 : 300;
    if (frame_counter_ > max_frames) {
        end_cutscene();
    }
}

void CutsceneEngine::end_cutscene() {
    playing_ = false;
    active_type_ = CutsceneType::NONE;
    frame_counter_ = 0;
    subtitles_.clear();
}

std::string CutsceneEngine::get_current_subtitle() const {
    if (!playing_ || subtitles_.empty()) return "";
    size_t idx = (frame_counter_ / 100) % subtitles_.size();
    return subtitles_[idx];
}

} // namespace ff1
