#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "DpsV55Effects.h"

class DpsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DpsLookAndFeel();
};

// ─────────────────────────────────────────────────────────────────────────────
// EffectPanel — interactive parameter control for one effect slot (FxA or FxB).
// Each parameter row is a slider (numeric range) or combo box (enum options).
// Changes are sent to the hardware via SysEx through the callback.
// ─────────────────────────────────────────────────────────────────────────────
class EffectPanel : public juce::Component
{
public:
    // block: 0 = FxA parameters, 1 = FxB parameters
    // fn: (block, paramIndex 0-7, value) — called whenever a param changes
    using SendParamFn = std::function<void(int block, int param, int value)>;

    EffectPanel();
    void init(int blockNumber, SendParamFn fn);
    void setEffect(int effectNumber);  // 1–45

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    static constexpr int kHdrH = 42;
    static constexpr int kRowH = 24;
    static constexpr int kMaxP = 8;

    int         fxNum   = 1;
    int         fxBlock = 0;
    int         numParams = 0;
    SendParamFn sendFn;

    juce::Label    nameLabels[kMaxP];
    juce::Slider   sliders[kMaxP];
    juce::ComboBox combos[kMaxP];
    bool           rowIsCombo[kMaxP] = {};
};

// ─────────────────────────────────────────────────────────────────────────────
class DpsV55Editor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit DpsV55Editor(DpsV55Processor&);
    ~DpsV55Editor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void triggerProgramChange();
    void triggerVolumeChange();
    void syncFromProcessor();

    DpsV55Processor& proc;
    DpsLookAndFeel   laf;

    // ── MIDI Channel ──────────────────────────────────────────────────────────
    juce::Label    channelLabel;
    juce::ComboBox channelBox;

    // ── Program Select ────────────────────────────────────────────────────────
    juce::Label        progSectionLabel;
    juce::Label        lcdDisplay;
    juce::TextButton   prevBtn  { "<" };
    juce::TextButton   nextBtn  { ">" };
    juce::Slider       progSlider;
    juce::TextButton   sendPCBtn  { "SEND PROGRAM CHANGE" };
    juce::ToggleButton autoSendBtn { "Auto-Send" };

    // ── Master Volume ─────────────────────────────────────────────────────────
    juce::Label      volSectionLabel;
    juce::Slider     volSlider;
    juce::Label      volValueLabel;
    juce::TextButton sendVolBtn { "SEND" };

    // ── Effect Control (SysEx) ────────────────────────────────────────────────
    juce::Label      fxSectionLabel;
    juce::Label      structureLabel;
    juce::ComboBox   structureCombo;       // parallel / serial
    juce::Label      fxaLabel, fxbLabel;
    juce::ComboBox   fxaCombo, fxbCombo;  // effect type selector
    juce::TextButton fxaOnOffBtn { "ON" };
    juce::TextButton fxbOnOffBtn { "ON" };
    EffectPanel      fxaPanel, fxbPanel;

    // ── Utility ───────────────────────────────────────────────────────────────
    juce::TextButton allNotesOffBtn { "ALL NOTES OFF" };
    juce::Label      statusLabel;

    int autoSendCountdown { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DpsV55Editor)
};
