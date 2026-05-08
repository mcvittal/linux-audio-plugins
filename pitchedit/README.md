# PitchEdit — Linux ARA2 Pitch Editor VST3

A Melodyne-style pitch and time editor plugin for Linux, built with JUCE 7 + ARA2.

## Features
- **ARA2 integration** — seamless, non-destructive audio access via ARA2-capable DAWs (Reaper 6.51+, Studio One, Cubase 12+)
- **YIN pitch detection** — accurate monophonic pitch analysis using the de Cheveigné / Kawahara algorithm
- **Phase vocoder** — 2048-point overlap-add pitch shifter with formant-preserving synthesis
- **Piano-roll note canvas** — drag notes up/down to correct pitch; horizontal nudge for timing
- **Per-note pitch contour** — micro-pitch wiggles rendered inside each note blob
- **Snap to grid** — one-click quantize to nearest semitone
- **Project persistence** — note edits serialised as JSON inside the DAW project via ARA2 archive API

## Build Requirements

```bash
sudo apt install \
    cmake ninja-build git \
    libasound2-dev libfreetype6-dev \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
    libglu1-mesa-dev libcurl4-openssl-dev
```

CMake ≥ 3.22 and a C++17 compiler (GCC 11+ or Clang 14+) are required.  
JUCE 7.0.12 is fetched automatically via `FetchContent` — no manual download needed.

## Building

```bash
./build.sh
# or manually:
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Output: `build/PitchEdit_artefacts/Release/VST3/PitchEdit.vst3`

Install to your user VST3 folder:
```bash
cp -r build/PitchEdit_artefacts/Release/VST3/PitchEdit.vst3 ~/.vst3/
```

## DAW Setup (Reaper example)

1. Open Reaper → Preferences → Plug-ins → VST → scan for new plug-ins
2. On any audio track, click the FX button → add **PitchEdit**
3. Right-click the plugin instance → **Open ARA editor** (Reaper 6.57+)
4. The plugin analyses all audio in the track and displays detected notes
5. Drag notes vertically to correct pitch; double-click to reset

## Architecture

```
Source/
├── NoteData.h              — PitchFrame / NoteBlob / AudioAnalysisData structs
├── PitchDetector.h/cpp     — YIN pitch detection (per-frame + whole-buffer)
├── PhaseVocoder.h/cpp      — JUCE FFT-based phase vocoder pitch shifter
├── NoteSegmenter.h/cpp     — Convert pitch frames → NoteBlob list
├── ARADocumentController.h/cpp  — ARA2 document model + background analysis
├── PluginProcessor.h/cpp   — VST3 AudioProcessor + ARA extension
├── NoteCanvas.h/cpp        — JUCE Component: piano roll editor
└── PluginEditor.h/cpp      — Main editor shell + toolbar
```

## Extending

| Goal | Where |
|------|--------|
| Polyphonic pitch tracking | Replace YIN in `PitchDetector` with CREPE or pYIN |
| Formant correction | Add spectral envelope division in `PhaseVocoder::processFrame` |
| Time stretching | `PEPlaybackRenderer::processBlock` — adjust `regionRate` and chain vocoder |
| MIDI export | Iterate `PEAudioModification::activeNotes()` in the editor |
| LV2 format | Add `FORMATS LV2` to `juce_add_plugin` (JUCE 7 experimental) |

## Limitations
- Monophonic pitch detection only (one note at a time)
- ARA2 requires a compatible DAW; without it the plugin passes audio through unchanged
- Phase vocoder introduces ~2048 samples of latency (reported via `getLatencySamples`)
- No MIDI out, no polyphonic decomposition (these are Melodyne Essential vs. DNA territory)
