#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

namespace ui
{
    const juce::Colour bg       { 0xff0f0d14 };
    const juce::Colour panel    { 0xff1a1622 };
    const juce::Colour panelHi  { 0xff241f30 };
    const juce::Colour accent   { 0xff9dff3a };   // acid green
    const juce::Colour accent2  { 0xffb266ff };   // purple
    const juce::Colour text     { 0xffe8e4f0 };
    const juce::Colour dim      { 0xff8a8399 };
}

class Wub4LookAndFeel : public juce::LookAndFeel_V4
{
public:
    Wub4LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::FontOptions (13.0f, juce::Font::bold); }
    juce::Font getPopupMenuFont() override { return juce::FontOptions (14.0f); }
};

// A knob with its caption, bound to one parameter.
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState&, int paramIndex, juce::Colour colour);
    void resized() override;
    juce::Slider slider;

private:
    juce::Label caption;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// A combo box with its caption, bound to one choice parameter.
class Choice : public juce::Component
{
public:
    Choice (juce::AudioProcessorValueTreeState&, int paramIndex);
    void resized() override;
    juce::ComboBox box;

private:
    juce::Label caption;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

// Shows the wavetable frame currently being played, plus the wobble.
class WaveDisplay : public juce::Component, private juce::Timer
{
public:
    explicit WaveDisplay (Wub4Processor&);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override { repaint(); }
    Wub4Processor& proc;
};

class Wub4Editor : public juce::AudioProcessorEditor
{
public:
    explicit Wub4Editor (Wub4Processor&);
    ~Wub4Editor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void refreshPresetBox();
    void addKnob (int paramIndex, juce::Colour c = ui::accent);
    void addChoice (int paramIndex);
    Knob* knob (int paramIndex);
    Choice* choice (int paramIndex);

    Wub4Processor& proc;
    Wub4LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::ToggleButton legatoButton { "Legato" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> legatoAttachment;

    WaveDisplay display;
    std::map<int, std::unique_ptr<Knob>> knobs;
    std::map<int, std::unique_ptr<Choice>> choices;

    struct Section { juce::String title; juce::Rectangle<int> area; juce::Colour colour; };
    std::vector<Section> sections;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Wub4Editor)
};
