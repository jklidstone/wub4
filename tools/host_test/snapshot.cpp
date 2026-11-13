// Renders the WUB4 editor to PNG (one image per requested preset) without a host.
#include <JuceHeader.h>
#include "../../Source/PluginProcessor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File out (argc > 1 ? argv[1] : "snap");
    out.createDirectory();
    Wub4Processor proc;
    proc.prepareToPlay (48000.0, 512);
    for (int prog : { 0, 1, 3 })
    {
        proc.setCurrentProgram (prog);
        // play a little so the display shows a live frame
        juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 29, (juce::uint8) 100), 0);
        for (int i = 0; i < 20; ++i) { buf.clear(); proc.processBlock (buf, midi); midi.clear(); }
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        auto f = out.getChildFile ("editor_" + juce::String (prog) + ".png");
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat().writeImageToStream (img, os);
    }
    std::puts ("ok");
    return 0;
}
