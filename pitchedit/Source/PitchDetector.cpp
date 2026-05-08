#include "PitchDetector.h"
#include <cmath>
#include <numeric>
#include <algorithm>

PitchDetector::PitchDetector(int frameSize, float yinThreshold)
    : m_frameSize(frameSize), m_threshold(yinThreshold)
{
}

float PitchDetector::rms(const float* buf, int n) const
{
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) sum += buf[i] * buf[i];
    return std::sqrt(sum / n);
}

// Step 2: difference function d(tau) = sum_j (x_j - x_{j+tau})^2
void PitchDetector::computeDifference(const float* buf, int n,
                                       std::vector<float>& df) const
{
    int half = n / 2;
    df.assign(half, 0.0f);
    for (int tau = 0; tau < half; ++tau) {
        float s = 0.0f;
        for (int j = 0; j < half; ++j) {
            float d = buf[j] - buf[j + tau];
            s += d * d;
        }
        df[tau] = s;
    }
}

// Step 3: cumulative mean normalized difference function
void PitchDetector::computeCMNDF(std::vector<float>& df) const
{
    df[0] = 1.0f;
    float runSum = 0.0f;
    for (int tau = 1; tau < (int)df.size(); ++tau) {
        runSum += df[tau];
        df[tau] = (runSum > 0.0f) ? df[tau] * tau / runSum : 1.0f;
    }
}

// Step 4+5: find first tau below threshold; fall back to global minimum
int PitchDetector::findPeriod(const std::vector<float>& cmndf) const
{
    const int minTau = 2;
    const int maxTau = (int)cmndf.size() - 1;

    for (int tau = minTau; tau <= maxTau; ++tau) {
        if (cmndf[tau] < m_threshold) {
            while (tau + 1 <= maxTau && cmndf[tau + 1] < cmndf[tau])
                ++tau;
            return tau;
        }
    }
    // No clear period — pick global minimum as fallback
    int best = minTau;
    for (int tau = minTau + 1; tau <= maxTau; ++tau)
        if (cmndf[tau] < cmndf[best]) best = tau;

    return (cmndf[best] < 0.5f) ? best : -1;
}

// Step 6: sub-sample refinement via parabolic interpolation
float PitchDetector::parabolicRefine(const std::vector<float>& cmndf, int tau) const
{
    if (tau <= 0 || tau >= (int)cmndf.size() - 1) return (float)tau;
    float s0 = cmndf[tau - 1], s1 = cmndf[tau], s2 = cmndf[tau + 1];
    float denom = 2.0f * (s0 - 2.0f * s1 + s2);
    if (std::abs(denom) < 1e-8f) return (float)tau;
    return (float)tau + (s0 - s2) / denom;
}

float PitchDetector::detectPitch(const float* samples, int numSamples,
                                  float sampleRate) const
{
    if (numSamples < 4) return 0.0f;
    std::vector<float> df;
    computeDifference(samples, numSamples, df);
    computeCMNDF(df);
    int tau = findPeriod(df);
    if (tau < 0) return 0.0f;
    float refined = parabolicRefine(df, tau);
    return refined >= 1.0f ? sampleRate / refined : 0.0f;
}

void PitchDetector::analyzeBuffer(const float* samples,
                                   int numSamples,
                                   float sampleRate,
                                   std::vector<PitchFrame>& outFrames,
                                   int hopSize) const
{
    outFrames.clear();
    int pos = 0;
    while (pos + m_frameSize <= numSamples) {
        const float* frame = samples + pos;

        PitchFrame pf;
        pf.timeSeconds   = (double)pos / sampleRate;
        pf.rmsAmplitude  = rms(frame, m_frameSize);

        if (pf.rmsAmplitude > 0.001f) {
            std::vector<float> df;
            computeDifference(frame, m_frameSize, df);
            computeCMNDF(df);
            int tau = findPeriod(df);
            if (tau > 0) {
                float refined = parabolicRefine(df, tau);
                if (refined >= 1.0f) {
                    pf.pitchHz = sampleRate / refined;
                    // Map to MIDI: A4=440Hz=note69
                    if (pf.pitchHz > 20.0f && pf.pitchHz < 20000.0f) {
                        pf.pitchMidi  = 69.0f + 12.0f * std::log2f(pf.pitchHz / 440.0f);
                        pf.confidence = std::max(0.0f, std::min(1.0f, 1.0f - df[tau]));
                    }
                }
            }
        }

        outFrames.push_back(pf);
        pos += hopSize;
    }
}
