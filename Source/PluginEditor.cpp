#include "PluginEditor.h"

using namespace wub;

// ============================================================== Look and feel
Wub4LookAndFeel::Wub4LookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, ui::panelHi);
    setColour (juce::ComboBox::textColourId, ui::text);
    setColour (juce::ComboBox::outlineColourId, ui::panelHi.brighter (0.15f));
    setColour (juce::ComboBox::arrowColourId, ui::accent);
    setColour (juce::PopupMenu::backgroundColourId, ui::panel);
    setColour (juce::PopupMenu::textColourId, ui::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::accent.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, ui::text);
    setColour (juce::Label::textColourId, ui::text);
    setColour (juce::TextButton::buttonColourId, ui::panelHi);
    setColour (juce::TextButton::textColourOffId, ui::text);
    setColour (juce::TooltipWindow::backgroundColourId, ui::panelHi);
    setColour (juce::TooltipWindow::textColourId, ui::text);
    setColour (juce::TooltipWindow::outlineColourId, ui::accent.withAlpha (0.5f));
    setColour (juce::BubbleComponent::backgroundColourId, ui::panelHi);
    setColour (juce::BubbleComponent::outlineColourId, ui::accent.withAlpha (0.6f));
    setColour (juce::Slider::textBoxTextColourId, ui::text);
}

void Wub4LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float start, float end, juce::Slider& s)
{
    const auto colour = s.findColour (juce::Slider::rotarySliderFillColourId);
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float trackW = juce::jmax (3.0f, radius * 0.14f);
    const float arcR = radius - trackW * 0.5f;

    // body
    g.setColour (ui::bg);
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
    g.setColour (ui::panelHi.brighter (0.05f));
    g.fillEllipse (juce::Rectangle<float> (radius * 1.5f, radius * 1.5f).withCentre (centre));

    // track
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, start, end, true);
    g.setColour (ui::panelHi.brighter (0.2f));
    g.strokePath (track, juce::PathStrokeType (trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc (bipolar knobs fill from the centre)
    const bool bipolar = s.getMinimum() < 0.0;
    const float angle = start + pos * (end - start);
    const float from = bipolar ? (start + end) * 0.5f : start;
    if (std::abs (angle - from) > 0.001f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour (colour.withAlpha (0.25f));
        g.strokePath (value, juce::PathStrokeType (trackW * 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (colour);
        g.strokePath (value, juce::PathStrokeType (trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // pointer
    juce::Path ptr;
    const float pl = radius * 0.5f;
    ptr.addRoundedRectangle (-1.5f, -radius * 0.72f, 3.0f, pl, 1.5f);
    g.setColour (ui::text);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (centre));
}

void Wub4LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    juce::Path arrow;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (ui::accent);
    g.fillPath (arrow);
}

void Wub4LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    g.setColour (on ? ui::accent.withAlpha (0.2f) : ui::panelHi);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? ui::accent : ui::panelHi.brighter (0.2f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    g.setColour (on ? ui::accent : ui::dim);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (b.getButtonText().toUpperCase(), r, juce::Justification::centred);
}

void Wub4LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? ui::accent.withAlpha (0.3f) : over ? ui::panelHi.brighter (0.1f) : ui::panelHi);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (ui::panelHi.brighter (0.2f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

// ============================================================== Knob / Choice
static juce::String shortCaption (int i)
{
    switch (i)
    {
        case LfoToTable:  return "TABLE";
        case LfoToGrowl:  return "GROWL";
        case LfoToFilter: return "FILTER";
        case LfoToVolume: return "VOLUME";
        case LfoToPitch:  return "PITCH";
        case SubLevel:    return "LEVEL";
        case DirtMix:     return "MIX";
        case DropTime:    return "DROP TIME";
        default:          return juce::String (kParams[i].name).toUpperCase();
    }
}

Knob::Knob (juce::AudioProcessorValueTreeState& state, int i, juce::Colour colour)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled (true, true, nullptr, 2000);
    slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    slider.setTooltip (kParams[i].help);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible (slider);

    caption.setText (shortCaption (i), juce::dontSendNotification);
    caption.setJustificationType (juce::Justification::centred);
    caption.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    caption.setColour (juce::Label::textColourId, ui::dim);
    caption.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (caption);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, kParams[i].id, slider);
    slider.setDoubleClickReturnValue (true, (double) kParams[i].def);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    caption.setBounds (r.removeFromBottom (16));
    slider.setBounds (r);
}

Choice::Choice (juce::AudioProcessorValueTreeState& state, int i)
{
    for (int c = 0; c < kParams[i].numChoices; ++c) box.addItem (kParams[i].choices[c], c + 1);
    box.setTooltip (kParams[i].help);
    addAndMakeVisible (box);

    caption.setText (shortCaption (i), juce::dontSendNotification);
    caption.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    caption.setColour (juce::Label::textColourId, ui::dim);
    caption.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (caption);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, kParams[i].id, box);
}

void Choice::resized()
{
    auto r = getLocalBounds();
    caption.setBounds (r.removeFromTop (15));
    box.setBounds (r.removeFromTop (26));
}

// ============================================================== Wave display
WaveDisplay::WaveDisplay (Wub4Processor& p) : proc (p) { startTimerHz (30); }

void WaveDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (ui::bg);
    g.fillRoundedRectangle (r, 6.0f);

    // grid
    g.setColour (ui::panelHi);
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * i / 4.0f), r.getX(), r.getRight());
    for (int i = 1; i < 8; ++i) g.drawVerticalLine ((int) (r.getX() + r.getWidth() * i / 8.0f), r.getY(), r.getBottom());

    const int tableIdx = (int) std::round (proc.apvts.getRawParameterValue ("table")->load());
    const float position = proc.synth.displayPosition.load();
    const auto& wt = WavetableBank::get()[tableIdx];

    auto plot = r.reduced (8.0f, 18.0f);
    juce::Path path;
    const int points = juce::jmax (64, (int) plot.getWidth());
    for (int i = 0; i <= points; ++i)
    {
        const double ph = (double) i / points * 0.9999;
        const float v = readTable (wt, 0, position, ph);
        const float x = plot.getX() + plot.getWidth() * (float) i / points;
        const float y = plot.getCentreY() - v * plot.getHeight() * 0.48f;
        if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
    }
    g.setColour (ui::accent.withAlpha (0.18f));
    g.strokePath (path, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (ui::accent);
    g.strokePath (path, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // labels
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.setColour (ui::text);
    g.drawText (kTableNames[juce::jlimit (0, kNumTables - 1, tableIdx)], r.reduced (8.0f, 4.0f), juce::Justification::topLeft);
    g.setColour (ui::dim);
    g.drawText ("POS " + juce::String ((int) std::round (position * 100.0f)) + "%", r.reduced (8.0f, 4.0f), juce::Justification::topRight);

    // wobble meter
    const float lfo = proc.synth.displayLfo.load() * proc.synth.displayEnv.load();
    auto bar = r.reduced (8.0f, 5.0f).removeFromBottom (4.0f);
    g.setColour (ui::panelHi);
    g.fillRoundedRectangle (bar, 2.0f);
    g.setColour (ui::accent2);
    g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * juce::jlimit (0.0f, 1.0f, lfo)), 2.0f);
}

