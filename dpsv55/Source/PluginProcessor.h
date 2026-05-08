#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>

class DpsV55Processor : public juce::AudioProcessor
{
public:
    DpsV55Processor();
    ~DpsV55Processor() override;

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // ── Shared state ──────────────────────────────────────────────────────────
    std::atomic<int>  midiChannel   { 1   };  // 1–16
    std::atomic<int>  programNumber { 0   };  // 0–127 (MIDI PC value)
    std::atomic<int>  masterVolume  { 100 };  // 0–127 (CC #7)

    // Effect type — SysEx block 2 params 1/3
    std::atomic<int>  fxAType    { 1  };   // 1–45
    std::atomic<int>  fxBType    { 10 };   // 10–45

    // Effect on/off — SysEx block 2 params 2/4  (0=off, 1=on)
    std::atomic<int>  fxAOnOff   { 1 };
    std::atomic<int>  fxBOnOff   { 1 };

    // Structure — SysEx block 2 param 0  (0=parallel, 1=serial)
    std::atomic<int>  fxStructure { 0 };

    // ── Simple MIDI pending flags ─────────────────────────────────────────────
    std::atomic<bool> pendingProgramChange { false };
    std::atomic<bool> pendingVolumeChange  { false };
    std::atomic<bool> pendingAllNotesOff   { false };

    // ── SysEx parameter queue (message thread → audio thread) ─────────────────
    // SysEx format: F0 4C 0n 24 20 [block] [param] [v>>12] [v>>8] [v>>4] [v&F] F7
    // n = (midiChannel-1) & 0x0F
    // Handles all real-time parameter changes, effect type/on-off, structure.
    void queueParamChange(int block, int param, int value);

private:
    juce::CriticalSection          sysexLock;
    std::vector<juce::MidiMessage> sysexQueue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DpsV55Processor)
};
