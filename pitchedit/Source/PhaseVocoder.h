#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

// Phase vocoder pitch shifter.
// Uses JUCE's real-only FFT. Processes mono audio in a FIFO-based streaming
// fashion so it can be driven from any block size.
class PhaseVocoder {
public:
    // fftOrder: FFT size = 2^fftOrder (11 → 2048, 12 → 4096)
    // hopDivisor: overlap = fftSize / hopDivisor (4 → 75% overlap)
    PhaseVocoder(int fftOrder = 11, int hopDivisor = 4);
    ~PhaseVocoder() = default;

    void prepare(double sampleRate);
    void reset();

    void setPitchShift(float semitones);  // positive = higher
    void setTimeStretch(float ratio);     // 1.0 = no stretch

    // Stream samples through the vocoder.
    // Returns number of output samples written (may be less than numIn due to latency).
    int process(const float* input, int numIn, float* output, int maxOut);

    int getLatencySamples() const { return m_fftSize; }

private:
    juce::dsp::FFT m_fft;
    int    m_fftSize    { 2048 };
    int    m_hopSize    { 512  };
    float  m_pitchRatio { 1.0f };
    float  m_timeStretch{ 1.0f };
    double m_sampleRate { 44100.0 };

    std::vector<float> m_window;
    // fftBuf holds 2*fftSize floats (interleaved real/imag for JUCE's real FFT)
    std::vector<float> m_fftBuf;

    // Per-bin analysis state
    std::vector<float> m_lastAnalysisPhase;
    std::vector<float> m_synthPhaseAccum;
    std::vector<float> m_analysisMag;
    std::vector<float> m_analysisFreq;   // true frequency in bins/hop

    // Overlap-add output accumulator
    std::vector<float> m_olaOut;

    // Input/output FIFOs
    std::vector<float> m_inFifo;
    std::vector<float> m_outFifo;
    int m_inFill  { 0 };
    int m_outFill { 0 };

    void buildWindow();
    void processFrame();
};
