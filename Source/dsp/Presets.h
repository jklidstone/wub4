// WUB4 — factory presets. Each preset lists only the values that differ from the defaults.
#pragma once

#include "ParamDefs.h"
#include <initializer_list>
#include <utility>

namespace wub
{

struct Preset
{
    const char* name;
    std::initializer_list<std::pair<P, float>> values;

    Params toParams() const
    {
        Params p;
        for (auto& kv : values) p[kv.first] = kv.second;
        return p;
    }
};

// Choice indices, for readability
namespace T  { enum { Sqr4, Metal, Pwm, Yoi, Fm, Sync }; }
namespace R  { enum { R1_1, R1_2, R1_2T, R1_4, R1_4T, R1_8, R1_8T, R1_16, R1_16T, R1_32 }; }
namespace S  { enum { Sine, Tri, SawDown, SawUp, Square, Wonk, Stairs }; }
namespace F  { enum { Off, LP, BP, Notch, CombP, CombM, Vowel }; }
namespace D  { enum { Off, Tube, Clip, Fold, Crush }; }

inline const Preset kPresets[] = {
    { "SQR4 Classic", {
        { Table, T::Sqr4 }, { Position, 0.30f }, { LfoRate, R::R1_8T }, { LfoShape, S::SawDown },
        { LfoToTable, 0.50f }, { FiltMode, F::LP }, { Cutoff, 3000 }, { Reso, 0.20f },
        { DirtMode, D::Tube }, { Drive, 0.40f }, { Ott, 0.50f }, { Space, 0.08f } } },

    { "Square4 Metal", {
        { Table, T::Metal }, { Position, 0.55f }, { LfoRate, R::R1_8T }, { LfoShape, S::Wonk },
        { LfoToTable, 0.35f }, { LfoToFilter, 0.15f },
        { FiltMode, F::CombP }, { Cutoff, 130 }, { Reso, 0.80f }, { KeyTrack, 1.0f },
        { DirtMode, D::Tube }, { Drive, 0.55f }, { Ott, 0.70f }, { Space, 0.10f }, { LowCut, 120 } } },

    { "Wonky Wub", {
        { Table, T::Sqr4 }, { Position, 0.15f }, { LfoRate, R::R1_4T }, { LfoShape, S::Sine },
        { LfoToTable, 0.40f }, { LfoToFilter, 0.70f }, { LfoToPitch, 0.06f },
        { FiltMode, F::LP }, { Cutoff, 300 }, { Reso, 0.45f },
        { DirtMode, D::Clip }, { Drive, 0.45f }, { Ott, 0.60f } } },

    { "Yoi Talker", {
        { Table, T::Yoi }, { Position, 0.20f }, { LfoRate, R::R1_8 }, { LfoShape, S::Tri },
        { LfoToTable, 0.75f }, { FiltMode, F::Vowel }, { Cutoff, 500 }, { Reso, 0.35f }, { LfoToFilter, 0.35f },
        { DirtMode, D::Crush }, { Drive, 0.30f }, { DirtMix, 0.45f }, { Ott, 0.45f }, { Space, 0.06f } } },

    { "FM Grind", {
        { Table, T::Fm }, { Position, 0.30f }, { Growl, 0.35f }, { FmRatio, 1 },
        { LfoRate, R::R1_16T }, { LfoShape, S::SawDown }, { LfoToGrowl, 0.45f }, { LfoToTable, 0.30f },
        { FiltMode, F::LP }, { Cutoff, 6000 }, { Reso, 0.10f },
        { DirtMode, D::Fold }, { Drive, 0.35f }, { Ott, 0.70f } } },

    { "Robot Riddim", {
        { Table, T::Pwm }, { Position, 0.60f }, { Growl, 0.25f }, { FmRatio, 5 },
        { LfoRate, R::R1_8 }, { LfoShape, S::Stairs }, { LfoToTable, 0.50f }, { LfoToGrowl, 0.30f },
        { FiltMode, F::BP }, { Cutoff, 1400 }, { Reso, 0.25f },
        { DirtMode, D::Crush }, { Drive, 0.35f }, { DirtMix, 0.70f }, { Ott, 0.55f }, { LowCut, 150 } } },

    { "Triplet Gate", {
        { Table, T::Sqr4 }, { Position, 0.75f }, { LfoRate, R::R1_8T }, { LfoShape, S::Square },
        { LfoToVolume, 0.90f }, { LfoToTable, 0.20f },
        { FiltMode, F::LP }, { Cutoff, 4500 }, { Reso, 0.15f },
        { DirtMode, D::Tube }, { Drive, 0.30f }, { Ott, 0.50f } } },

    { "Sync Screech", {
        { Table, T::Sync }, { Position, 0.45f }, { LfoRate, R::R1_16 }, { LfoShape, S::Wonk },
        { LfoToTable, 0.55f }, { FiltMode, F::BP }, { Cutoff, 1600 }, { Reso, 0.30f }, { KeyTrack, 0.5f },
        { DirtMode, D::Clip }, { Drive, 0.55f }, { Ott, 0.80f }, { Space, 0.18f }, { Octave, 1 } } },

    { "Comb Minus Hollow", {
        { Table, T::Sqr4 }, { Position, 0.50f }, { LfoRate, R::R1_4 }, { LfoShape, S::SawUp },
        { LfoToTable, 0.45f }, { LfoToFilter, 0.25f },
        { FiltMode, F::CombM }, { Cutoff, 260 }, { Reso, 0.70f }, { KeyTrack, 1.0f },
        { DirtMode, D::Tube }, { Drive, 0.50f }, { Ott, 0.60f }, { LowCut, 130 } } },

    { "Drop Dive", {
        { Table, T::Sqr4 }, { Position, 0.40f }, { DropAmt, 24 }, { DropTime, 350 },
        { LfoRate, R::R1_16T }, { LfoShape, S::SawDown }, { LfoToTable, 0.40f },
        { FiltMode, F::LP }, { Cutoff, 2500 }, { Reso, 0.25f },
        { DirtMode, D::Tube }, { Drive, 0.45f }, { Ott, 0.55f } } },

    { "Deep Square (Clean)", {
        { Table, T::Sqr4 }, { Position, 0.0f }, { LfoToTable, 0.0f },
        { FiltMode, F::LP }, { Cutoff, 900 }, { Reso, 0.10f },
        { DirtMode, D::Tube }, { Drive, 0.15f }, { Ott, 0.25f }, { Space, 0.0f } } },

    { "Init", {} },
};

inline constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));

} // namespace wub
