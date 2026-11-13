// Minimal VST3 host test: loads WUB4.vst3, checks params/programs/state, renders audio and a UI snapshot.
#include <JuceHeader.h>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 3) { std::puts ("usage: host_test <plugin.vst3> <outdir>"); return 2; }
    const juce::File pluginFile (argv[1]), outDir (argv[2]);
    outDir.createDirectory();

    juce::AudioPluginFormatManager fm;
    fm.addFormat (new juce::VST3PluginFormat());
    juce::OwnedArray<juce::PluginDescription> descs;
    juce::VST3PluginFormat vst3;
    vst3.findAllTypesForFile (descs, pluginFile.getFullPathName());
    if (descs.isEmpty()) { std::puts ("FAIL: no plugin found"); return 1; }
    auto& d = *descs[0];
    std::printf ("Found: %s by %s, instrument=%d, uid=%x\n", d.name.toRawUTF8(), d.manufacturerName.toRawUTF8(), (int) d.isInstrument, d.uniqueId);

    juce::String err;
    const double sr = 48000.0; const int block = 480;
    auto inst = fm.createPluginInstance (d, sr, block, err);
    if (! inst) { std::printf ("FAIL: %s\n", err.toRawUTF8()); return 1; }
    std::printf ("Params: %d  Programs: %d\n", inst->getParameters().size(), inst->getNumPrograms());
    for (int i = 0; i < inst->getNumPrograms(); ++i) std::printf ("  program %d: %s\n", i, inst->getProgramName (i).toRawUTF8());

    inst->setPlayConfigDetails (0, 2, sr, block);
    inst->prepareToPlay (sr, block);

    int failures = 0;
    for (int prog : { 0, 1, 3, 6 })
    {
        inst->setCurrentProgram (prog);
        juce::AudioBuffer<float> buf (2, block);
        double sumSq = 0; float peak = 0; bool bad = false;
        const int blocks = (int) (2.0 * sr / block);
        for (int b = 0; b < blocks; ++b)
        {
            juce::MidiBuffer midi;
            if (b == 2)  midi.addEvent (juce::MidiMessage::noteOn (1, 29, (juce::uint8) 110), 17);
            if (b == 70) midi.addEvent (juce::MidiMessage::noteOff (1, 29), 3);
            if (b == 80) midi.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 110), 0);
            if (b == 150) midi.addEvent (juce::MidiMessage::allNotesOff (1), 0);
            buf.clear();
            inst->processBlock (buf, midi);
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < block; ++i)
                {
                    const float v = buf.getSample (c, i);
                    if (! std::isfinite (v)) bad = true;
                    peak = std::max (peak, std::abs (v)); sumSq += v * v;
                }
        }
        const double rms = std::sqrt (sumSq / (2.0 * blocks * block));
        std::printf ("program %d '%s': peak %.3f rms %.4f %s\n", prog, inst->getProgramName (prog).toRawUTF8(), peak, rms, bad ? "NaN!" : "ok");
        if (bad || peak < 0.05f || peak > 1.0f) ++failures;
    }

    // state round trip
    inst->setCurrentProgram (2);
    juce::MemoryBlock state;
    inst->getStateInformation (state);
    auto* p0 = inst->getParameters()[1];
    const float before = p0->getValue();
    p0->setValueNotifyingHost (before > 0.5f ? 0.1f : 0.9f);
    inst->setStateInformation (state.getData(), (int) state.getSize());
    const float after = p0->getValue();
    std::printf ("state round-trip: %s (%.3f -> %.3f)\n", std::abs (before - after) < 1e-4f ? "ok" : "FAIL", before, after);
    if (std::abs (before - after) >= 1e-4f) ++failures;

    // editor snapshot
    inst->setCurrentProgram (0);
    if (auto* ed = inst->createEditorIfNeeded())
    {
        std::printf ("editor %dx%d\n", ed->getWidth(), ed->getHeight());
        // the VST3 editor is wrapped; paint whatever we got
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::PNGImageFormat png;
        auto f = outDir.getChildFile ("editor.png");
        f.deleteFile();
        juce::FileOutputStream os (f);
        png.writeImageToStream (img, os);
        delete ed;
    }
    inst->releaseResources();
    inst.reset();
    std::printf (failures == 0 ? "ALL PASSED\n" : "FAILURES: %d\n", failures);
    return failures;
}
