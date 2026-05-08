#include "PluginProcessor.h"
#include "PluginEditor.h"

DpsV55Processor::DpsV55Processor()
    : AudioProcessor(BusesProperties())
{
}

DpsV55Processor::~DpsV55Processor() {}

// Called on the message thread. Builds a Sony SysEx parameter-transfer message.
// Format: F0 4C 0n 24 20 [block] [param] [v>>12&F] [v>>8&F] [v>>4&F] [v&F] F7
void DpsV55Processor::queueParamChange(int block, int param, int value)
{
    const auto ch = static_cast<uint8_t>((midiChannel.load() - 1) & 0x0F);
    const auto v  = static_cast<uint16_t>(static_cast<int16_t>(value));

    const uint8_t data[12] = {
        0xF0, 0x4C, ch, 0x24, 0x20,
        static_cast<uint8_t>(block),
        static_cast<uint8_t>(param),
        static_cast<uint8_t>((v >> 12) & 0x0F),
        static_cast<uint8_t>((v >>  8) & 0x0F),
        static_cast<uint8_t>((v >>  4) & 0x0F),
        static_cast<uint8_t>( v        & 0x0F),
        0xF7
    };

    const juce::ScopedLock sl(sysexLock);
    sysexQueue.push_back(juce::MidiMessage(data, 12));
}

void DpsV55Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    buffer.clear();
    midi.clear();

    const int ch = midiChannel.load();

    // Drain SysEx queue (effect type / parameter changes)
    if (sysexLock.tryEnter())
    {
        for (auto& m : sysexQueue)
            midi.addEvent(m, 0);
        sysexQueue.clear();
        sysexLock.exit();
    }

    if (pendingProgramChange.exchange(false))
        midi.addEvent(juce::MidiMessage::programChange(ch, programNumber.load()), 0);

    if (pendingVolumeChange.exchange(false))
        midi.addEvent(juce::MidiMessage::controllerEvent(ch, 7, masterVolume.load()), 0);

    if (pendingAllNotesOff.exchange(false))
    {
        midi.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
        midi.addEvent(juce::MidiMessage::allSoundOff(ch), 0);
    }
}

juce::AudioProcessorEditor* DpsV55Processor::createEditor()
{
    return new DpsV55Editor(*this);
}

void DpsV55Processor::getStateInformation(juce::MemoryBlock& dest)
{
    juce::XmlElement xml("DpsV55State");
    xml.setAttribute("midiChannel",   (int)midiChannel.load());
    xml.setAttribute("programNumber", (int)programNumber.load());
    xml.setAttribute("masterVolume",  (int)masterVolume.load());
    xml.setAttribute("fxAType",       (int)fxAType.load());
    xml.setAttribute("fxBType",       (int)fxBType.load());
    xml.setAttribute("fxAOnOff",      (int)fxAOnOff.load());
    xml.setAttribute("fxBOnOff",      (int)fxBOnOff.load());
    xml.setAttribute("fxStructure",   (int)fxStructure.load());
    copyXmlToBinary(xml, dest);
}

void DpsV55Processor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
    {
        midiChannel.store   (xml->getIntAttribute("midiChannel",   1));
        programNumber.store (xml->getIntAttribute("programNumber", 0));
        masterVolume.store  (xml->getIntAttribute("masterVolume",  100));
        fxAType.store       (xml->getIntAttribute("fxAType",       1));
        fxBType.store       (xml->getIntAttribute("fxBType",       10));
        fxAOnOff.store      (xml->getIntAttribute("fxAOnOff",      1));
        fxBOnOff.store      (xml->getIntAttribute("fxBOnOff",      1));
        fxStructure.store   (xml->getIntAttribute("fxStructure",   0));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DpsV55Processor();
}
