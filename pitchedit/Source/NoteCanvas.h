#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "NoteData.h"
#include <functional>
#include <optional>

// Piano-roll style component showing detected NoteBlobs.
// Supports drag-to-correct pitch and horizontal time nudge.
class NoteCanvas final : public juce::Component,
                          public juce::Timer
{
public:
    static constexpr int kPianoWidth   = 56;
    static constexpr int kHeaderHeight = 28;

    NoteCanvas();
    ~NoteCanvas() override;

    // Feed note data.  Pointer must stay valid while canvas is alive.
    void setNotes(const std::vector<NoteBlob>* notes);

    // Visible range
    void setTimeRange (double startSec, double endSec);
    void setPitchRange(float  loMidi,  float  hiMidi);

    // Callback fired when the user moves a note.
    // Arguments: noteId, new pitchCorrection semitones, new timeOffset seconds.
    using NoteEditCallback = std::function<void(int id, double pitchCorr, double timeOff)>;
    void setNoteEditCallback(NoteEditCallback cb);

    // Playhead position (seconds) for display
    void setPlayheadPosition(double seconds);

    // ── juce::Component ───────────────────────────────────────────────────────
    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDown      (const juce::MouseEvent&) override;
    void mouseDrag      (const juce::MouseEvent&) override;
    void mouseUp        (const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&,
                         const juce::MouseWheelDetails&) override;

    // ── juce::Timer ──────────────────────────────────────────────────────────
    void timerCallback() override;

private:
    const std::vector<NoteBlob>* m_notes     { nullptr };

    double m_viewStart  { 0.0  };
    double m_viewEnd    { 10.0 };
    float  m_pitchLo    { 36.0f };  // C2
    float  m_pitchHi    { 84.0f };  // C6

    double m_playhead   { -1.0 };

    NoteEditCallback m_editCb;

    // Drag state
    struct DragState {
        int    noteId          { -1 };
        double origPitchCorr   { 0.0 };
        double origTimeOff     { 0.0 };
        juce::Point<float> startPos;
    };
    std::optional<DragState> m_drag;

    // Layout helpers
    juce::Rectangle<float> contentArea() const;
    float timeToX (double t)  const;
    float pitchToY(float midi) const;
    double xToTime (float x)   const;
    float  yToPitch(float y)   const;

    // Note rectangle in component coordinates
    juce::Rectangle<float> noteRect(const NoteBlob& n) const;

    // Hit testing; returns nullptr if no note hit
    const NoteBlob* hitTest(juce::Point<float> pos) const;
    NoteBlob*       hitTestMutable(juce::Point<float> pos);

    // Drawing helpers
    void drawBackground    (juce::Graphics&, juce::Rectangle<float> area);
    void drawPianoKeys     (juce::Graphics&, juce::Rectangle<float> pianoArea);
    void drawTimeline      (juce::Graphics&, juce::Rectangle<float> headerArea);
    void drawNotes         (juce::Graphics&, juce::Rectangle<float> area);
    void drawPlayhead      (juce::Graphics&, juce::Rectangle<float> area);

    juce::Colour noteColour(const NoteBlob& n) const;
    juce::Colour pitchClassColour(int semitone) const;
    bool isBlackKey(int midiNote) const;
};
