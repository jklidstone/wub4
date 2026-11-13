# CLAUDE.md

WUB4 is a mono riddim bass synth built as a JUCE 8 plugin (VST3, plus AU on macOS). The README covers what it sounds like and how users install it. This file covers how the code is put together and how to work on it.

## Layout

```
Source/dsp/        Header-only DSP core. Plain C++17, no JUCE, namespace `wub`.
  ParamDefs.h      Single source of truth for parameters (enum P + kParams table + Params struct)
  Presets.h        Factory presets (kPresets), expressed as overrides of the defaults
  Synth.h          Mono voice: note stack, envelopes, WUB LFO, signal chain, 2x oversampling
  Wavetables.h     WavetableBank singleton (SQR*4, Metal Square, PWM, Yoi Saw, FM Growl, Sync Saw)
  Effects.h        Svf, Comb, Vowel filter, Dirt, Decimator, OTT (LR4 3-band), Reverb
Source/            JUCE wrapper
  PluginProcessor  Builds the APVTS layout from kParams, feeds Params into wub::Synth each block
  PluginEditor     Custom UI (look and feel, Knob, Choice, WaveDisplay); fixed 1000x660
tools/render_test.cpp      Offline renderer: every preset -> WAV + stats. No JUCE.
tools/host_test/           VST3 host smoke test and editor PNG snapshot (opt-in CMake targets)
scripts/install-mac.command  End-user installer shipped in the CI artifact
```

The DSP core must stay JUCE-free so `render_test` and the Linux CI job can compile it with only `g++`. Don't include `<JuceHeader.h>` or JUCE types anywhere under `Source/dsp/`.

## Parameters and presets

Everything about a parameter lives in `kParams` in `ParamDefs.h`: id, display name, kind, range, default, skew, choices, unit and tooltip. The processor, the editor tooltips and the presets all read from it.

- The `enum P` order must match the `kParams` row order exactly. `kParams` is indexed by `P`.
- Parameter `id` strings are saved in host sessions and presets. Renaming or removing one breaks users' saved projects. Add new parameters instead, and give them a default that keeps old sessions sounding the same.
- Choice parameters are stored as float indices. Read them with `Params::idx()`. When adding a choice option, append it to the end of the names array (plus any matching value array like `kRatioValues`/`kRateBeats`) and update the `max` in `kParams` and the readable enums at the top of `Presets.h` (`T`, `R`, `S`, `F`, `D`). Filter modes are also mirrored by `FilterMode` in `Effects.h`. Inserting in the middle shifts saved values.
- A new parameter also needs a control in `PluginEditor.cpp`. Knobs are added with `addKnob`/`addChoice` in the constructor and placed by section in `resized()`.
- `Preset::values` lists only overrides; anything not listed uses the `kParams` default. `Init` is the empty preset and must stay last. Presets are exposed to hosts as programs, and `setCurrentProgram` pushes values through the APVTS.

## Real-time rules

`Synth::process` and `Wub4Processor::processBlock` run on the audio thread. No allocation, locks, logging or file I/O there. Buffers are sized in `prepare()`. The UI reads DSP state only through the `std::atomic` display taps on `Synth`.

The chain runs at 2x the host rate (`osr`). Filter and envelope coefficients use `osr`; OTT and reverb use `sr`. The sub sine bypasses filter, dirt and OTT by design, and a safety clipper sits at the output.

## Build

```bash
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release --target WUB4_VST3 WUB4_AU
```

JUCE 8.0.10 is fetched by CMake on first configure. Pass `-DJUCE_DIR=/path/to/JUCE` to use a local checkout. Built plugins land in `build/WUB4_artefacts/Release/`; `COPY_PLUGIN_AFTER_BUILD` is off, so nothing is installed automatically.

## Testing

Fastest check after any DSP or preset change (no JUCE, a few seconds):

```bash
g++ -std=c++17 -O2 tools/render_test.cpp -o render_test && mkdir -p renders && ./render_test renders
```

It prints `preset peak rmsDb xRT nan` per preset and writes WAVs to `renders/`. CI fails if any preset has a NaN or a peak below 0.05. The CI check reads columns from the end of each line (preset names contain spaces), so keep the last four columns in that order if you change the output.

Host smoke test and UI snapshots (needs a built VST3):

```bash
cmake -S . -B build_test -DWUB4_HOST_TEST=ON
cmake --build build_test --target wub4_host_test wub4_snapshot
```

`wub4_host_test <WUB4.vst3> <outdir>` loads the plugin, renders a few programs, checks peaks and state round-trips. `wub4_snapshot <outdir>` writes editor PNGs for a few presets, which is the quickest way to see a UI change without a DAW.

## CI

`.github/workflows/build.yml` runs on pushes to `main`, tags `v*` and PRs:
- **macos**: universal Xcode build of VST3 + AU, ad-hoc codesign, pluginval at strictness 7, then uploads the `WUB4-macOS` artifact. Tags also publish a GitHub Release.
- **dsp-test** (Ubuntu): the `render_test` check above.

## Conventions

- C++17, JUCE spacing style: a space before call parentheses (`foo (x)`), Allman braces, `float` literals with `f`.
- Keep the DSP header-only and inside `namespace wub`.
- `build/`, `build_test/`, `renders/`, `dist/` and audio files are gitignored. Don't commit renders.
- Licensing: JUCE is used under AGPLv3 / Starter. No Reason or Malström content may be added; SQR*4 is an original table.
