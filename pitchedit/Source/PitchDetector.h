#pragma once

#include "NoteData.h"
#include <vector>

// YIN pitch detector (de Cheveigné & Kawahara 2002).
// Analyzes a buffer of mono PCM and produces a PitchFrame per hop.
class PitchDetector {
public:
    explicit PitchDetector(int frameSize = 2048, float yinThreshold = 0.15f);

    // Single-frame detection; returns Hz or 0 if unvoiced.
    float detectPitch(const float* samples, int numSamples, float sampleRate) const;

    // Whole-buffer analysis, one PitchFrame per hopSize samples.
    void analyzeBuffer(const float* samples,
                       int          numSamples,
                       float        sampleRate,
                       std::vector<PitchFrame>& outFrames,
                       int hopSize = 512) const;

private:
    int   m_frameSize;
    float m_threshold;

    void  computeDifference (const float* buf, int n, std::vector<float>& df) const;
    void  computeCMNDF      (std::vector<float>& df) const;
    int   findPeriod        (const std::vector<float>& cmndf) const;
    float parabolicRefine   (const std::vector<float>& cmndf, int tau) const;
    float rms               (const float* buf, int n) const;
};
