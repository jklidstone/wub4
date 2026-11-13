# WUB4 — riddim bass synth (VST3 / AU)

A mono riddim bass synth. Pick a preset, play notes around **F1 to F2**, and turn the knobs.

It is built around an original **SQR\*4** table: a hard-synced square that morphs from 1× to 4× as you turn Position. It's a nod to the SQUARE\*4 graintable in Reason's Malström. Malström is where the "Square4" riddim sound comes from; it's not a Serum wavetable. No Reason content is used.

## Signal flow

```
 OSC (wavetable + FM "Growl", 1-5 unison)            SUB (clean mono sine)
   -> FILTER (LP / BP / Notch / Comb+ / Comb- / Vowel)       |
   -> DIRT   (Tube / Clip / Fold / Crush)                    |
   -> AMP env x WUB volume gate                              |
   -> LOW CUT (mid bass only) -> OTT -> SPACE --------------(+)--> safety clipper -> out
        ^ all of the above runs at 2x oversampling

 WUB LFO: tempo-synced (incl. 1/8T, 1/16T), restarts every note, 7 shapes,
          one depth knob each for Table / Growl / Filter / Volume / Pitch
```

## Quick tips
- **The classic riddim move:** Wub Rate **1/8T**, Shape **Saw Down**, then turn up **Wub > Table**.
- **Metallic "Square4" sound:** Filter **Comb +**, Key Track **100%**, Reso **70-85%**, Cutoff about 100-200 Hz.
- **Yoi / talking bass:** use the **Yoi Saw** table with Wub > Table, or the **Vowel** filter with Wub > Filter. Crush Dirt at low mix gives Steve Duda's "sample-rate reduction" vowel trick.
- **Gating chops:** Shape **Square**, Wub > Volume up. The sub follows the gate too.
- The sub never goes through Dirt or OTT. **Low Cut** (default 90 Hz) keeps the mid bass out of its way.
- Double-click any knob to reset it. Hover over a knob for a tooltip.

## Presets
SQR4 Classic, Square4 Metal, Wonky Wub, Yoi Talker, FM Grind, Robot Riddim, Triplet Gate, Sync Screech, Comb Minus Hollow, Drop Dive, Deep Square (Clean), Init.

## Install (macOS)
1. Download the **WUB4-macOS** artifact from the latest GitHub Actions run (or a Release) and unzip it.
2. Right-click **install-mac.command** → **Open**. You need right-click the first time because the script isn't notarized. The script copies the plugin into `~/Library/Audio/Plug-Ins/VST3` and clears the download quarantine.
3. In Ableton Live, open **Settings → Plug-Ins**, make sure **Use VST3 Plug-in System Folders** is on, then click **Rescan**.

The plugin is ad-hoc signed, not notarized. If macOS still blocks it, run:
`xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/WUB4.vst3`

## Build from source
```
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release --target WUB4_VST3
```
CMake downloads JUCE 8 automatically. You can pass `-DJUCE_DIR=/path/to/JUCE` to use a local copy.

`tools/render_test.cpp` renders every preset offline to WAV, with no JUCE needed:
`g++ -std=c++17 -O2 tools/render_test.cpp -o render_test && ./render_test out/`

## License note
WUB4 uses JUCE 8. Personal and free projects can use JUCE under its AGPLv3 option or the free JUCE Starter licence. Check the JUCE licence terms before selling it.
