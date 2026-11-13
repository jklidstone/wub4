// WUB4 — monophonic riddim bass engine (framework-free so it can be tested offline).
#pragma once

#include "ParamDefs.h"
#include "Wavetables.h"
#include "Effects.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <vector>

namespace wub
{

class Synth
{
public:
    static constexpr int kMaxUnison = 5;

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        osr = sr * 2.0f;
        (void) WavetableBank::get();
        for (auto& f : filt) f.prepare (osr);
        for (auto& d : dec)  d.reset();
        for (auto& d : dirt) d.reset();
        for (int c = 0; c < 2; ++c) { lowCutA[c].reset(); lowCutB[c].reset(); }
        ott.prepare (sr);
        reverb.prepare (sr);
        notes.clear();
        envStage = SIdle; env = 0;
        smoothCoef = 1.0f - std::exp (-1.0f / (0.006f * osr));
        first = true;
    }

    void setBpm (double b) { if (b > 20.0 && b < 999.0) bpm = b; }

    // ------------------------------------------------------------- MIDI
    void noteOn (int note, float /*velocity*/)
    {
        const bool wasHeld = ! notes.empty();
        notes.erase (std::remove (notes.begin(), notes.end(), note), notes.end());
        notes.push_back (note);
        targetNote = (float) note;

        const bool legato = params.idx (Legato) != 0 && wasHeld && envStage != SIdle && envStage != SRelease;
        if (! (wasHeld && envStage != SIdle && glideMs > 0.0f))
            currentNote = targetNote; // jump unless gliding from a held note

        if (! legato)
        {
            envStage = SAttack;
            lfoPhase = 0.0;
            dropT = 0.0f;
            if (env < 0.001f)
            {
                for (int v = 0; v < kMaxUnison; ++v) { phase[v] = std::fmod (v * 0.37, 1.0); modPhase[v] = 0.0; }
                subPhase = 0.0;
                for (auto& f : filt) f.reset();
            }
        }
    }

    void noteOff (int note)
    {
        notes.erase (std::remove (notes.begin(), notes.end(), note), notes.end());
        if (notes.empty())
        {
            if (envStage != SIdle) envStage = SRelease;
        }
        else
            targetNote = (float) notes.back();
    }

    void allNotesOff() { notes.clear(); if (envStage != SIdle) envStage = SRelease; }

    // ------------------------------------------------------------- Display taps (read by the UI)
    std::atomic<float> displayPosition { 0.0f };
    std::atomic<float> displayLfo { 0.0f };
    std::atomic<float> displayEnv { 0.0f };

    // ------------------------------------------------------------- Render
    void setParams (const Params& p) { params = p; }

