#include "PluginProcessor.h"
#include "PluginEditor.h"

// Global function required by JUCE's ARA machinery — tells it which
// document controller specialisation to instantiate.
const ARA::ARAFactory* JUCE_CALLTYPE createARAFactory()
{
    return juce::ARADocumentControllerSpecialisation::createARAFactory<PEDocumentController>();
}

// ─── Constructor ──────────────────────────────────────────────────────────────

PitchEditProcessor::PitchEditProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

// ─── Layout ──────────────────────────────────────────────────────────────────

bool PitchEditProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() ||
           out == juce::AudioChannelSet::stereo();
}

// ─── Lifecycle ────────────────────────────────────────────────────────────────

void PitchEditProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    if (auto* renderer = getPlaybackRenderer<PEPlaybackRenderer>())
        renderer->prepareToPlay(sampleRate, samplesPerBlock,
                                getTotalNumOutputChannels(),
                                getProcessingPrecision(),
                                juce::ARAPlaybackRenderer::AlwaysNonRealtime::no);
}

void PitchEditProcessor::releaseResources()
{
    if (auto* renderer = getPlaybackRenderer<PEPlaybackRenderer>())
        renderer->releaseResources();
}

// ─── Processing ──────────────────────────────────────────────────────────────

void PitchEditProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (isPlaybackRenderer())
    {
        auto* renderer = getPlaybackRenderer<PEPlaybackRenderer>();
        if (!renderer) { buffer.clear(); return; }

        juce::AudioPlayHead::PositionInfo posInfo;
        if (auto* ph = getPlayHead())
            if (auto p = ph->getPosition())
                posInfo = *p;

        renderer->processBlock(buffer, juce::AudioProcessor::Realtime::yes, posInfo);
    }
    // Without ARA: pass-through (do nothing)
}

// ─── Editor ──────────────────────────────────────────────────────────────────

juce::AudioProcessorEditor* PitchEditProcessor::createEditor()
{
    return new PitchEditEditor(*this);
}

// ─── State ───────────────────────────────────────────────────────────────────

void PitchEditProcessor::getStateInformation(juce::MemoryBlock&)    {}
void PitchEditProcessor::setStateInformation(const void*, int)      {}

// ─── Plugin entry point ──────────────────────────────────────────────────────

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PitchEditProcessor();
}
