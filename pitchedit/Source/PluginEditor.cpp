#include "PluginEditor.h"
#include <algorithm>

// ─── Constructor / destructor ─────────────────────────────────────────────────

PitchEditEditor::PitchEditEditor(PitchEditProcessor& proc)
    : AudioProcessorEditor(proc),
      AudioProcessorEditorARAExtension(&proc),   // required for getARAEditorView()
      m_proc(proc)
{
    setSize(900, 520);
    setResizable(true, false);
    setResizeLimits(600, 340, 2400, 1400);

    // Style toolbar buttons
    auto styleBtn = [](juce::TextButton& b) {
        b.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xFF2D2D4E));
        b.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xFF4040AA));
        b.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    };
    styleBtn(m_btnSnapPitch);
    styleBtn(m_btnSelectAll);
    styleBtn(m_btnResetPitch);

    m_lblStatus.setColour(juce::Label::textColourId, juce::Colours::grey);
    m_lblStatus.setFont(12.0f);
    m_lblStatus.setText("No ARA audio source loaded", juce::dontSendNotification);

    m_btnSnapPitch.onClick = [this] {
        if (!m_currentMod) return;
        for (auto& n : m_currentMod->editedNotes)
            n.pitchCorrection = std::round(n.basePitchMidi) - n.basePitchMidi;
        m_currentMod->hasEdits = true;
        refreshCanvas();
    };

    m_btnSelectAll.onClick = [this] {
        if (!m_currentMod) return;
        auto& notes = m_currentMod->editedNotes;
        bool allSel = std::all_of(notes.begin(), notes.end(),
                                   [](const NoteBlob& n) { return n.selected; });
        for (auto& n : notes) n.selected = !allSel;
        m_canvas.repaint();
    };

    m_btnResetPitch.onClick = [this] {
        if (!m_currentMod) return;
        for (auto& n : m_currentMod->editedNotes)
            if (n.selected) n.pitchCorrection = 0.0;
        refreshCanvas();
    };

    addAndMakeVisible(m_btnSnapPitch);
    addAndMakeVisible(m_btnSelectAll);
    addAndMakeVisible(m_btnResetPitch);
    addAndMakeVisible(m_lblStatus);
    addAndMakeVisible(m_canvas);

    m_canvas.setNoteEditCallback([this](int id, double pc, double to) {
        onNoteEdited(id, pc, to);
    });

    connectToARA();
}

PitchEditEditor::~PitchEditEditor()
{
    if (auto* dc = peDocumentController()) dc->removeListener(this);
    if (auto* view = getARAEditorView())   view->removeListener(this);
}

// ─── Layout ───────────────────────────────────────────────────────────────────

void PitchEditEditor::resized()
{
    auto area    = getLocalBounds();
    auto toolbar = area.removeFromTop(36).reduced(4, 4);

    const int btnW = 110;
    m_btnSnapPitch .setBounds(toolbar.removeFromLeft(btnW).reduced(2, 0));
    m_btnSelectAll .setBounds(toolbar.removeFromLeft(btnW).reduced(2, 0));
    m_btnResetPitch.setBounds(toolbar.removeFromLeft(btnW).reduced(2, 0));
    toolbar.removeFromLeft(8);
    m_lblStatus.setBounds(toolbar);

    m_canvas.setBounds(area);
}

void PitchEditEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF12122A));
}

// ─── ARA wiring ──────────────────────────────────────────────────────────────

PEDocumentController* PitchEditEditor::peDocumentController()
{
    if (auto* view = getARAEditorView())
    {
        // getDocumentController() on ARA::PlugIn::EditorView returns ARA::PlugIn::DocumentController*
        auto* araPluginDC = view->getDocumentController();
        return juce::ARADocumentControllerSpecialisation
                   ::getSpecialisedDocumentController<PEDocumentController>(araPluginDC);
    }
    return nullptr;
}

void PitchEditEditor::connectToARA()
{
    if (auto* dc   = peDocumentController()) dc->addListener(this);
    if (auto* view = getARAEditorView())      view->addListener(this);
    refreshCanvas();
}

void PitchEditEditor::refreshCanvas()
{
    if (!m_currentMod) {
        m_canvas.setNotes(nullptr);
        m_lblStatus.setText("No region selected", juce::dontSendNotification);
        return;
    }

    // Promote analysis data into editedNotes the first time
    if (!m_currentMod->hasEdits) {
        if (auto* src = dynamic_cast<PEAudioSource*>(m_currentMod->getAudioSource()))
            if (src->analysisData.analysisComplete) {
                m_currentMod->editedNotes = src->analysisData.notes;
                m_currentMod->hasEdits    = true;
            }
    }

    auto& notes = m_currentMod->editedNotes;
    m_canvas.setNotes(&notes);

    if (!notes.empty()) {
        double minT = notes.front().startTime;
        double maxT = notes.back().editedEndTime();
        m_canvas.setTimeRange(minT, maxT + 1.0);

        float lo = 127.0f, hi = 0.0f;
        for (const auto& n : notes) {
            lo = std::min(lo, (float)n.correctedPitchMidi() - 3.0f);
            hi = std::max(hi, (float)n.correctedPitchMidi() + 3.0f);
        }
        m_canvas.setPitchRange(lo, hi);
        m_lblStatus.setText(juce::String(notes.size()) + " notes",
                             juce::dontSendNotification);
    } else {
        m_lblStatus.setText("Analysing…", juce::dontSendNotification);
    }

    m_canvas.repaint();
}

// ─── ARAEditorView::Listener ─────────────────────────────────────────────────

void PitchEditEditor::onNewSelection(const juce::ARAViewSelection& sel)
{
    m_currentMod = nullptr;

    for (auto* region : sel.getPlaybackRegions()) {
        m_currentMod = dynamic_cast<PEAudioModification*>(region->getAudioModification());
        if (m_currentMod) break;
    }

    refreshCanvas();
}

// ─── PEDocumentController::Listener ─────────────────────────────────────────

void PitchEditEditor::analysisComplete(PEAudioSource* src)
{
    if (m_currentMod && m_currentMod->getAudioSource() == src) {
        m_currentMod->editedNotes = src->analysisData.notes;
        m_currentMod->hasEdits    = true;
        refreshCanvas();
    }

    m_lblStatus.setText(juce::String(src->analysisData.notes.size())
                        + " notes detected",
                        juce::dontSendNotification);
}

// ─── Note editing ────────────────────────────────────────────────────────────

void PitchEditEditor::onNoteEdited(int noteId, double pitchCorr, double timeOff)
{
    if (!m_currentMod) return;

    for (auto& n : m_currentMod->editedNotes) {
        if (n.id != noteId) continue;
        n.pitchCorrection = juce::jlimit(-24.0, 24.0, pitchCorr);
        n.timeOffset      = juce::jlimit(-5.0,   5.0, timeOff);
        break;
    }

    m_currentMod->hasEdits = true;
    m_canvas.repaint();
}
