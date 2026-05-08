#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
// Colour palette
// ─────────────────────────────────────────────────────────────────────────────
static const juce::Colour kBg       (0xff1a1a1a);
static const juce::Colour kPanelBg  (0xff242424);
static const juce::Colour kParamRow0(0xff202020);
static const juce::Colour kParamRow1(0xff252525);
static const juce::Colour kLcdBg    (0xff081208);
static const juce::Colour kLcdGreen (0xff22ee55);
static const juce::Colour kSonyRed  (0xffcc1111);
static const juce::Colour kBorder   (0xff3c3c3c);
static const juce::Colour kText     (0xffcccccc);
static const juce::Colour kDimText  (0xff777777);
static const juce::Colour kBtnBg    (0xff303030);
static const juce::Colour kBtnText  (0xffeeeeee);
static const juce::Colour kTagColor (0xff4488cc);
static const juce::Colour kGreen    (0xff448844);

// ─────────────────────────────────────────────────────────────────────────────
// DpsLookAndFeel
// ─────────────────────────────────────────────────────────────────────────────
DpsLookAndFeel::DpsLookAndFeel()
{
    setColour(juce::Slider::backgroundColourId,        juce::Colour(0xff2e2e2e));
    setColour(juce::Slider::thumbColourId,             kSonyRed);
    setColour(juce::Slider::trackColourId,             juce::Colour(0xff882222));
    setColour(juce::Slider::textBoxTextColourId,       kText);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1c1c1c));
    setColour(juce::Slider::textBoxOutlineColourId,    kBorder);

    setColour(juce::TextButton::buttonColourId,   kBtnBg);
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff6a1a1a));
    setColour(juce::TextButton::textColourOffId,  kBtnText);
    setColour(juce::TextButton::textColourOnId,   juce::Colours::white);

    setColour(juce::ToggleButton::textColourId,        kText);
    setColour(juce::ToggleButton::tickColourId,        kSonyRed);
    setColour(juce::ToggleButton::tickDisabledColourId,kDimText);

    setColour(juce::ComboBox::backgroundColourId,             juce::Colour(0xff2a2a2a));
    setColour(juce::ComboBox::textColourId,                   kText);
    setColour(juce::ComboBox::outlineColourId,                kBorder);
    setColour(juce::ComboBox::arrowColourId,                  kText);
    setColour(juce::ComboBox::focusedOutlineColourId,         juce::Colour(0xff555555));
    setColour(juce::PopupMenu::backgroundColourId,            juce::Colour(0xff2a2a2a));
    setColour(juce::PopupMenu::textColourId,                  kText);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff882222));
    setColour(juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);

    setColour(juce::Label::textColourId,       kText);
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
}

// ─────────────────────────────────────────────────────────────────────────────
// Range parser — converts a DpsParam range string into slider min/max or
// enum options for a combo box.
// ─────────────────────────────────────────────────────────────────────────────
struct ParamSpec {
    bool              isEnum = false;
    int               minVal = 0;
    int               maxVal = 100;
    juce::StringArray items;  // populated when isEnum == true
};

