#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace wub;

juce::AudioProcessorValueTreeState::ParameterLayout Wub4Processor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < NumParams; ++i)
    {
        const auto& d = kParams[i];
        const juce::ParameterID pid { d.id, 1 };

        switch (d.kind)
        {
            case Kind::Float:
            {
                juce::NormalisableRange<float> range (d.min, d.max);
                if (d.skewCentre > 0.0f) range.setSkewForCentre (d.skewCentre);
                const juce::String unit (d.unit);
                auto attrs = juce::AudioParameterFloatAttributes()
                                 .withLabel (unit)
                                 .withStringFromValueFunction ([unit, i] (float v, int)
                                 {
                                     if (unit == "Hz") return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String ((int) v) + " Hz";
                                     if (unit == "ms") return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " s" : juce::String (v, v < 10 ? 1 : 0) + " ms";
                                     if (unit == "dB") return juce::String (v, 1) + " dB";
                                     if (unit == "st") return juce::String (v, 1) + " st";
                                     if (kParams[i].min < 0.0f) return juce::String ((int) std::round (v * 100.0f)) + "%";
                                     return juce::String ((int) std::round (v * 100.0f)) + "%";
                                 });
                layout.add (std::make_unique<juce::AudioParameterFloat> (pid, d.name, range, d.def, attrs));
                break;
            }
            case Kind::Choice:
            {
                juce::StringArray choices;
                for (int c = 0; c < d.numChoices; ++c) choices.add (d.choices[c]);
                layout.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, choices, (int) d.def));
                break;
            }
            case Kind::Int:
                layout.add (std::make_unique<juce::AudioParameterInt> (pid, d.name, (int) d.min, (int) d.max, (int) d.def));
                break;
            case Kind::Bool:
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def > 0.5f));
                break;
        }
    }
    return layout;
}

Wub4Processor::Wub4Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "WUB4", createLayout())
{
    for (int i = 0; i < NumParams; ++i)
        raw[(size_t) i] = apvts.getRawParameterValue (kParams[i].id);
}

bool Wub4Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void Wub4Processor::prepareToPlay (double sampleRate, int)
{
    synth.prepare (sampleRate);
}

void Wub4Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                synth.setBpm (*bpm);

    Params p;
    for (int i = 0; i < NumParams; ++i) p[i] = raw[(size_t) i]->load();
    synth.setParams (p);

    const int numSamples = buffer.getNumSamples();
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    // Render into a stereo scratch so a mono bus still works.
    std::array<float, 512> scratchR {};
    int pos = 0;

    auto render = [&] (int upTo)
    {
        while (pos < upTo)
        {
            const int n = std::min (upTo - pos, (int) scratchR.size());
            float* r = R != nullptr ? R + pos : scratchR.data();
            synth.process (L + pos, r, n);
            if (R == nullptr)
                for (int i = 0; i < n; ++i) L[pos + i] = 0.5f * (L[pos + i] + scratchR[(size_t) i]);
            pos += n;
        }
    };

    for (const auto meta : midi)
    {
        render (juce::jlimit (0, numSamples, meta.samplePosition));
        const auto m = meta.getMessage();
        if (m.isNoteOn())                       synth.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())                 synth.noteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) synth.allNotesOff();
    }
    render (numSamples);
}

void Wub4Processor::setCurrentProgram (int index)
{
    index = juce::jlimit (0, kNumPresets - 1, index);
    currentProgram = index;
    const Params values = kPresets[index].toParams();
    for (int i = 0; i < NumParams; ++i)
        if (auto* param = apvts.getParameter (kParams[i].id))
            param->setValueNotifyingHost (param->convertTo0to1 (values[i]));
}

const juce::String Wub4Processor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, kNumPresets) ? juce::String (kPresets[index].name) : juce::String();
}

void Wub4Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void Wub4Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = (int) tree.getProperty ("program", 0);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* Wub4Processor::createEditor() { return new Wub4Editor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Wub4Processor(); }
