#include "PhaseVocoder.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PhaseVocoder::PhaseVocoder(int fftOrder, int hopDivisor)
    : m_fft(fftOrder)
    , m_fftSize(1 << fftOrder)
    , m_hopSize((1 << fftOrder) / hopDivisor)
{
    const int bins = m_fftSize / 2 + 1;

    m_window           .resize(m_fftSize, 0.0f);
    m_fftBuf           .resize(m_fftSize * 2, 0.0f); // 2x for JUCE real FFT
    m_lastAnalysisPhase.resize(bins, 0.0f);
    m_synthPhaseAccum  .resize(bins, 0.0f);
    m_analysisMag      .resize(bins, 0.0f);
    m_analysisFreq     .resize(bins, 0.0f);
    m_olaOut           .resize(m_fftSize * 2, 0.0f);
    m_inFifo           .resize(m_fftSize * 4, 0.0f);
    m_outFifo          .resize(m_fftSize * 4, 0.0f);

    buildWindow();
}

void PhaseVocoder::buildWindow()
{
    // Hann window, normalised so OLA gives unity gain
    float norm = 0.0f;
    for (int i = 0; i < m_fftSize; ++i) {
        m_window[i] = 0.5f * (1.0f - std::cos(2.0f * (float)M_PI * i / m_fftSize));
        norm += m_window[i];
    }
    // Scale for overlap-add reconstruction
    float scale = (float)m_hopSize / norm;
    for (auto& w : m_window) w *= scale;
}

void PhaseVocoder::prepare(double sampleRate)
{
    m_sampleRate = sampleRate;
    reset();
}

void PhaseVocoder::reset()
{
    std::fill(m_lastAnalysisPhase.begin(), m_lastAnalysisPhase.end(), 0.0f);
    std::fill(m_synthPhaseAccum  .begin(), m_synthPhaseAccum  .end(), 0.0f);
    std::fill(m_olaOut           .begin(), m_olaOut           .end(), 0.0f);
    std::fill(m_inFifo           .begin(), m_inFifo           .end(), 0.0f);
    std::fill(m_outFifo          .begin(), m_outFifo          .end(), 0.0f);
    m_inFill = m_outFill = 0;
}

void PhaseVocoder::setPitchShift(float semitones)
{
    m_pitchRatio = std::pow(2.0f, semitones / 12.0f);
}

void PhaseVocoder::setTimeStretch(float ratio)
{
    m_timeStretch = (ratio > 0.01f) ? ratio : 0.01f;
}

void PhaseVocoder::processFrame()
{
    const int bins = m_fftSize / 2 + 1;
    const float twoPi = 2.0f * (float)M_PI;
    // Expected phase advance per hop for bin k: 2π * k * hopSize / fftSize
    const float phaseAdvPerBin = twoPi * (float)m_hopSize / (float)m_fftSize;

    // ---- Analysis FFT ----
    // Fill fftBuf: first fftSize reals from input FIFO (windowed), rest = 0
    for (int i = 0; i < m_fftSize; ++i)
        m_fftBuf[i] = m_inFifo[i] * m_window[i];
    std::fill(m_fftBuf.begin() + m_fftSize, m_fftBuf.end(), 0.0f);

    m_fft.performRealOnlyForwardTransform(m_fftBuf.data(), true);

    // Unpack complex pairs and compute true frequencies
    for (int k = 0; k < bins; ++k) {
        float re = m_fftBuf[k * 2];
        float im = m_fftBuf[k * 2 + 1];

        float mag   = std::sqrt(re * re + im * im);
        float phase = std::atan2(im, re);

        // Instantaneous frequency deviation from bin centre
        float phaseDiff = phase - m_lastAnalysisPhase[k] - (float)k * phaseAdvPerBin;
        m_lastAnalysisPhase[k] = phase;

        // Wrap to [-π, π]
        phaseDiff -= twoPi * std::round(phaseDiff / twoPi);

        // True frequency in fractional bins
        m_analysisMag [k] = mag;
        m_analysisFreq[k] = (float)k + phaseDiff / phaseAdvPerBin;
    }

    // ---- Synthesis: pitch-shift by resampling bins ----
    std::fill(m_fftBuf.begin(), m_fftBuf.end(), 0.0f);

    // Synthesis hop accounts for time stretch
    const float synthPhaseAdvPerBin = phaseAdvPerBin * m_timeStretch;

    // Accumulate shifted bins
    std::vector<float> synthMag (bins, 0.0f);
    std::vector<float> synthFreq(bins, 0.0f);

    for (int k = 0; k < bins; ++k) {
        int destBin = static_cast<int>(std::round(k * m_pitchRatio));
        if (destBin < 0 || destBin >= bins) continue;
        synthMag [destBin] += m_analysisMag [k];
        synthFreq[destBin]  = m_analysisFreq[k] * m_pitchRatio;
    }

    // Accumulate synthesis phases and write complex spectrum
    for (int k = 0; k < bins; ++k) {
        m_synthPhaseAccum[k] += synthFreq[k] * synthPhaseAdvPerBin;
        float ph = m_synthPhaseAccum[k];
        m_fftBuf[k * 2]     = synthMag[k] * std::cos(ph);
        m_fftBuf[k * 2 + 1] = synthMag[k] * std::sin(ph);
    }

    // ---- Inverse FFT ----
    m_fft.performRealOnlyInverseTransform(m_fftBuf.data());

    // Overlap-add into olaOut (shifted by hopSize each frame)
    for (int i = 0; i < m_fftSize; ++i) {
        int pos = i; // relative to current write head
        if (pos < (int)m_olaOut.size())
            m_olaOut[pos] += m_fftBuf[i];
    }

    // Drain hopSize samples from olaOut into outFifo
    for (int i = 0; i < m_hopSize; ++i) {
        if (m_outFill < (int)m_outFifo.size())
            m_outFifo[m_outFill++] = m_olaOut[i];
    }
    // Slide olaOut
    std::copy(m_olaOut.begin() + m_hopSize, m_olaOut.end(), m_olaOut.begin());
    std::fill(m_olaOut.end() - m_hopSize, m_olaOut.end(), 0.0f);
}

int PhaseVocoder::process(const float* input, int numIn, float* output, int maxOut)
{
    int written = 0;

    for (int i = 0; i < numIn; ++i) {
        // Push into input FIFO
        if (m_inFill < (int)m_inFifo.size())
            m_inFifo[m_inFill++] = input[i];

        // When we have a full frame, process and slide by hopSize
        if (m_inFill >= m_fftSize) {
            processFrame();
            // Slide input FIFO
            int remaining = m_inFill - m_hopSize;
            std::copy(m_inFifo.begin() + m_hopSize,
                      m_inFifo.begin() + m_inFill,
                      m_inFifo.begin());
            m_inFill = remaining;
        }

        // Drain one sample from output FIFO
        if (m_outFill > 0 && written < maxOut) {
            output[written++] = m_outFifo[0];
            --m_outFill;
            std::copy(m_outFifo.begin() + 1,
                      m_outFifo.begin() + 1 + m_outFill,
                      m_outFifo.begin());
        }
    }

    return written;
}