static ParamSpec parseRange(const char* rangeStr)
{
    juce::String r(rangeStr);
    ParamSpec ps;

    const bool hasComma = r.containsChar(',');
    const bool hasSlash = r.containsChar('/') && !r.contains("Hz") && !r.contains("kHz");
    const auto c0 = r.isEmpty() ? ' ' : r[0];
    const bool startsAlpha = (c0 >= 'A' && c0 <= 'Z') || (c0 >= 'a' && c0 <= 'z');

    // ── Enum: alpha start + comma/slash, or digit tokens with only commas ─────
    if ((startsAlpha || !r.containsChar('-')) && (hasComma || hasSlash))
    {
        ps.isEnum = true;
        const juce::String sep = hasSlash ? "/" : ",";
        auto tokens = juce::StringArray::fromTokens(r, sep, "");
        for (auto& t : tokens) { auto s = t.trim(); if (s.isNotEmpty()) ps.items.add(s); }
        return ps;
    }

    // ── Special: pan L180-R180 ────────────────────────────────────────────────
    if (r.startsWith("L") && r.contains("R"))
    {
        ps.minVal = -180; ps.maxVal = 180;
        return ps;
    }

    // ── Special: contains "Thru" or infinity/arrow chars → raw 0-100 ─────────
    if (r.contains("Thru") || r.containsChar(juce::juce_wchar(0x221e)) /* ∞ */
                            || r.containsChar(juce::juce_wchar(0x2192)) /* → */)
        return ps;  // defaults: min=0 max=100

    // ── Numeric range: scan for first and last numbers ────────────────────────
    juce::Array<double> nums;
    int i = 0, len = r.length();

    while (i < len)
    {
        const auto ch = r[i];
        const bool prevDigit = (i > 0 && juce::CharacterFunctions::isDigit(r[i - 1]));

        if ((ch == '-' || ch == '+') && !prevDigit
            && i + 1 < len && juce::CharacterFunctions::isDigit(r[i + 1]))
        {
            int j = i + 1;
            while (j < len && (juce::CharacterFunctions::isDigit(r[j]) || r[j] == '.')) j++;
            nums.add(r.substring(i, j).getDoubleValue());
            i = j;
        }
        else if (juce::CharacterFunctions::isDigit(ch))
        {
            int j = i;
            while (j < len && (juce::CharacterFunctions::isDigit(r[j]) || r[j] == '.')) j++;
            nums.add(r.substring(i, j).getDoubleValue());
            i = j;
        }
        else ++i;
    }

    if (nums.size() >= 2)
    {
        ps.minVal = (int)std::round(nums.getFirst());
        ps.maxVal = (int)std::round(nums.getLast());
    }
    else if (nums.size() == 1)
    {
        ps.minVal = 0;
        ps.maxVal = (int)std::round(nums[0]);
    }

    if (ps.maxVal <= ps.minVal)
        ps.maxVal = ps.minVal + 100;

    return ps;
}

// ─────────────────────────────────────────────────────────────────────────────
// EffectPanel
// ─────────────────────────────────────────────────────────────────────────────
EffectPanel::EffectPanel()
{
    for (int i = 0; i < kMaxP; i++)
    {
        addAndMakeVisible(nameLabels[i]);
        addAndMakeVisible(sliders[i]);
        addAndMakeVisible(combos[i]);

        nameLabels[i].setVisible(false);
        nameLabels[i].setFont(juce::Font(juce::FontOptions().withHeight(10.5f)));
        nameLabels[i].setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

        sliders[i].setVisible(false);
        sliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        sliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 46, kRowH - 6);

        combos[i].setVisible(false);
    }
}

void EffectPanel::init(int blockNumber, SendParamFn fn)
{
    fxBlock = blockNumber;
    sendFn  = std::move(fn);
}

void EffectPanel::setEffect(int effectNumber)
{
    fxNum     = juce::jlimit(1, 45, effectNumber);
    numParams = kEffects[fxNum].paramCount;

    for (int i = 0; i < kMaxP; i++)
    {
        const bool active = (i < numParams);
        nameLabels[i].setVisible(active);

        if (!active)
        {
            sliders[i].setVisible(false);
            combos[i].setVisible(false);
            continue;
        }

        const auto& p  = kEffects[fxNum].params[i];
        const auto  ps = parseRange(p.range);
        rowIsCombo[i]  = ps.isEnum;

        nameLabels[i].setText(p.name, juce::dontSendNotification);

        if (ps.isEnum)
        {
            sliders[i].setVisible(false);
            combos[i].setVisible(true);
            combos[i].clear(juce::dontSendNotification);
            for (int k = 0; k < ps.items.size(); k++)
                combos[i].addItem(ps.items[k], k + 1);
            combos[i].setSelectedItemIndex(0, juce::dontSendNotification);
            combos[i].onChange = [this, i]
            {
                if (sendFn) sendFn(fxBlock, i, combos[i].getSelectedItemIndex());
            };
        }
        else
        {
            combos[i].setVisible(false);
            sliders[i].setVisible(true);
            sliders[i].setRange(ps.minVal, ps.maxVal, 1.0);
            sliders[i].setValue(ps.minVal, juce::dontSendNotification);
            sliders[i].onValueChange = [this, i]
            {
                if (sendFn) sendFn(fxBlock, i, (int)sliders[i].getValue());
            };
        }
    }

    resized();
    repaint();
}

