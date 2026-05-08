#include "NoteCanvas.h"
#include <cmath>
#include <algorithm>

NoteCanvas::NoteCanvas()
{
    setOpaque(true);
    startTimerHz(30);
}

NoteCanvas::~NoteCanvas()
{
    stopTimer();
}

void NoteCanvas::setNotes(const std::vector<NoteBlob>* notes)
{
    m_notes = notes;
    repaint();
}

void NoteCanvas::setTimeRange(double start, double end)
{
    m_viewStart = start;
    m_viewEnd   = std::max(end, start + 0.01);
    repaint();
}

void NoteCanvas::setPitchRange(float lo, float hi)
{
    m_pitchLo = lo;
    m_pitchHi = std::max(hi, lo + 1.0f);
    repaint();
}

void NoteCanvas::setNoteEditCallback(NoteEditCallback cb) { m_editCb = std::move(cb); }
void NoteCanvas::setPlayheadPosition(double s)            { m_playhead = s; }

// ─── Layout ──────────────────────────────────────────────────────────────────

juce::Rectangle<float> NoteCanvas::contentArea() const
{
    return getLocalBounds()
           .toFloat()
           .withTrimmedLeft  ((float)kPianoWidth)
           .withTrimmedTop   ((float)kHeaderHeight);
}

float NoteCanvas::timeToX(double t) const
{
    auto area = contentArea();
    double frac = (t - m_viewStart) / (m_viewEnd - m_viewStart);
    return area.getX() + (float)(frac * area.getWidth());
}

float NoteCanvas::pitchToY(float midi) const
{
    auto area = contentArea();
    float frac = (midi - m_pitchLo) / (m_pitchHi - m_pitchLo);
    // Higher pitch = lower y
    return area.getBottom() - frac * area.getHeight();
}

double NoteCanvas::xToTime(float x) const
{
    auto area = contentArea();
    double frac = (x - area.getX()) / area.getWidth();
    return m_viewStart + frac * (m_viewEnd - m_viewStart);
}

float NoteCanvas::yToPitch(float y) const
{
    auto area = contentArea();
    float frac = (area.getBottom() - y) / area.getHeight();
    return m_pitchLo + frac * (m_pitchHi - m_pitchLo);
}

juce::Rectangle<float> NoteCanvas::noteRect(const NoteBlob& n) const
{
    float x1 = timeToX(n.editedStartTime());
    float x2 = timeToX(n.editedEndTime());
    float midi = (float)n.correctedPitchMidi();

    float semitoneHeight = contentArea().getHeight() / (m_pitchHi - m_pitchLo);
    float noteH = std::max(semitoneHeight * 0.85f, 4.0f);
    float y     = pitchToY(midi) - noteH * 0.5f;

    return { x1, y, std::max(x2 - x1, 4.0f), noteH };
}

const NoteBlob* NoteCanvas::hitTest(juce::Point<float> pos) const
{
    if (!m_notes) return nullptr;
    for (auto it = m_notes->rbegin(); it != m_notes->rend(); ++it)
        if (noteRect(*it).expanded(2.0f).contains(pos)) return &(*it);
    return nullptr;
}

NoteBlob* NoteCanvas::hitTestMutable(juce::Point<float> pos)
{
    // We only hold a const pointer so we can't mutate; the owning layer does.
    return nullptr;
}

// ─── Paint ───────────────────────────────────────────────────────────────────

bool NoteCanvas::isBlackKey(int midiNote) const
{
    static const bool BLACK[12] = { false,true,false,true,false,false,
                                    true, false,true, false,true, false };
    return BLACK[midiNote % 12];
}

juce::Colour NoteCanvas::pitchClassColour(int pc) const
{
    // Spectral mapping: C=red … B=violet
    static const juce::Colour cols[12] = {
        juce::Colour(0xFFE53935), // C  red
        juce::Colour(0xFFFF7043), // C# orange-red
        juce::Colour(0xFFFB8C00), // D  orange
        juce::Colour(0xFFFFD600), // D# yellow
        juce::Colour(0xFF8BC34A), // E  yellow-green
        juce::Colour(0xFF43A047), // F  green
        juce::Colour(0xFF00ACC1), // F# teal
        juce::Colour(0xFF1E88E5), // G  blue
        juce::Colour(0xFF3949AB), // G# indigo
        juce::Colour(0xFF8E24AA), // A  purple
        juce::Colour(0xFFD81B60), // A# pink
        juce::Colour(0xFFB71C1C), // B  deep red
    };
    return cols[pc % 12];
}

juce::Colour NoteCanvas::noteColour(const NoteBlob& n) const
{
    int pc = (int)std::round(n.correctedPitchMidi()) % 12;
    if (pc < 0) pc += 12;
    auto col = pitchClassColour(pc);
    if (n.selected) col = col.brighter(0.4f);
    if (n.muted)    col = col.withAlpha(0.4f);
    return col;
}