// ============================================================== Editor
Wub4Editor::Wub4Editor (Wub4Processor& p)
    : AudioProcessorEditor (&p), proc (p), display (p)
{
    setLookAndFeel (&lnf);

    for (int i : { (int) Position, (int) Growl, (int) Octave, (int) Unison, (int) Detune, (int) Width }) addKnob (i);
    for (int i : { (int) SubLevel, (int) LowCut }) addKnob (i, ui::accent2);
    for (int i : { (int) LfoToTable, (int) LfoToGrowl, (int) LfoToFilter, (int) LfoToVolume, (int) LfoToPitch }) addKnob (i, ui::accent2);
    for (int i : { (int) Cutoff, (int) Reso, (int) KeyTrack }) addKnob (i);
    for (int i : { (int) Drive, (int) DirtMix }) addKnob (i);
    for (int i : { (int) Ott, (int) Space }) addKnob (i);
    for (int i : { (int) Attack, (int) Decay, (int) Sustain, (int) Release, (int) Glide, (int) DropAmt, (int) DropTime }) addKnob (i, ui::accent2);
    addKnob (Master);

    for (int i : { (int) Table, (int) FmRatio, (int) SubOct, (int) LfoRate, (int) LfoShape, (int) FiltMode, (int) DirtMode }) addChoice (i);

    addAndMakeVisible (display);

    legatoButton.setTooltip (kParams[Legato].help);
    addAndMakeVisible (legatoButton);
    legatoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, kParams[Legato].id, legatoButton);

    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0 && idx != proc.getCurrentProgram()) proc.setCurrentProgram (idx);
    };
    presetBox.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (presetBox);

    auto step = [this] (int delta)
    {
        const int n = proc.getNumPrograms();
        const int idx = (proc.getCurrentProgram() + delta + n) % n;
        proc.setCurrentProgram (idx);
        presetBox.setSelectedItemIndex (idx, juce::dontSendNotification);
    };
    prevButton.onClick = [step] { step (-1); };
    nextButton.onClick = [step] { step (1); };
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);

    setSize (1000, 660);
}

Wub4Editor::~Wub4Editor() { setLookAndFeel (nullptr); }

void Wub4Editor::addKnob (int i, juce::Colour c)
{
    auto k = std::make_unique<Knob> (proc.apvts, i, c);
    addAndMakeVisible (*k);
    knobs[i] = std::move (k);
}

void Wub4Editor::addChoice (int i)
{
    auto c = std::make_unique<Choice> (proc.apvts, i);
    addAndMakeVisible (*c);
    choices[i] = std::move (c);
}

Knob* Wub4Editor::knob (int i)     { return knobs.count (i) ? knobs[i].get() : nullptr; }
Choice* Wub4Editor::choice (int i) { return choices.count (i) ? choices[i].get() : nullptr; }