void EffectPanel::resized()
{
    const int W     = getWidth();
    const int nameW = 108;
    const int xNum  = 7;
    const int xName = xNum + 18;
    const int xCtrl = xName + nameW;
    const int wCtrl = W - xCtrl - 8;

    for (int i = 0; i < kMaxP; i++)
    {
        if (!nameLabels[i].isVisible()) continue;

        const int y = kHdrH + i * kRowH;
        nameLabels[i].setBounds(xName, y + 3, nameW, kRowH - 6);

        if (rowIsCombo[i])
            combos[i].setBounds(xCtrl, y + 2, wCtrl, kRowH - 4);
        else
            sliders[i].setBounds(xCtrl, y + 3, wCtrl, kRowH - 6);
    }
}

void EffectPanel::paint(juce::Graphics& g)
{
    const auto& e = kEffects[fxNum];
    const int   W = getWidth();

    g.setColour(kPanelBg);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
    g.setColour(kBorder);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);

    // Effect number badge
    g.setColour(kTagColor);
    g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f).withStyle("Bold")));
    g.drawText(juce::String::formatted("%02d", e.number), 7, 5, 22, 14,
               juce::Justification::left);

    // TAP badge
    if (e.tap)
    {
        g.setColour(kGreen);
        g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
        g.drawText("TAP", W - 32, 5, 26, 14, juce::Justification::right);
    }

    // Type badge (4ch / 2ch / M-P)
    g.setColour(kDimText);
    g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
    g.drawText(e.type, W - 32 - (e.tap ? 28 : 0), 5, 28, 14,
               juce::Justification::right);

    // Effect full name
    g.setColour(kText);
    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f).withStyle("Bold")));
    g.drawText(e.name, 7, 20, W - 14, 16, juce::Justification::left);

    // Separator
    g.setColour(kBorder);
    g.drawLine(7.0f, (float)kHdrH, (float)(W - 7), (float)kHdrH, 1.0f);

    // Alternating row backgrounds + param number labels
    for (int i = 0; i < kMaxP; i++)
    {
        const int y = kHdrH + i * kRowH;
        g.setColour(i % 2 == 0 ? kParamRow0 : kParamRow1);
        g.fillRect(7, y, W - 14, kRowH - 1);

        if (i < e.paramCount)
        {
            g.setColour(kDimText);
            g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
            g.drawText(juce::String(i + 1), 9, y + 4, 14, kRowH - 8,
                       juce::Justification::left);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DpsV55Editor
// ─────────────────────────────────────────────────────────────────────────────
DpsV55Editor::DpsV55Editor(DpsV55Processor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&laf);
    setSize(700, 680);

    // ── MIDI Channel ──────────────────────────────────────────────────────────
    channelLabel.setText("MIDI CH:", juce::dontSendNotification);
    channelLabel.setFont(juce::Font(juce::FontOptions().withHeight(13.0f).withStyle("Bold")));
    addAndMakeVisible(channelLabel);

    for (int ch = 1; ch <= 16; ++ch)
        channelBox.addItem(juce::String(ch), ch);
    channelBox.setSelectedId(proc.midiChannel.load(), juce::dontSendNotification);
    channelBox.onChange = [this]
    {
        proc.midiChannel.store(channelBox.getSelectedId());
        statusLabel.setText("MIDI channel: " + juce::String(channelBox.getSelectedId()),
                            juce::dontSendNotification);
    };
    addAndMakeVisible(channelBox);

    // ── Program Select ────────────────────────────────────────────────────────
    progSectionLabel.setText("PROGRAM SELECT", juce::dontSendNotification);
    progSectionLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
    progSectionLabel.setColour(juce::Label::textColourId, kDimText);
    addAndMakeVisible(progSectionLabel);

    lcdDisplay.setFont(juce::Font(juce::FontOptions()
        .withName(juce::Font::getDefaultMonospacedFontName()).withHeight(16.0f)));
    lcdDisplay.setColour(juce::Label::textColourId,       kLcdGreen);
    lcdDisplay.setColour(juce::Label::backgroundColourId, kLcdBg);
    lcdDisplay.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(lcdDisplay);

    prevBtn.onClick = [this]
    {
        const int cur = proc.programNumber.load();
        if (cur > 0)
        {
            proc.programNumber.store(cur - 1);
            progSlider.setValue(cur - 1, juce::dontSendNotification);
            syncFromProcessor();
            if (autoSendBtn.getToggleState()) triggerProgramChange();
        }
    };
    addAndMakeVisible(prevBtn);

    nextBtn.onClick = [this]
    {
        const int cur = proc.programNumber.load();
        if (cur < 127)
        {
            proc.programNumber.store(cur + 1);
            progSlider.setValue(cur + 1, juce::dontSendNotification);
            syncFromProcessor();
            if (autoSendBtn.getToggleState()) triggerProgramChange();
        }
    };
    addAndMakeVisible(nextBtn);

    progSlider.setRange(0, 127, 1);
    progSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    progSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    progSlider.setValue(proc.programNumber.load(), juce::dontSendNotification);
    progSlider.onValueChange = [this]
    {
        proc.programNumber.store((int)progSlider.getValue());
        syncFromProcessor();
        if (autoSendBtn.getToggleState()) autoSendCountdown = 3;
    };
    addAndMakeVisible(progSlider);

    sendPCBtn.onClick = [this] { triggerProgramChange(); };
    addAndMakeVisible(sendPCBtn);

    autoSendBtn.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(autoSendBtn);

    // ── Master Volume ─────────────────────────────────────────────────────────
    volSectionLabel.setText("MASTER VOLUME  (CC #7)", juce::dontSendNotification);
    volSectionLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
    volSectionLabel.setColour(juce::Label::textColourId, kDimText);
    addAndMakeVisible(volSectionLabel);

    volSlider.setRange(0, 127, 1);
    volSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volSlider.setValue(proc.masterVolume.load(), juce::dontSendNotification);
    volSlider.onValueChange = [this]
    {
        proc.masterVolume.store((int)volSlider.getValue());
        volValueLabel.setText(juce::String((int)volSlider.getValue()),
                              juce::dontSendNotification);
    };
    addAndMakeVisible(volSlider);

    volValueLabel.setFont(juce::Font(juce::FontOptions()
        .withName(juce::Font::getDefaultMonospacedFontName()).withHeight(14.0f)));
    volValueLabel.setJustificationType(juce::Justification::centred);
    volValueLabel.setText(juce::String(proc.masterVolume.load()),
                          juce::dontSendNotification);
    addAndMakeVisible(volValueLabel);

    sendVolBtn.onClick = [this] { triggerVolumeChange(); };
    addAndMakeVisible(sendVolBtn);

    // ── Effect Control ────────────────────────────────────────────────────────
    fxSectionLabel.setText("EFFECT CONTROL  (SysEx)", juce::dontSendNotification);
    fxSectionLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
    fxSectionLabel.setColour(juce::Label::textColourId, kDimText);
    addAndMakeVisible(fxSectionLabel);

    // Structure
    structureLabel.setText("Structure:", juce::dontSendNotification);
    structureLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    addAndMakeVisible(structureLabel);

    structureCombo.addItem("Parallel", 1);
    structureCombo.addItem("Serial",   2);
    structureCombo.setSelectedId(proc.fxStructure.load() + 1, juce::dontSendNotification);
    structureCombo.onChange = [this]
    {
        const int val = structureCombo.getSelectedId() - 1;  // 0=parallel, 1=serial
        proc.fxStructure.store(val);
        proc.queueParamChange(2, 0, val);
        statusLabel.setText("Sent: Structure = " + structureCombo.getText(),
                            juce::dontSendNotification);
    };
    addAndMakeVisible(structureCombo);

    // FxA label + combo + ON button
    fxaLabel.setText("FxA:", juce::dontSendNotification);
    fxaLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
    fxaLabel.setColour(juce::Label::textColourId, kTagColor);
    addAndMakeVisible(fxaLabel);

    for (int i = 1; i <= 45; ++i)
        fxaCombo.addItem(juce::String::formatted("%02d  %s", i, kEffects[i].name), i);
    fxaCombo.setSelectedId(proc.fxAType.load(), juce::dontSendNotification);
    fxaCombo.onChange = [this]
    {
        const int sel = fxaCombo.getSelectedId();
        proc.fxAType.store(sel);
        fxaPanel.setEffect(sel);
        proc.queueParamChange(2, 1, sel);  // block 2, param 1 = FxA type
        statusLabel.setText("Sent: FxA type → " + juce::String(kEffects[sel].name),
                            juce::dontSendNotification);
    };
    addAndMakeVisible(fxaCombo);

    fxaOnOffBtn.setClickingTogglesState(true);
    fxaOnOffBtn.setToggleState(proc.fxAOnOff.load() == 1, juce::dontSendNotification);
    fxaOnOffBtn.setColour(juce::TextButton::buttonOnColourId, kGreen);
    fxaOnOffBtn.onClick = [this]
    {
        const int val = fxaOnOffBtn.getToggleState() ? 1 : 0;
        proc.fxAOnOff.store(val);
        proc.queueParamChange(2, 2, val);  // block 2, param 2 = FxA on/off
        fxaOnOffBtn.setButtonText(val ? "ON" : "OFF");
        statusLabel.setText("Sent: FxA " + juce::String(val ? "On" : "Off"),
                            juce::dontSendNotification);
    };
    addAndMakeVisible(fxaOnOffBtn);

    // FxB label + combo + ON button
    fxbLabel.setText("FxB:", juce::dontSendNotification);
    fxbLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
    fxbLabel.setColour(juce::Label::textColourId, kTagColor);
    addAndMakeVisible(fxbLabel);

    for (int i = 10; i <= 45; ++i)
        fxbCombo.addItem(juce::String::formatted("%02d  %s", i, kEffects[i].name), i);
    fxbCombo.setSelectedId(proc.fxBType.load(), juce::dontSendNotification);
    fxbCombo.onChange = [this]
    {
        const int sel = fxbCombo.getSelectedId();
        proc.fxBType.store(sel);
        fxbPanel.setEffect(sel);
        proc.queueParamChange(2, 3, sel);  // block 2, param 3 = FxB type
        statusLabel.setText("Sent: FxB type → " + juce::String(kEffects[sel].name),
                            juce::dontSendNotification);
    };
    addAndMakeVisible(fxbCombo);

    fxbOnOffBtn.setClickingTogglesState(true);
    fxbOnOffBtn.setToggleState(proc.fxBOnOff.load() == 1, juce::dontSendNotification);
    fxbOnOffBtn.setColour(juce::TextButton::buttonOnColourId, kGreen);
    fxbOnOffBtn.onClick = [this]
    {
        const int val = fxbOnOffBtn.getToggleState() ? 1 : 0;
        proc.fxBOnOff.store(val);
        proc.queueParamChange(2, 4, val);  // block 2, param 4 = FxB on/off
        fxbOnOffBtn.setButtonText(val ? "ON" : "OFF");
        statusLabel.setText("Sent: FxB " + juce::String(val ? "On" : "Off"),
                            juce::dontSendNotification);
    };
    addAndMakeVisible(fxbOnOffBtn);

    // ── Effect panels with parameter sliders ──────────────────────────────────
    fxaPanel.init(0, [this](int block, int param, int value)
    {
        proc.queueParamChange(block, param, value);
        statusLabel.setText("Sent: FxA param " + juce::String(param + 1)
                            + " = " + juce::String(value),
                            juce::dontSendNotification);
    });
    fxaPanel.setEffect(proc.fxAType.load());
    addAndMakeVisible(fxaPanel);

    fxbPanel.init(1, [this](int block, int param, int value)
    {
        proc.queueParamChange(block, param, value);
        statusLabel.setText("Sent: FxB param " + juce::String(param + 1)
                            + " = " + juce::String(value),
                            juce::dontSendNotification);
    });
    fxbPanel.setEffect(proc.fxBType.load());
    addAndMakeVisible(fxbPanel);

    // ── Utility ───────────────────────────────────────────────────────────────
    allNotesOffBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff4a1010));
    allNotesOffBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff8888));
    allNotesOffBtn.onClick = [this]
    {
        proc.pendingAllNotesOff.store(true);
        statusLabel.setText("Sent: All Notes Off", juce::dontSendNotification);
    };
    addAndMakeVisible(allNotesOffBtn);

    statusLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    statusLabel.setColour(juce::Label::textColourId, kDimText);
    statusLabel.setText("Ready", juce::dontSendNotification);
    addAndMakeVisible(statusLabel);

    syncFromProcessor();
    startTimerHz(10);
}

