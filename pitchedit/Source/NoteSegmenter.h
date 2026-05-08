#pragma once

#include "NoteData.h"
#include <vector>

// Converts a sequence of PitchFrames (from PitchDetector) into NoteBlobs
// by detecting onsets, tracking pitch stability, and grouping voiced frames.
class NoteSegmenter {
public:
    struct Config {
        float minNoteDuration       { 0.04f  };  // seconds
        float maxPitchJump          { 1.2f   };  // semitones before we start a new note
        float silenceThresholdRMS   { 0.008f };
        float confidenceThreshold   { 0.25f  };
        float minPitchHz            { 50.0f  };
        float maxPitchHz            { 2200.0f};
    };

    NoteSegmenter();
    explicit NoteSegmenter(Config cfg);

    void segment(const std::vector<PitchFrame>& frames,
                 double                          sampleRate,
                 std::vector<NoteBlob>&          outNotes);

private:
    Config m_cfg;
    int    m_nextId { 0 };

    bool  isVoiced(const PitchFrame& f) const;
    float medianPitch(const std::vector<PitchFrame>& frames) const;
};
