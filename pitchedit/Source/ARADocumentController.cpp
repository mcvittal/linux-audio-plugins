#include "ARADocumentController.h"
#include <cmath>
#include <algorithm>

// ─── PEAudioModification ─────────────────────────────────────────────────────

const std::vector<NoteBlob>& PEAudioModification::activeNotes() const
{
    if (hasEdits) return editedNotes;

    if (auto* src = dynamic_cast<PEAudioSource*>(getAudioSource()))
        return src->analysisData.notes;

    static const std::vector<NoteBlob> empty;
    return empty;
}

// ─── PEPlaybackRenderer ───────────────────────────────────────────────────────

void PEPlaybackRenderer::prepareToPlay(double sr, int maxBlock, int numCh,
                                        juce::AudioProcessor::ProcessingPrecision,
                                        AlwaysNonRealtime)
{
    m_sampleRate   = sr;
    m_numChannels  = numCh;
    m_maxBlockSize = maxBlock;
    rebuildRegionState(sr, maxBlock, numCh);
}

void PEPlaybackRenderer::releaseResources()
{
    m_regionState.clear();
}

void PEPlaybackRenderer::rebuildRegionState(double sr, int /*maxBlock*/, int numCh)
{
    m_regionState.clear();

    for (auto* region : getPlaybackRegions())
    {
        auto* mod = dynamic_cast<PEAudioModification*>(region->getAudioModification());
        if (!mod) continue;
        auto* src = dynamic_cast<PEAudioSource*>(mod->getAudioSource());
        if (!src) continue;

        RegionState rs;
        rs.reader = std::make_unique<juce::ARAAudioSourceReader>(src);

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto v = std::make_unique<PhaseVocoder>();
            v->prepare(sr);
            rs.vocoders.push_back(std::move(v));
        }

        m_regionState.emplace(region, std::move(rs));
    }
}

bool PEPlaybackRenderer::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::AudioProcessor::Realtime,
                                       const juce::AudioPlayHead::PositionInfo& pos) noexcept
{
    buffer.clear();

    const auto timeOpt = pos.getTimeInSeconds();
    if (!timeOpt.hasValue()) return true;

    const double playTime = *timeOpt;
    const int    numSamps = buffer.getNumSamples();
    const int    numCh    = buffer.getNumChannels();

    for (auto& [region, rs] : m_regionState)
    {
        if (!rs.reader || !rs.reader->isValid()) continue;

        const double regionStart = region->getStartInPlaybackTime();
        const double regionEnd   = region->getEndInPlaybackTime();
        if (playTime + numSamps / m_sampleRate < regionStart || playTime >= regionEnd)
            continue;

        // Map playback time → source audio position
        const double playDur = region->getDurationInPlaybackTime();
        const double srcDur  = region->getDurationInAudioModificationTime();
        const double rate    = (playDur > 0.0) ? srcDur / playDur : 1.0;

        const double srcStart   = region->getStartInAudioModificationTime();
        const double srcTimeSec = srcStart + (playTime - regionStart) * rate;

        // juce::int64 = long long on all platforms; int64_t = long on Linux x64
        const juce::int64 srcSample = static_cast<juce::int64>(srcTimeSec * m_sampleRate);

        // Find pitch correction from the note currently under the playhead
        float pitchShiftSemitones = 0.0f;
        if (auto* mod = dynamic_cast<PEAudioModification*>(region->getAudioModification()))
        {
            for (const auto& note : mod->activeNotes())
            {
                if (srcTimeSec >= note.startTime && srcTimeSec < note.editedEndTime() && !note.muted)
                {
                    double quantized = std::round(note.basePitchMidi);
                    pitchShiftSemitones = static_cast<float>((quantized - note.basePitchMidi)
                                                             + note.pitchCorrection);
                    break;
                }
            }
        }

        for (int ch = 0; ch < (int)rs.vocoders.size() && ch < numCh; ++ch)
            rs.vocoders[ch]->setPitchShift(pitchShiftSemitones);

        // Read source audio using AudioFormatReader interface
        // Signature: read(float* const* destChannels, int numDestChannels,
        //                 int64 startSampleInFile, int numSamples, bool fillLeftover)
        juce::AudioBuffer<float> srcBuf(numCh, numSamps);
        srcBuf.clear();

        std::vector<float*> chPtrs;
        for (int ch = 0; ch < numCh; ++ch)
            chPtrs.push_back(srcBuf.getWritePointer(ch));

        rs.reader->read(chPtrs.data(), numCh, srcSample, numSamps);

        // Apply phase vocoder and mix into output
        std::vector<float> procOut(numSamps, 0.0f);
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float* inPtr = srcBuf.getReadPointer(ch);
            if (ch < (int)rs.vocoders.size())
            {
                int written = rs.vocoders[ch]->process(inPtr, numSamps,
                                                        procOut.data(), numSamps);
                buffer.addFrom(ch, 0, procOut.data(), written);
            }
            else
            {
                buffer.addFrom(ch, 0, inPtr, numSamps);
            }
        }
    }

    return true;
}