DpsV55Editor::~DpsV55Editor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
void DpsV55Editor::triggerProgramChange()
{
    proc.pendingProgramChange.store(true);
    const int pc  = proc.programNumber.load();
    const int dev = pc + 1;
    statusLabel.setText("Sent: PC " + juce::String(pc).paddedLeft('0', 3)
                        + "  →  Device Prg " + juce::String(dev).paddedLeft('0', 3),
                        juce::dontSendNotification);
}

void DpsV55Editor::triggerVolumeChange()
{
    proc.pendingVolumeChange.store(true);
    statusLabel.setText("Sent: CC#7 Volume = " + juce::String(proc.masterVolume.load()),
                        juce::dontSendNotification);
}

void DpsV55Editor::syncFromProcessor()
{
    const int pc  = proc.programNumber.load();
    const int dev = pc + 1;
    lcdDisplay.setText("PC " + juce::String(pc).paddedLeft('0', 3)
                       + "   Prg " + juce::String(dev).paddedLeft('0', 3),
                       juce::dontSendNotification);
}

void DpsV55Editor::timerCallback()
{
    if (autoSendCountdown > 0)
    {
        --autoSendCountdown;
        if (autoSendCountdown == 0)
            triggerProgramChange();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Paint — structural chrome only; components draw themselves
// ─────────────────────────────────────────────────────────────────────────────
void DpsV55Editor::paint(juce::Graphics& g)
{
    const int W = getWidth();
    g.fillAll(kBg);

    // ── Header ────────────────────────────────────────────────────────────────
    g.setColour(juce::Colour(0xff121212));
    g.fillRect(0, 0, W, 56);
    g.setColour(kSonyRed);
    g.fillRect(0, 0, 6, 56);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions().withHeight(22.0f).withStyle("Bold")));
    g.drawText("SONY", 18, 6, 80, 26, juce::Justification::left);

    g.setColour(kSonyRed);
    g.setFont(juce::Font(juce::FontOptions().withHeight(15.0f).withStyle("Bold")));
    g.drawText("DPS-V55M", 18, 30, 130, 20, juce::Justification::left);

    g.setColour(kDimText);
    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    g.drawText("Multi-Effect Processor  //  MIDI Controller",
               160, 0, W - 170, 56, juce::Justification::centredLeft);

    g.setColour(kBorder);
    g.drawLine(0.0f, 55.5f, (float)W, 55.5f, 1.0f);

    // ── Section panels (background) ───────────────────────────────────────────
    auto drawPanel = [&](juce::Rectangle<int> r)
    {
        g.setColour(kPanelBg);
        g.fillRoundedRectangle(r.toFloat(), 4.0f);
        g.setColour(kBorder);
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 4.0f, 1.0f);
    };

    drawPanel({ 10, 62,  W - 20, 38  });   // MIDI channel
    drawPanel({ 10, 108, W - 20, 130 });   // Program
    drawPanel({ 10, 248, W - 20, 70  });   // Volume
    // Effect section label + structure row
    drawPanel({ 10, 326, W - 20, 46  });   // Effect header panel
    // EffectPanel components draw their own panel backgrounds
}

