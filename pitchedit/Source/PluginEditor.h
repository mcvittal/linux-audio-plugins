#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ARADocumentController.h"
#include "NoteCanvas.h"

class PitchEditEditor final : public juce::AudioProcessorEditor,
                               public juce::AudioProcessorEditorARAExtension,
                               public juce::ARAEditorView::Listener,
                               public PEDocumentController::Listener
{
public:
    explicit PitchEditEditor(PitchEditProcessor&);
    ~PitchEditEditor() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

    // juce::ARAEditorView::Listener
    void onNewSelection     (const juce::ARAViewSelection&)             override;
    void onHideRegionSequences(const std::vector<juce::ARARegionSequence*>&) override {}

    // PEDocumentController::Listener
    void analysisComplete(PEAudioSource*) override;

private:
    PitchEditProcessor& m_proc;

    juce::TextButton m_btnSnapPitch  { "Snap to Grid" };
    juce::TextButton m_btnSelectAll  { "Select All"   };
    juce::TextButton m_btnResetPitch { "Reset Pitch"  };
    juce::Label      m_lblStatus;

    NoteCanvas m_canvas;

    PEAudioModification* m_currentMod { nullptr };

    // Returns the document controller specialisation, or nullptr if not in ARA context.
    PEDocumentController* peDocumentController();

    void connectToARA();
    void refreshCanvas();
    void onNoteEdited(int noteId, double pitchCorr, double timeOff);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchEditEditor)
};
