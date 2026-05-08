#include "NoteSegmenter.h"
#include <cmath>
#include <algorithm>
#include <numeric>

NoteSegmenter::NoteSegmenter() : m_cfg({}) {}
NoteSegmenter::NoteSegmenter(Config cfg) : m_cfg(std::move(cfg)) {}

bool NoteSegmenter::isVoiced(const PitchFrame& f) const
{
    return f.confidence    >= m_cfg.confidenceThreshold &&
           f.pitchHz       >= m_cfg.minPitchHz          &&
           f.pitchHz       <= m_cfg.maxPitchHz           &&
           f.rmsAmplitude  >= m_cfg.silenceThresholdRMS;
}

float NoteSegmenter::medianPitch(const std::vector<PitchFrame>& frames) const
{
    std::vector<float> pitches;
    for (const auto& f : frames)
        if (f.pitchMidi > 0.0f) pitches.push_back(f.pitchMidi);

    if (pitches.empty()) return 0.0f;
    std::sort(pitches.begin(), pitches.end());
    return pitches[pitches.size() / 2];
}

void NoteSegmenter::segment(const std::vector<PitchFrame>& frames,
                              double                          sampleRate,
                              std::vector<NoteBlob>&          outNotes)
{
    outNotes.clear();
    if (frames.empty()) return;

    // Accumulate voiced frames into runs; split when pitch jumps > maxPitchJump
    struct Run {
        std::vector<PitchFrame> frames;
    };

    std::vector<Run> runs;
    Run current;

    for (const auto& f : frames) {
        if (!isVoiced(f)) {
            if (!current.frames.empty()) {
                runs.push_back(std::move(current));
                current = {};
            }
            continue;
        }

        if (!current.frames.empty()) {
            float prevMidi = current.frames.back().pitchMidi;
            if (std::abs(f.pitchMidi - prevMidi) > m_cfg.maxPitchJump) {
                runs.push_back(std::move(current));
                current = {};
            }
        }
        current.frames.push_back(f);
    }
    if (!current.frames.empty())
        runs.push_back(std::move(current));

    // Convert runs to NoteBlobs
    for (auto& run : runs) {
        if (run.frames.empty()) continue;

        double startTime = run.frames.front().timeSeconds;
        double endTime   = run.frames.back().timeSeconds;
        double duration  = endTime - startTime;

        if (duration < m_cfg.minNoteDuration) continue;

        float med = medianPitch(run.frames);
        if (med <= 0.0f) continue;

        float maxAmp = 0.0f;
        for (const auto& f : run.frames)
            maxAmp = std::max(maxAmp, f.rmsAmplitude);

        NoteBlob blob;
        blob.id            = m_nextId++;
        blob.startTime     = startTime;
        blob.duration      = duration;
        blob.basePitchMidi = (double)med;
        blob.amplitude     = maxAmp;
        blob.frames        = std::move(run.frames);

        outNotes.push_back(std::move(blob));
    }
}