void NoteCanvas::drawBackground(juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour(juce::Colour(0xFF1A1A2E));
    g.fillRect(area);

    float semH = area.getHeight() / (m_pitchHi - m_pitchLo);

    for (int midi = (int)std::floor(m_pitchLo); midi <= (int)std::ceil(m_pitchHi); ++midi) {
        float y = pitchToY((float)midi) - semH * 0.5f;
        if (isBlackKey(midi)) {
            g.setColour(juce::Colour(0xFF0D0D1A));
            g.fillRect(area.getX(), y, area.getWidth(), semH);
        }
        // Horizontal lane divider
        g.setColour(juce::Colour(0x18FFFFFF));
        g.drawHorizontalLine((int)(y + semH), area.getX(), area.getRight());

        // Highlight C notes
        if (midi % 12 == 0) {
            g.setColour(juce::Colour(0x28FFFFFF));
            g.drawHorizontalLine((int)y, area.getX(), area.getRight());
        }
    }

    // Vertical time grid (every second)
    g.setColour(juce::Colour(0x22FFFFFF));
    double step = 1.0;
    for (double t = std::ceil(m_viewStart); t <= m_viewEnd; t += step) {
        float x = timeToX(t);
        g.drawVerticalLine((int)x, area.getY(), area.getBottom());
    }
}

void NoteCanvas::drawPianoKeys(juce::Graphics& g, juce::Rectangle<float> pianoArea)
{
    float semH = contentArea().getHeight() / (m_pitchHi - m_pitchLo);

    for (int midi = (int)std::floor(m_pitchLo); midi <= (int)std::ceil(m_pitchHi); ++midi) {
        float y  = pitchToY((float)midi) - semH * 0.5f;
        bool black = isBlackKey(midi);

        juce::Rectangle<float> key(pianoArea.getX(), y,
                                   pianoArea.getWidth() * (black ? 0.65f : 1.0f),
                                   semH - 0.5f);

        g.setColour(black ? juce::Colours::black : juce::Colours::white);
        g.fillRect(key);
        g.setColour(juce::Colour(0x40000000));
        g.drawRect(key, 0.5f);

        // Note label for C notes
        if (midi % 12 == 0 && semH > 8.0f) {
            g.setColour(juce::Colours::grey);
            g.setFont(std::min(semH * 0.65f, 11.0f));
            g.drawText("C" + juce::String(midi / 12 - 1),
                       key.withTrimmedLeft(2.0f), juce::Justification::centredLeft, false);
        }
    }

    // Background behind keys
    g.setColour(juce::Colour(0xFF1A1A2E));
    g.fillRect(pianoArea.getRight() - 1.0f, pianoArea.getY(), 1.0f, pianoArea.getHeight());
}

void NoteCanvas::drawTimeline(juce::Graphics& g, juce::Rectangle<float> header)
{
    g.setColour(juce::Colour(0xFF12122A));
    g.fillRect(header);

    g.setFont(10.0f);
    g.setColour(juce::Colours::grey);

    double step = 1.0;
    if ((m_viewEnd - m_viewStart) > 60.0) step = 10.0;
    if ((m_viewEnd - m_viewStart) < 5.0)  step = 0.5;

    for (double t = std::ceil(m_viewStart / step) * step; t <= m_viewEnd; t += step) {
        float x = timeToX(t);
        if (x < (float)kPianoWidth) continue;
        int mins = (int)(t / 60);
        int secs = (int)t % 60;
        juce::String label = mins > 0
            ? juce::String(mins) + ":" + juce::String(secs).paddedLeft('0', 2)
            : juce::String(secs) + "s";
        g.drawVerticalLine((int)x, header.getY(), header.getBottom());
        g.drawText(label, (int)x + 2, (int)header.getY(), 40, (int)header.getHeight(),
                   juce::Justification::centredLeft, false);
    }
}

