// WUB4 — parameter definitions shared by the DSP core, the plugin wrapper and the presets.
#pragma once

namespace wub
{

enum P : int
{
    // Oscillator
    Table, Position, Growl, FmRatio, Octave, Unison, Detune, Width,
    // Sub
    SubLevel, SubOct,
    // Wub LFO
    LfoRate, LfoShape, LfoToTable, LfoToGrowl, LfoToFilter, LfoToVolume, LfoToPitch,
    // Filter
    FiltMode, Cutoff, Reso, KeyTrack,
    // Dirt
    DirtMode, Drive, DirtMix,
    // FX
    Ott, Space, LowCut,
    // Amp / pitch
    Attack, Decay, Sustain, Release, Glide, Legato, DropAmt, DropTime,
    // Output
    Master,
    NumParams
};

enum class Kind { Float, Choice, Int, Bool };

inline const char* const kTableNames[]   = { "SQR*4", "Metal Square", "PWM", "Yoi Saw", "FM Growl", "Sync Saw" };
inline const char* const kRatioNames[]   = { "0.5", "1", "1.5", "2", "3", "4" };
inline const float       kRatioValues[]  = { 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f };
inline const char* const kSubOctNames[]  = { "Same", "-1 Oct" };
inline const char* const kRateNames[]    = { "1/1", "1/2", "1/2T", "1/4", "1/4T", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
inline const double      kRateBeats[]    = { 4.0, 2.0, 4.0 / 3.0, 1.0, 2.0 / 3.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
inline const char* const kShapeNames[]   = { "Sine", "Triangle", "Saw Down", "Saw Up", "Square", "Wonk", "Stairs" };
inline const char* const kFilterNames[]  = { "Off", "Low Pass", "Band Pass", "Notch", "Comb +", "Comb -", "Vowel" };
inline const char* const kDirtNames[]    = { "Off", "Tube", "Clip", "Fold", "Crush" };

struct ParamDef
{
    const char* id;
    const char* name;
    Kind kind;
    float min, max, def;
    float skewCentre;           // 0 = linear
    const char* const* choices; // for Choice
    int numChoices;
    const char* unit;
    const char* help;           // tooltip
};

#define WUB_CHOICES(arr) arr, (int) (sizeof (arr) / sizeof (arr[0]))

inline const ParamDef kParams[NumParams] = {
    { "table",     "Table",        Kind::Choice, 0, 5, 0, 0, WUB_CHOICES (kTableNames), "", "Wavetable. SQR*4 is a hard-synced square that morphs from 1x to 4x as you turn Position." },
    { "position",  "Position",     Kind::Float, 0, 1, 0.30f, 0, nullptr, 0, "", "Where in the wavetable to read (Malstrom calls this Index)." },
    { "growl",     "Growl",        Kind::Float, 0, 1, 0.0f, 0, nullptr, 0, "", "Amount of FM (phase modulation) from a hidden sine. Adds grit and snarl." },
    { "fmratio",   "FM Ratio",     Kind::Choice, 0, 5, 1, 0, WUB_CHOICES (kRatioNames), "", "Pitch ratio of the FM modulator." },
    { "octave",    "Octave",       Kind::Int,   -3, 2, 0, 0, nullptr, 0, "", "Octave of the main bass (the sub ignores this)." },
    { "unison",    "Unison",       Kind::Int,    1, 5, 1, 0, nullptr, 0, "", "Number of stacked oscillators. Riddim usually stays at 1-2." },
    { "detune",    "Detune",       Kind::Float, 0, 1, 0.15f, 0, nullptr, 0, "", "Spread of the unison voices." },
    { "width",     "Width",        Kind::Float, 0, 1, 0.30f, 0, nullptr, 0, "", "Stereo spread of the unison voices." },

    { "sublevel",  "Sub",          Kind::Float, 0, 1, 0.50f, 0, nullptr, 0, "", "Level of the clean mono sine sub. It bypasses the filter, dirt and OTT." },
    { "suboct",    "Sub Oct",      Kind::Choice, 0, 1, 0, 0, WUB_CHOICES (kSubOctNames), "", "Sub at the played note or one octave lower." },

    { "lforate",   "Wub Rate",     Kind::Choice, 0, 9, 6, 0, WUB_CHOICES (kRateNames), "", "Tempo-synced wobble rate. T = triplet, the riddim staple." },
    { "lfoshape",  "Wub Shape",    Kind::Choice, 0, 6, 2, 0, WUB_CHOICES (kShapeNames), "", "Shape of the wobble. It restarts on every note." },
    { "lfotable",  "Wub > Table",  Kind::Float, -1, 1, 0.50f, 0, nullptr, 0, "", "How far the wobble moves the wavetable Position." },
    { "lfogrowl",  "Wub > Growl",  Kind::Float, -1, 1, 0.0f, 0, nullptr, 0, "", "How much the wobble moves the FM amount." },
    { "lfofilter", "Wub > Filter", Kind::Float, -1, 1, 0.0f, 0, nullptr, 0, "", "How much the wobble opens the filter (up to 6 octaves)." },
    { "lfovolume", "Wub > Volume", Kind::Float, -1, 1, 0.0f, 0, nullptr, 0, "", "Rhythmic volume gating, also applied to the sub." },
    { "lfopitch",  "Wub > Pitch",  Kind::Float, -1, 1, 0.0f, 0, nullptr, 0, "", "Pitch wobble (up to 12 semitones) for wonky sounds." },

    { "filtmode",  "Filter",       Kind::Choice, 0, 6, 1, 0, WUB_CHOICES (kFilterNames), "", "Filter type. Comb filters give the metallic square sound." },
    { "cutoff",    "Cutoff",       Kind::Float, 20, 20000, 3000, 1000, nullptr, 0, "Hz", "Filter cutoff. For comb filters this is the comb pitch." },
    { "reso",      "Reso",         Kind::Float, 0, 1, 0.20f, 0, nullptr, 0, "", "Resonance or comb feedback." },
    { "keytrack",  "Key Track",    Kind::Float, 0, 1, 0.0f, 0, nullptr, 0, "", "How much the cutoff follows the note you play. Use 100% for combs." },

    { "dirtmode",  "Dirt",         Kind::Choice, 0, 4, 1, 0, WUB_CHOICES (kDirtNames), "", "Distortion type." },
    { "drive",     "Drive",        Kind::Float, 0, 1, 0.40f, 0, nullptr, 0, "", "Distortion amount." },
    { "dirtmix",   "Dirt Mix",     Kind::Float, 0, 1, 1.0f, 0, nullptr, 0, "", "Dry/wet of the distortion." },

    { "ott",       "OTT",          Kind::Float, 0, 1, 0.50f, 0, nullptr, 0, "", "Built-in 3-band upward/downward compressor (OTT-style)." },
    { "space",     "Space",        Kind::Float, 0, 1, 0.08f, 0, nullptr, 0, "", "Reverb on the mid bass only." },
    { "lowcut",    "Low Cut",      Kind::Float, 20, 300, 90, 100, nullptr, 0, "Hz", "High-pass on the mid bass so the sub owns the low end." },

    { "attack",    "Attack",       Kind::Float, 0, 2000, 2, 50, nullptr, 0, "ms", "Amp envelope attack." },
    { "decay",     "Decay",        Kind::Float, 1, 4000, 300, 300, nullptr, 0, "ms", "Amp envelope decay." },
    { "sustain",   "Sustain",      Kind::Float, 0, 1, 1.0f, 0, nullptr, 0, "", "Amp envelope sustain level." },
    { "release",   "Release",      Kind::Float, 1, 4000, 40, 200, nullptr, 0, "ms", "Amp envelope release." },
    { "glide",     "Glide",        Kind::Float, 0, 1000, 0, 100, nullptr, 0, "ms", "Portamento between overlapping notes." },
    { "legato",    "Legato",       Kind::Bool,  0, 1, 0, 0, nullptr, 0, "", "When on, overlapping notes do not restart the envelope and wobble." },
    { "dropamt",   "Drop",         Kind::Float, 0, 48, 0, 12, nullptr, 0, "st", "Pitch dive at the start of each note, in semitones." },
    { "droptime",  "Drop Time",    Kind::Float, 5, 2000, 120, 200, nullptr, 0, "ms", "How long the pitch dive takes." },

    { "master",    "Master",       Kind::Float, -24, 12, 0, 0, nullptr, 0, "dB", "Output level." },
};

#undef WUB_CHOICES

struct Params
{
    float v[NumParams];

    Params() { for (int i = 0; i < NumParams; ++i) v[i] = kParams[i].def; }
    float operator[] (int i) const { return v[i]; }
    float& operator[] (int i)      { return v[i]; }
    int   idx (int i) const        { return (int) (v[i] + 0.5f); }
};

} // namespace wub