void Wub4Editor::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    for (int i = 0; i < kNumPresets; ++i) presetBox.addItem (kPresets[i].name, i + 1);
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
}

void Wub4Editor::paint (juce::Graphics& g)
{
    g.fillAll (ui::bg);

    // header
    auto header = getLocalBounds().removeFromTop (64).toFloat();
    juce::ColourGradient grad (ui::accent2.withAlpha (0.20f), 0, 0, ui::bg, 0, header.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRect (header);
    g.setColour (ui::accent);
    g.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    g.drawText ("WUB4", 18, 10, 120, 44, juce::Justification::centredLeft);
    g.setColour (ui::dim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("RIDDIM BASS SYNTH", 126, 26, 160, 20, juce::Justification::centredLeft);

    for (auto& s : sections)
    {
        auto r = s.area.toFloat();
        g.setColour (ui::panel);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (s.colour.withAlpha (0.8f));
        g.fillRoundedRectangle (r.getX() + 12.0f, r.getY() + 9.0f, 3.0f, 12.0f, 1.5f);
        g.setColour (ui::text);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (s.title, (int) r.getX() + 21, (int) r.getY() + 5, 200, 20, juce::Justification::centredLeft);
    }
}

void Wub4Editor::resized()
{
    sections.clear();
    const int knobW = 70, knobH = 84, pad = 12;

    auto placeRow = [&] (juce::Rectangle<int> area, std::initializer_list<int> ids)
    {
        const int n = (int) ids.size();
        const int w = juce::jmin (knobW + 8, area.getWidth() / juce::jmax (1, n));
        auto row = area.withSizeKeepingCentre (w * n, knobH);
        for (int id : ids)
            if (auto* k = knob (id)) k->setBounds (row.removeFromLeft (w).withSizeKeepingCentre (juce::jmin (knobW, w), knobH));
    };

    auto placeChoices = [&] (juce::Rectangle<int>& area, std::initializer_list<std::pair<int, int>> idsAndWidths)
    {
        auto row = area.removeFromTop (44).reduced (pad, 0);
        for (auto [id, w] : idsAndWidths)
            if (auto* c = choice (id)) { c->setBounds (row.removeFromLeft (w)); row.removeFromLeft (10); }
    };

    auto section = [&] (const juce::String& title, juce::Rectangle<int> area, juce::Colour colour)
    {
        sections.push_back ({ title, area, colour });
        auto inner = area.reduced (0, 4);
        inner.removeFromTop (24);
        return inner;
    };

    // header
    presetBox.setBounds (getWidth() / 2 - 130, 18, 260, 28);
    prevButton.setBounds (presetBox.getX() - 34, 18, 28, 28);
    nextButton.setBounds (presetBox.getRight() + 6, 18, 28, 28);
    if (auto* m = knob (Master)) m->setBounds (getWidth() - 86, 2, 66, 62);

    // row 1
    {
        auto osc = section ("OSCILLATOR", { 12, 76, 488, 196 }, ui::accent);
        placeChoices (osc, { { Table, 160 }, { FmRatio, 90 } });
        placeRow (osc, { Position, Growl, Octave, Unison, Detune, Width });

        sections.push_back ({ "", { 512, 76, 290, 196 }, ui::accent });
        display.setBounds (520, 84, 274, 180);

        auto sub = section ("SUB", { 814, 76, 174, 196 }, ui::accent2);
        placeChoices (sub, { { SubOct, 100 } });
        placeRow (sub, { SubLevel, LowCut });
    }
    // row 2
    {
        auto wub = section ("WUB  (tempo-synced wobble)", { 12, 284, 588, 172 }, ui::accent2);
        placeChoices (wub, { { LfoRate, 100 }, { LfoShape, 120 } });
        placeRow (wub, { LfoToTable, LfoToGrowl, LfoToFilter, LfoToVolume, LfoToPitch });

        auto filt = section ("FILTER", { 612, 284, 376, 172 }, ui::accent);
        placeChoices (filt, { { FiltMode, 130 } });
        placeRow (filt, { Cutoff, Reso, KeyTrack });
    }
    // row 3
    {
        auto dirt = section ("DIRT", { 12, 468, 250, 180 }, ui::accent);
        placeChoices (dirt, { { DirtMode, 120 } });
        placeRow (dirt, { Drive, DirtMix });

        auto fx = section ("FX", { 274, 468, 226, 180 }, ui::accent);
        fx.removeFromTop (22);
        placeRow (fx, { Ott, Space });

        auto amp = section ("AMP + PITCH", { 512, 468, 476, 180 }, ui::accent2);
        auto top = amp.removeFromTop (44).reduced (pad, 0);
        legatoButton.setBounds (top.removeFromRight (90).withSizeKeepingCentre (90, 24).translated (0, 6));
        placeRow (amp, { Attack, Decay, Sustain, Release, Glide, DropAmt, DropTime });
    }
    repaint();
}