    void process (float* outL, float* outR, int numSamples)
    {
        const Params& p = params;
        const auto& wt = WavetableBank::get()[p.idx (Table)];
        const int unison = std::clamp (p.idx (Unison), 1, kMaxUnison);
        const float ratio = kRatioValues[std::clamp (p.idx (FmRatio), 0, 5)];
        const int filtMode = p.idx (FiltMode);
        const int dirtMode = p.idx (DirtMode);
        const int lfoShape = p.idx (LfoShape);
        const double beats = kRateBeats[std::clamp (p.idx (LfoRate), 0, 9)];
        const double lfoHz = (bpm / 60.0) / beats;
        const float octave = (float) p.idx (Octave);
        const float subOct = p.idx (SubOct) == 1 ? -12.0f : 0.0f;
        glideMs = p[Glide];

        // envelope coefficients (at oversampled rate)
        const float atkInc = 1.0f / std::max (1.0f, p[Attack] * 0.001f * osr);
        const float decCoef = std::exp (-4.0f / std::max (1.0f, p[Decay] * 0.001f * osr));
        const float relCoef = std::exp (-4.0f / std::max (1.0f, p[Release] * 0.001f * osr));
        const float glideCoef = glideMs <= 0.0f ? 1.0f : 1.0f - std::exp (-4.0f / (glideMs * 0.001f * osr));
        const float dropTau = std::max (1.0f, p[DropTime] * 0.001f * osr) / 4.0f;
        const float lowCut = p[LowCut];
        for (int c = 0; c < 2; ++c) { lowCutA[c].set (lowCut, 0.5412f, sr); lowCutB[c].set (lowCut, 1.3066f, sr); }

        if (first)
        {
            sPos = p[Position]; sGrowl = p[Growl]; sCut = p[Cutoff]; sDrive = p[Drive];
            sMaster = dbToGain (p[Master]); sSub = p[SubLevel]; sReso = p[Reso];
            first = false;
        }

        float lastLfo = 0.0f;

        for (int n = 0; n < numSamples; ++n)
        {
            float gateOut = 0.0f;
            float pitchNoteForSub = 0.0f;

            for (int os = 0; os < 2; ++os)
            {
                // parameter smoothing
                sPos   += smoothCoef * (p[Position] - sPos);
                sGrowl += smoothCoef * (p[Growl] - sGrowl);
                sCut   += smoothCoef * (p[Cutoff] - sCut);
                sReso  += smoothCoef * (p[Reso] - sReso);
                sDrive += smoothCoef * (p[Drive] - sDrive);

                // envelope
                switch (envStage)
                {
                    case SAttack:  env += atkInc; if (env >= 1.0f) { env = 1.0f; envStage = SDecay; } break;
                    case SDecay:   env = p[Sustain] + (env - p[Sustain]) * decCoef; break;
                    case SRelease: env *= relCoef; if (env < 1.0e-5f) { env = 0.0f; envStage = SIdle; } break;
                    case SIdle: default: break;
                }

                // LFO (retriggered per note)
                const float lfo = lfoValue (lfoShape, (float) lfoPhase);
                lfoPhase += lfoHz / osr;
                if (lfoPhase >= 1.0) lfoPhase -= 1.0;
                lfoSmooth += 0.25f * (lfo - lfoSmooth);
                const float L = lfoSmooth;
                lastLfo = L;

                // pitch
                currentNote += glideCoef * (targetNote - currentNote);
                const float drop = p[DropAmt] * std::exp (-dropT / dropTau);
                dropT += 1.0f;
                const float basePitch = currentNote - drop;
                const float pitch = basePitch + octave * 12.0f + L * p[LfoToPitch] * 12.0f;
                const float hz = 440.0f * std::pow (2.0f, (pitch - 69.0f) / 12.0f);
                pitchNoteForSub = basePitch;

                const float pos = std::clamp (sPos + L * p[LfoToTable], 0.0f, 1.0f);
                const float growl = std::clamp (sGrowl + L * p[LfoToGrowl], 0.0f, 1.0f);
                const float pmIndex = growl * growl * 0.9f; // in cycles

                // oscillators
                float sumL = 0.0f, sumR = 0.0f;
                for (int v = 0; v < unison; ++v)
                {
                    const float spread = unison == 1 ? 0.0f : (2.0f * v / (unison - 1) - 1.0f);
                    const float vhz = hz * std::pow (2.0f, spread * p[Detune] * 40.0f / 1200.0f);
                    const int mip = mipForFrequency (vhz * (1.0f + pmIndex * ratio * 2.0f), osr);
                    const double inc = vhz / osr;
                    double ph = phase[v];
                    if (pmIndex > 0.0001f)
                        ph += pmIndex * std::sin (2.0 * M_PI * modPhase[v]);
                    ph -= std::floor (ph);
                    const float s = readTable (wt, mip, pos, ph);
                    phase[v] += inc;          if (phase[v] >= 1.0) phase[v] -= 1.0;
                    modPhase[v] += inc * ratio; if (modPhase[v] >= 1.0) modPhase[v] -= std::floor (modPhase[v]);
                    const float pan = spread * p[Width];
                    sumL += s * (1.0f - std::max (0.0f, pan));
                    sumR += s * (1.0f + std::min (0.0f, pan));
                }
                const float uniNorm = 1.0f / std::sqrt ((float) unison);
                sumL *= uniNorm; sumR *= uniNorm;

                // filter
                const float keyMul = std::pow (2.0f, p[KeyTrack] * (basePitch + octave * 12.0f - 36.0f) / 12.0f);
                const float cut = std::clamp (sCut * keyMul * std::pow (2.0f, L * p[LfoToFilter] * 6.0f), 15.0f, 0.45f * osr);
                float yL = filt[0].process (sumL, filtMode, cut, sReso, osr);
                float yR = unison == 1 ? yL : filt[1].process (sumR, filtMode, cut, sReso, osr);

                // dirt
                yL = dirt[0].process (yL, dirtMode, sDrive, p[DirtMix]);
                yR = unison == 1 ? yL : dirt[1].process (yR, dirtMode, sDrive, p[DirtMix]);

                // amp + wub volume gate
                const float dv = p[LfoToVolume];
                const float gate = dv >= 0.0f ? 1.0f - dv * (1.0f - L) : 1.0f + dv * L;
                const float amp = env * gate;
                gateOut = amp;

                dec[0].push (yL * amp);
                dec[1].push (yR * amp);
            }

            float l = dec[0].output();
            float r = dec[1].output();

            // low cut on the mid bass (24 dB/oct)
            lowCutA[0].tick (l); lowCutB[0].tick (lowCutA[0].high); l = lowCutB[0].high;
            lowCutA[1].tick (r); lowCutB[1].tick (lowCutA[1].high); r = lowCutB[1].high;

            // OTT on the mid bass only
            ott.process (l, r, p[Ott]);

            // space
            if (p[Space] > 0.001f)
            {
                float wl, wr;
                reverb.process (l, r, wl, wr);
                l += wl * p[Space] * 1.2f;
                r += wr * p[Space] * 1.2f;
            }

            // clean mono sub (no octave, no wub pitch; follows env + wub gate)
            sSub += 0.002f * (p[SubLevel] - sSub);
            const float subHz = 440.0f * std::pow (2.0f, (pitchNoteForSub + subOct - 69.0f) / 12.0f);
            subPhase += subHz / sr;
            if (subPhase >= 1.0) subPhase -= 1.0;
            const float sub = (float) std::sin (2.0 * M_PI * subPhase) * sSub * gateOut * 0.8f;

            sMaster += 0.002f * (dbToGain (p[Master]) - sMaster);
            float outLs = (l * 0.9f + sub) * sMaster;
            float outRs = (r * 0.9f + sub) * sMaster;
            if (! std::isfinite (outLs) || ! std::isfinite (outRs)) { outLs = outRs = 0.0f; hardReset(); }
            outL[n] = softClip (outLs);
            outR[n] = softClip (outRs);
        }

        displayPosition.store (std::clamp (sPos + lastLfo * p[LfoToTable], 0.0f, 1.0f));
        displayLfo.store (lastLfo);
        displayEnv.store (env);
    }

private:
    enum Stage { SIdle, SAttack, SDecay, SRelease };