void NoteCanvas::drawNotes(juce::Graphics& g, juce::Rectangle<float>)
{
    if (!m_notes) return;

    for (const auto& note : *m_notes) {
        auto r = noteRect(note);
        if (r.getRight() < (float)kPianoWidth) continue;

        auto col = noteColour(note);

        // Shadow
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.fillRoundedRectangle(r.translated(1.5f, 1.5f), 3.0f);

        // Fill with gradient
        juce::ColourGradient grad(col.brighter(0.25f), r.getTopLeft(),
                                  col.darker(0.15f),   r.getBottomLeft(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(r, 3.0f);

        // Pitch contour overlay: draw a small waveform inside the blob
        if (note.frames.size() > 2 && r.getWidth() > 12.0f) {
            juce::Path contour;
            bool first = true;
            for (const auto& f : note.frames) {
                if (f.pitchMidi <= 0.0f) continue;
                float px = timeToX(f.timeSeconds);
                float py = pitchToY((float)(f.pitchMidi + note.pitchCorrection));
                py = juce::jlimit(r.getY(), r.getBottom(), py);
                if (first) { contour.startNewSubPath(px, py); first = false; }
                else        contour.lineTo(px, py);
            }
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.strokePath(contour, juce::PathStrokeType(1.0f));
        }

        // Border
        g.setColour(note.selected ? juce::Colours::white : col.darker(0.5f));
        g.drawRoundedRectangle(r, 3.0f, note.selected ? 1.5f : 0.8f);

        // Label: note name
        if (r.getWidth() > 24.0f && r.getHeight() > 8.0f) {
            static const char* NOTE_NAMES[] = {"C","C#","D","D#","E","F",
                                                "F#","G","G#","A","A#","B"};
            int midi = (int)std::round(note.correctedPitchMidi());
            juce::String name = NOTE_NAMES[midi % 12];
            g.setColour(juce::Colours::white.withAlpha(0.85f));
            g.setFont(std::min(r.getHeight() * 0.7f, 10.0f));
            g.drawText(name, r.reduced(2.0f, 0.0f),
                       juce::Justification::centredLeft, false);
        }
    }
}

void NoteCanvas::drawPlayhead(juce::Graphics& g, juce::Rectangle<float> area)
{
    if (m_playhead < m_viewStart || m_playhead > m_viewEnd) return;
    float x = timeToX(m_playhead);
    g.setColour(juce::Colour(0xFFFFDD00).withAlpha(0.85f));
    g.drawVerticalLine((int)x, area.getY(), area.getBottom());
    // Triangle head
    juce::Path head;
    head.addTriangle(x - 5, area.getY(), x + 5, area.getY(), x, area.getY() + 8);
    g.fillPath(head);
}

// ─── Main paint ──────────────────────────────────────────────────────────────

void NoteCanvas::paint(juce::Graphics& g)
{
    auto bounds   = getLocalBounds().toFloat();
    auto pianoArea  = bounds.withWidth(kPianoWidth).withTrimmedTop(kHeaderHeight);
    auto headerArea = bounds.withHeight(kHeaderHeight).withTrimmedLeft(kPianoWidth);
    auto content    = contentArea();

    drawBackground(g, content);
    drawPianoKeys (g, pianoArea);
    drawTimeline  (g, headerArea);

    // Corner fill
    g.setColour(juce::Colour(0xFF12122A));
    g.fillRect(bounds.withWidth(kPianoWidth).withHeight(kHeaderHeight));

    drawNotes    (g, content);
    drawPlayhead (g, content);

    // Analysis placeholder when no notes
    if (!m_notes || m_notes->empty()) {
        g.setColour(juce::Colours::grey.withAlpha(0.6f));
        g.setFont(14.0f);
        g.drawText("Waiting for ARA audio analysis…", content,
                   juce::Justification::centred, false);
    }
}

void NoteCanvas::resized() { repaint(); }

// ─── Interaction ─────────────────────────────────────────────────────────────

void NoteCanvas::mouseDown(const juce::MouseEvent& e)
{
    auto pt = e.position;
    const NoteBlob* hit = hitTest(pt);
    if (!hit) { m_drag.reset(); return; }

    DragState ds;
    ds.noteId        = hit->id;
    ds.origPitchCorr = hit->pitchCorrection;
    ds.origTimeOff   = hit->timeOffset;
    ds.startPos      = pt;
    m_drag = ds;
}

void NoteCanvas::mouseDrag(const juce::MouseEvent& e)
{
    if (!m_drag || !m_notes) return;

    // Find the note we're dragging
    auto it = std::find_if(m_notes->begin(), m_notes->end(),
                           [&](const NoteBlob& n){ return n.id == m_drag->noteId; });
    if (it == m_notes->end()) return;

    float dy = e.position.y - m_drag->startPos.y;
    float dx = e.position.x - m_drag->startPos.x;

    float semH = contentArea().getHeight() / (m_pitchHi - m_pitchLo);
    double deltaPitch = -(double)(dy / semH);  // drag up = higher pitch
    double deltaTime  = (double)(dx) / contentArea().getWidth()
                        * (m_viewEnd - m_viewStart);

    if (m_editCb)
        m_editCb(m_drag->noteId,
                 m_drag->origPitchCorr + deltaPitch,
                 m_drag->origTimeOff   + deltaTime);
}

void NoteCanvas::mouseUp(const juce::MouseEvent&) { m_drag.reset(); }

void NoteCanvas::mouseDoubleClick(const juce::MouseEvent& e)
{
    // Double-click: reset pitch correction on note
    const NoteBlob* hit = hitTest(e.position);
    if (!hit || !m_editCb) return;
    m_editCb(hit->id, 0.0, hit->timeOffset);
}

void NoteCanvas::mouseWheelMove(const juce::MouseEvent&,
                                 const juce::MouseWheelDetails& wheel)
{
    // Horizontal scroll = pan time view
    double span = m_viewEnd - m_viewStart;
    double shift = -wheel.deltaX * span * 0.15;
    m_viewStart += shift;
    m_viewEnd   += shift;
    repaint();
}

void NoteCanvas::timerCallback() { repaint(); }