// ─────────────────────────────────────────────────────────────────────────────
// Layout
// ─────────────────────────────────────────────────────────────────────────────
void DpsV55Editor::resized()
{
    const int W   = getWidth();
    const int pad = 16;

    // ── MIDI Channel (y=62, h=38) ─────────────────────────────────────────────
    channelLabel.setBounds(pad + 4, 70, 72, 22);
    channelBox.setBounds(pad + 78,  70, 70, 22);

    // ── Program Select (y=108, h=130) ────────────────────────────────────────
    progSectionLabel.setBounds(pad + 4, 110, 200, 16);
    const int lcdY = 128;
    prevBtn.setBounds(pad + 4,      lcdY, 30, 36);
    lcdDisplay.setBounds(pad + 38,  lcdY, W - 2*pad - 76, 36);
    nextBtn.setBounds(W - pad - 34, lcdY, 30, 36);
    progSlider.setBounds(pad + 4,   lcdY + 42, W - 2*pad - 8, 22);
    sendPCBtn.setBounds(pad + 4,    lcdY + 70, 220, 28);
    autoSendBtn.setBounds(pad + 232,lcdY + 70, 120, 28);

    // ── Volume (y=248, h=70) ──────────────────────────────────────────────────
    volSectionLabel.setBounds(pad + 4, 251, 220, 16);
    const int volY = 269;
    volSlider.setBounds(pad + 4,          volY, W - 2*pad - 100, 28);
    volValueLabel.setBounds(W - pad - 90, volY, 42, 28);
    sendVolBtn.setBounds(W - pad - 44,    volY, 44, 28);

    // ── Effect Control (y=326) ────────────────────────────────────────────────
    fxSectionLabel.setBounds(pad + 4, 330, 220, 16);

    // Structure row (inside the 46px header panel)
    structureLabel.setBounds(W - pad - 160, 330, 72, 16);
    structureCombo.setBounds(W - pad - 86,  328, 86, 20);

    // FxA/FxB selector row (y=376)
    const int fxRowY  = 376;
    const int halfW   = (W - 2*pad - 10) / 2;
    const int onW     = 34;
    const int comboW  = halfW - 40 - onW - 4;

    fxaLabel.setBounds(pad + 4,           fxRowY, 36, 22);
    fxaCombo.setBounds(pad + 42,          fxRowY, comboW, 22);
    fxaOnOffBtn.setBounds(pad + 44 + comboW, fxRowY, onW, 22);

    fxbLabel.setBounds(pad + 10 + halfW,           fxRowY, 36, 22);
    fxbCombo.setBounds(pad + 48 + halfW,           fxRowY, comboW, 22);
    fxbOnOffBtn.setBounds(pad + 50 + halfW + comboW, fxRowY, onW, 22);

    // Effect panels (y=402)
    const int panelY = fxRowY + 26;
    const int panelH = getHeight() - panelY - 34;

    fxaPanel.setBounds(pad + 4,          panelY, halfW, panelH);
    fxbPanel.setBounds(pad + 14 + halfW, panelY, halfW, panelH);

    // ── Utility (bottom) ─────────────────────────────────────────────────────
    const int utilY = getHeight() - 28;
    allNotesOffBtn.setBounds(pad, utilY, 160, 22);
    statusLabel.setBounds(pad + 168, utilY, W - pad - 178, 22);
}