    static float lfoValue (int shape, float ph)
    {
        switch (shape)
        {
            case 0: return 0.5f - 0.5f * std::cos (2.0f * (float) M_PI * ph);          // Sine (starts low)
            case 1: return ph < 0.5f ? 2.0f * ph : 2.0f - 2.0f * ph;                   // Triangle
            case 2: return 1.0f - ph;                                                  // Saw down
            case 3: return ph;                                                         // Saw up
            case 4: return ph < 0.5f ? 1.0f : 0.0f;                                    // Square
            case 5: { const float e = std::exp (-5.0f * ph); return (e - 0.006738f) / (1.0f - 0.006738f); } // Wonk
            case 6: return std::floor (ph * 4.0f) / 3.0f;                              // Stairs
            default: return 0.0f;
        }
    }

    void hardReset()
    {
        for (auto& f : filt) f.reset();
        for (auto& d : dec) d.reset();
        ott.prepare (sr);
        reverb.prepare (sr);
    }

    Params params;
    float sr = 48000.0f, osr = 96000.0f;
    double bpm = 140.0;

    std::vector<int> notes;
    float targetNote = 36, currentNote = 36, glideMs = 0;
    Stage envStage = SIdle;
    float env = 0;
    double lfoPhase = 0;
    float lfoSmooth = 0;
    float dropT = 0;

    double phase[kMaxUnison] {}, modPhase[kMaxUnison] {};
    double subPhase = 0;

    float smoothCoef = 0.01f;
    float sPos = 0, sGrowl = 0, sCut = 1000, sReso = 0, sDrive = 0, sMaster = 1, sSub = 0;
    bool first = true;

    FilterChannel filt[2];
    Dirt dirt[2];
    Decimator dec[2];
    Svf lowCutA[2], lowCutB[2];
    OttComp ott;
    Reverb reverb;
};

} // namespace wub