// ─── PEDocumentController — factory overrides ─────────────────────────────────

juce::ARAAudioSource* PEDocumentController::doCreateAudioSource(
    juce::ARADocument* document,
    ARA::ARAAudioSourceHostRef hostRef)
{
    // AudioSource constructor takes (Document*, hostRef) — not the DocumentController
    auto* src = new PEAudioSource(document, hostRef);
    scheduleAnalysis(src);
    return src;
}

juce::ARAAudioModification* PEDocumentController::doCreateAudioModification(
    juce::ARAAudioSource* audioSource,
    ARA::ARAAudioModificationHostRef hostRef,
    const juce::ARAAudioModification* optionalModificationToClone)
{
    auto* mod = new PEAudioModification(audioSource, hostRef, optionalModificationToClone);
    if (auto* orig = dynamic_cast<const PEAudioModification*>(optionalModificationToClone))
    {
        mod->editedNotes = orig->editedNotes;
        mod->hasEdits    = orig->hasEdits;
    }
    return mod;
}

juce::ARAPlaybackRegion* PEDocumentController::doCreatePlaybackRegion(
    juce::ARAAudioModification* modification,
    ARA::ARAPlaybackRegionHostRef hostRef)
{
    return new PEPlaybackRegion(modification, hostRef);
}

juce::ARAPlaybackRenderer* PEDocumentController::doCreatePlaybackRenderer() noexcept
{
    return new PEPlaybackRenderer(getDocumentController());
}

juce::ARAEditorRenderer* PEDocumentController::doCreateEditorRenderer() noexcept
{
    return new juce::ARAEditorRenderer(getDocumentController());
}

juce::ARAEditorView* PEDocumentController::doCreateEditorView() noexcept
{
    return new juce::ARAEditorView(getDocumentController());
}

// ─── Persistence ─────────────────────────────────────────────────────────────

bool PEDocumentController::doRestoreObjectsFromStream(
    juce::ARAInputStream& input,
    const juce::ARARestoreObjectsFilter* /*filter*/)
{
    juce::int64 len = 0;
    if (input.read(&len, sizeof(len)) != sizeof(len) || len <= 0) return true;

    juce::MemoryBlock mb;
    mb.setSize((size_t)len);
    if (input.read(mb.getData(), (int)len) != (int)len) return true;

    auto json = juce::JSON::parse(mb.toString());
    if (!json.isObject()) return true;

    for (auto* src : getDocument()->getAudioSources())
    {
        for (auto* mod : src->getAudioModifications<PEAudioModification>())
        {
            // Use hex address string as a stable key within an archive session
            auto key = juce::String::toHexString((juce::int64)(uintptr_t)mod);
            auto notesJson = json[juce::Identifier(key)];
            if (!notesJson.isArray()) continue;

            mod->editedNotes.clear();
            for (int i = 0; i < notesJson.size(); ++i)
            {
                auto n = notesJson[i];
                NoteBlob blob;
                blob.id             = (int)    n["id"];
                blob.startTime      = (double)  n["st"];
                blob.duration       = (double)  n["dur"];
                blob.basePitchMidi  = (double)  n["bpm"];
                blob.pitchCorrection= (double)  n["pc"];
                blob.timeOffset     = (double)  n["to"];
                blob.amplitude      = (float)   n["amp"];
                blob.muted          = (bool)    n["mute"];
                mod->editedNotes.push_back(blob);
            }
            mod->hasEdits = true;
        }
    }

    return true;
}

