#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>

struct PitchFrame {
    double timeSeconds { 0.0 };
    float  pitchHz     { 0.0f };  // 0 = unvoiced
    float  pitchMidi   { 0.0f };  // fractional MIDI note
    float  confidence  { 0.0f };  // 0–1
    float  rmsAmplitude{ 0.0f };
};

struct NoteBlob {
    int    id             { -1 };
    double startTime      { 0.0 };   // seconds in source audio
    double duration       { 0.0 };   // seconds
    double basePitchMidi  { 60.0 };  // detected median pitch
    double pitchCorrection{ 0.0 };   // semitones user applied
    double timeOffset     { 0.0 };   // seconds user applied
    float  amplitude      { 0.0f };  // 0–1

    std::vector<PitchFrame> frames;

    bool selected { false };
    bool muted    { false };

    double correctedPitchMidi() const { return basePitchMidi + pitchCorrection; }
    double editedStartTime()    const { return startTime + timeOffset; }
    double editedEndTime()      const { return editedStartTime() + duration; }
};

struct AudioAnalysisData {
    double               sampleRate       { 44100.0 };
    double               totalDuration    { 0.0 };
    std::vector<PitchFrame> allFrames;
    std::vector<NoteBlob>   notes;
    bool                 analysisComplete { false };
};
