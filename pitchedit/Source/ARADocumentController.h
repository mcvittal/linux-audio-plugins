#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include "NoteData.h"
#include "PitchDetector.h"
#include "NoteSegmenter.h"
#include "PhaseVocoder.h"

#include <atomic>
#include <mutex>
#include <memory>
#include <map>

// ─── Model objects ────────────────────────────────────────────────────────────

// ARAAudioSource subclass: holds per-source pitch analysis data.
class PEAudioSource final : public juce::ARAAudioSource
{
public:
    // Inherit ARA SDK constructor: AudioSource(DocumentController*, ARAAudioSourceHostRef)
    using juce::ARAAudioSource::ARAAudioSource;

    AudioAnalysisData analysisData;
    std::atomic<bool> needsAnalysis { true };
};

// ARAAudioModification subclass: holds user note edits.
class PEAudioModification final : public juce::ARAAudioModification
{
public:
    using juce::ARAAudioModification::ARAAudioModification;

    std::vector<NoteBlob> editedNotes;
    bool hasEdits { false };

    const std::vector<NoteBlob>& activeNotes() const;
};

// ARAPlaybackRegion subclass.
class PEPlaybackRegion final : public juce::ARAPlaybackRegion
{
public:
    using juce::ARAPlaybackRegion::ARAPlaybackRegion;
};

// ─── Playback renderer ────────────────────────────────────────────────────────

class PEPlaybackRenderer final : public juce::ARAPlaybackRenderer
{
public:
    using juce::ARAPlaybackRenderer::ARAPlaybackRenderer;

    void prepareToPlay (double sampleRate,
                        int    maximumSamplesPerBlock,
                        int    numChannels,
                        juce::AudioProcessor::ProcessingPrecision,
                        AlwaysNonRealtime) override;

    void releaseResources() override;

    bool processBlock (juce::AudioBuffer<float>&,
                       juce::AudioProcessor::Realtime,
                       const juce::AudioPlayHead::PositionInfo&) noexcept override;

private:
    double m_sampleRate   { 44100.0 };
    int    m_numChannels  { 2 };
    int    m_maxBlockSize { 512 };

    struct RegionState {
        std::unique_ptr<juce::ARAAudioSourceReader> reader;
        std::vector<std::unique_ptr<PhaseVocoder>>  vocoders;
    };
    std::map<juce::ARAPlaybackRegion*, RegionState> m_regionState;

    void rebuildRegionState(double sr, int maxBlockSize, int numCh);
};

// ─── Document controller ──────────────────────────────────────────────────────

class PEDocumentController final : public juce::ARADocumentControllerSpecialisation
{
public:
    using juce::ARADocumentControllerSpecialisation::ARADocumentControllerSpecialisation;

    void scheduleAnalysis(PEAudioSource* source);

    // ── Listener ─────────────────────────────────────────────────────────────
    struct Listener {
        virtual ~Listener() = default;
        virtual void analysisComplete(PEAudioSource*) = 0;
    };
    void addListener   (Listener* l);
    void removeListener(Listener* l);

protected:
    // ── Factory overrides (signatures match juce_ARADocumentController.h) ────
    juce::ARAAudioSource* doCreateAudioSource(
        juce::ARADocument* document,
        ARA::ARAAudioSourceHostRef hostRef) override;

    juce::ARAAudioModification* doCreateAudioModification(
        juce::ARAAudioSource* audioSource,
        ARA::ARAAudioModificationHostRef hostRef,
        const juce::ARAAudioModification* optionalModificationToClone) override;

    juce::ARAPlaybackRegion* doCreatePlaybackRegion(
        juce::ARAAudioModification* modification,
        ARA::ARAPlaybackRegionHostRef hostRef) override;

    juce::ARAPlaybackRenderer* doCreatePlaybackRenderer() noexcept override;
    juce::ARAEditorRenderer*   doCreateEditorRenderer()   noexcept override;
    juce::ARAEditorView*       doCreateEditorView()       noexcept override;

    // ── Persistence (both pure virtual in base) ───────────────────────────────
    bool doRestoreObjectsFromStream(juce::ARAInputStream&,
                                    const juce::ARARestoreObjectsFilter*) override;
    bool doStoreObjectsToStream    (juce::ARAOutputStream&,
                                    const juce::ARAStoreObjectsFilter*) override;

private:
    PitchDetector  m_detector  { 2048, 0.15f };
    NoteSegmenter  m_segmenter;
    juce::ThreadPool m_threadPool { 1 };

    std::vector<Listener*> m_listeners;
    std::mutex             m_listenerMutex;

    void runAnalysis    (PEAudioSource* source);
    void notifyListeners(PEAudioSource* source);
};