bool PEDocumentController::doStoreObjectsToStream(
    juce::ARAOutputStream& output,
    const juce::ARAStoreObjectsFilter* /*filter*/)
{
    juce::DynamicObject::Ptr root(new juce::DynamicObject());

    for (auto* src : getDocument()->getAudioSources())
    {
        for (auto* mod : src->getAudioModifications<PEAudioModification>())
        {
            if (!mod->hasEdits) continue;

            auto key = juce::String::toHexString((juce::int64)(uintptr_t)mod);
            juce::Array<juce::var> notesArr;

            for (const auto& blob : mod->editedNotes)
            {
                juce::DynamicObject::Ptr n(new juce::DynamicObject());
                n->setProperty("id",   blob.id);
                n->setProperty("st",   blob.startTime);
                n->setProperty("dur",  blob.duration);
                n->setProperty("bpm",  blob.basePitchMidi);
                n->setProperty("pc",   blob.pitchCorrection);
                n->setProperty("to",   blob.timeOffset);
                n->setProperty("amp",  blob.amplitude);
                n->setProperty("mute", (int)blob.muted);
                notesArr.add(juce::var(n.get()));
            }
            root->setProperty(key, notesArr);
        }
    }

    juce::String jsonStr = juce::JSON::toString(juce::var(root.get()));
    juce::MemoryBlock mb;
    mb.append(jsonStr.toRawUTF8(), jsonStr.getNumBytesAsUTF8());
    juce::int64 len = (juce::int64)mb.getSize();
    output.write(&len,         sizeof(len));
    output.write(mb.getData(), mb.getSize());

    return true;
}

// ─── Analysis ─────────────────────────────────────────────────────────────────

void PEDocumentController::addListener(Listener* l)
{
    std::lock_guard<std::mutex> lk(m_listenerMutex);
    m_listeners.push_back(l);
}

void PEDocumentController::removeListener(Listener* l)
{
    std::lock_guard<std::mutex> lk(m_listenerMutex);
    m_listeners.erase(std::remove(m_listeners.begin(), m_listeners.end(), l),
                      m_listeners.end());
}

void PEDocumentController::scheduleAnalysis(PEAudioSource* source)
{
    source->needsAnalysis = true;
    m_threadPool.addJob([this, source]() { runAnalysis(source); });
}

void PEDocumentController::runAnalysis(PEAudioSource* source)
{
    const double   sr    = source->getSampleRate();
    const juce::int64 total = source->getSampleCount();
    const int      numCh = source->getChannelCount();

    juce::ARAAudioSourceReader reader(source);
    if (!reader.isValid()) return;

    // Accumulate all channels into a mono buffer
    const int chunkSize = 65536;
    juce::AudioBuffer<float> mono(1, (int)total);
    mono.clear();

    std::vector<float*> chPtrs(numCh);
    juce::AudioBuffer<float> chunk(numCh, chunkSize);

    for (juce::int64 pos = 0; pos < total; pos += chunkSize)
    {
        int n = (int)std::min((juce::int64)chunkSize, total - pos);
        chunk.setSize(numCh, n, false, false, true);

        for (int ch = 0; ch < numCh; ++ch)
            chPtrs[ch] = chunk.getWritePointer(ch);
        reader.read(chPtrs.data(), numCh, pos, n);

        for (int ch = 0; ch < numCh; ++ch)
            mono.addFrom(0, (int)pos, chunk, ch, 0, n, 1.0f / numCh);
    }

    source->analysisData.sampleRate    = sr;
    source->analysisData.totalDuration = total / sr;

    m_detector.analyzeBuffer(mono.getReadPointer(0), (int)total, (float)sr,
                              source->analysisData.allFrames, 512);

    m_segmenter.segment(source->analysisData.allFrames, sr,
                        source->analysisData.notes);

    source->analysisData.analysisComplete = true;
    source->needsAnalysis = false;

    juce::MessageManager::callAsync([this, source]() { notifyListeners(source); });
}

void PEDocumentController::notifyListeners(PEAudioSource* source)
{
    std::lock_guard<std::mutex> lk(m_listenerMutex);
    for (auto* l : m_listeners)
        l->analysisComplete(source);
}
