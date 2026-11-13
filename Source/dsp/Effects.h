// WUB4 — filters, distortion, OTT-style compressor, reverb and helpers.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace wub
{

inline float dbToGain (float db) { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g)  { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }

// ---------------------------------------------------------------- TPT state-variable filter
struct Svf
{
    float ic1 = 0, ic2 = 0;
    float g = 0, k = 1.414f, a1 = 0, a2 = 0, a3 = 0;
    float low = 0, band = 0, high = 0;

    void reset() { ic1 = ic2 = 0; }

    void set (float cutoff, float q, float sampleRate)
    {
        cutoff = std::clamp (cutoff, 10.0f, 0.49f * sampleRate);
        g = std::tan ((float) M_PI * cutoff / sampleRate);
        k = 1.0f / std::max (q, 0.05f);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    void tick (float x)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        low = v2; band = v1; high = x - k * v1 - v2;
        if (! std::isfinite (ic1) || ! std::isfinite (ic2)) reset();
    }
};

// ---------------------------------------------------------------- Comb filter (feedback, damped)
struct Comb
{
    std::vector<float> buf;
    int w = 0;
    float damp = 0;

    void prepare (float sampleRate)
    {
        buf.assign ((size_t) std::max (64, (int) (sampleRate / 15.0f) + 4), 0.0f);
        w = 0; damp = 0;
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); damp = 0; }

    float process (float x, float freq, float feedback, float sampleRate)
    {
        const int size = (int) buf.size();
        const float delay = std::clamp (sampleRate / std::max (freq, 15.0f), 2.0f, (float) size - 2.0f);
        float rp = (float) w - delay;
        if (rp < 0) rp += (float) size;
        const int i0 = (int) rp;
        const int i1 = (i0 + 1) % size;
        const float fr = rp - (float) i0;
        const float d = buf[(size_t) i0] + fr * (buf[(size_t) i1] - buf[(size_t) i0]);
        damp += 0.55f * (d - damp); // gentle high damping inside the loop
        const float y = x + feedback * damp;
        buf[(size_t) w] = std::clamp (y, -8.0f, 8.0f);
        if (++w >= size) w = 0;
        return y * (1.0f - 0.75f * std::abs (feedback));
    }
};

// ---------------------------------------------------------------- Filter section
enum FilterMode { FOff, FLowPass, FBandPass, FNotch, FCombPlus, FCombMinus, FVowel };

struct FilterChannel
{
    Svf a, b;
    Svf formant[3];
    Comb comb;

    void prepare (float sr) { comb.prepare (sr); reset(); }
    void reset()
    {
        a.reset(); b.reset(); comb.reset();
        for (auto& f : formant) f.reset();
    }

    float process (float x, int mode, float cutoff, float reso, float sr)
    {
        switch (mode)
        {
            case FLowPass:
            {
                const float q = 0.5f + reso * reso * 9.5f;
                a.set (cutoff, 0.54f, sr); b.set (cutoff, q, sr);
                a.tick (x); b.tick (a.low);
                return b.low;
            }
            case FBandPass:
            {
                const float q = 0.7f + reso * 12.0f;
                a.set (cutoff, q, sr); a.tick (x);
                return a.band * a.k * 1.4f;
            }
            case FNotch:
            {
                const float q = 0.4f + reso * 6.0f;
                a.set (cutoff, q, sr); a.tick (x);
                return a.low + a.high;
            }
            case FCombPlus:  return comb.process (x, cutoff,  0.25f + 0.72f * reso, sr);
            case FCombMinus: return comb.process (x, cutoff, -(0.25f + 0.72f * reso), sr);
            case FVowel:
            {
                // cutoff picks the vowel: u - o - a - e - i from dark to bright
                static const float F[5][3] = { { 300, 870, 2240 }, { 570, 840, 2410 }, { 730, 1090, 2440 },
                                               { 530, 1840, 2480 }, { 270, 2290, 3010 } };
                const float norm = std::clamp (std::log (cutoff / 80.0f) / std::log (8000.0f / 80.0f), 0.0f, 1.0f);
                const float xi = norm * 4.0f;
                const int i0 = std::min (3, (int) xi);
                const float fr = xi - (float) i0;
                const float gains[3] = { 1.0f, 0.7f, 0.4f };
                const float q = 4.0f + reso * 16.0f;
                float y = 0.0f;
                for (int k = 0; k < 3; ++k)
                {
                    const float f = F[i0][k] + fr * (F[i0 + 1][k] - F[i0][k]);
                    formant[k].set (f, q, sr);
                    formant[k].tick (x);
                    y += formant[k].band * formant[k].k * gains[k];
                }
                return y * 1.6f + x * 0.05f;
            }
            default: return x;
        }
    }
};

// ---------------------------------------------------------------- Dirt (distortion)
enum DirtMode { DOff, DTube, DClip, DFold, DCrush };

struct Dirt
{
    float held = 0.0f;
    float counter = 0.0f;

    void reset() { held = 0; counter = 0; }

    float process (float x, int mode, float drive, float mix)
    {
        if (mode == DOff || mix <= 0.0f) return x;
        const float g = dbToGain (drive * 34.0f);
        float y = x;
        switch (mode)
        {
            case DTube:
            {
                const float bias = 0.25f;
                y = (std::tanh (x * g + bias) - std::tanh (bias)) * 0.95f;
                break;
            }
            case DClip:  y = std::clamp (x * g, -0.9f, 0.9f) * 1.05f; break;
            case DFold:  y = std::sin (x * (1.0f + drive * 9.0f) * (float) M_PI * 0.5f) * 0.9f; break;
            case DCrush:
            {
                // sample-rate reduction + bit reduction (Duda's "SR redux" yoi trick)
                const float hold = 1.0f + drive * drive * 40.0f;
                counter += 1.0f;
                if (counter >= hold) { counter -= hold; held = x; }
                const float bits = 16.0f - drive * 12.0f;
                const float steps = std::pow (2.0f, bits);
                y = std::round (std::clamp (held * (1.0f + drive * 2.0f), -1.0f, 1.0f) * steps) / steps;
                break;
            }
            default: break;
        }
        return x + mix * (y - x);
    }
};

// ---------------------------------------------------------------- 2x decimator (windowed-sinc FIR)
struct Decimator
{
    static constexpr int kTaps = 63;
    std::array<float, kTaps> h {};
    std::array<float, kTaps * 2> hist {};
    int pos = 0;

    Decimator()
    {
        const double fc = 0.225; // of the oversampled rate (0.45 of the output rate)
        double sum = 0;
        for (int i = 0; i < kTaps; ++i)
        {
            const double n = i - (kTaps - 1) / 2.0;
            const double sinc = n == 0 ? 2 * fc : std::sin (2 * M_PI * fc * n) / (M_PI * n);
            // Blackman window
            const double w = 0.42 - 0.5 * std::cos (2 * M_PI * i / (kTaps - 1)) + 0.08 * std::cos (4 * M_PI * i / (kTaps - 1));
            h[(size_t) i] = (float) (sinc * w);
            sum += sinc * w;
        }
        for (auto& c : h) c = (float) (c / sum);
    }

    void reset() { hist.fill (0.0f); pos = 0; }

    void push (float x)
    {
        hist[(size_t) pos] = x;
        hist[(size_t) (pos + kTaps)] = x;
        if (++pos >= kTaps) pos = 0;
    }

    float output() const
    {
        float acc = 0;
        const float* p = hist.data() + pos; // oldest .. newest
        for (int i = 0; i < kTaps; ++i) acc += h[(size_t) i] * p[i];
        return acc;
    }
};

// ---------------------------------------------------------------- Linkwitz-Riley 4th-order split
struct LR4
{
    Svf lp1, lp2, hp1, hp2;
    void set (float f, float sr)
    {
        for (auto* s : { &lp1, &lp2, &hp1, &hp2 }) s->set (f, 0.70710678f, sr);
    }
    void reset() { lp1.reset(); lp2.reset(); hp1.reset(); hp2.reset(); }
    void split (float x, float& lo, float& hi)
    {
        lp1.tick (x);  lp2.tick (lp1.low);  lo = lp2.low;
        hp1.tick (x);  hp2.tick (hp1.high); hi = hp2.high;
    }
};

// ---------------------------------------------------------------- OTT-style 3-band up/down compressor
struct OttComp
{
    struct Band
    {
        float env = 0;
        float atk = 0, rel = 0;
        float thrAbove, ratioAbove, thrBelow, ratioBelow, makeup;
    };

    LR4 xLow[2], xHigh[2], apLow[2], dryAp88[2], dryAp2500[2];
    Band bands[3];
    float sr = 48000;

    void prepare (float sampleRate)
    {
        sr = sampleRate;
        for (int c = 0; c < 2; ++c)
        {
            xLow[c].set (88.3f, sr); xHigh[c].set (2500.0f, sr); apLow[c].set (2500.0f, sr);
            xLow[c].reset(); xHigh[c].reset(); apLow[c].reset();
            dryAp88[c].set (88.3f, sr); dryAp2500[c].set (2500.0f, sr); dryAp88[c].reset(); dryAp2500[c].reset();
        }
        //                attack ms, release ms, thrAbove, ratioAbove, thrBelow, ratioBelow, makeup dB
        const float cfg[3][7] = { { 47.8f, 282.0f, -18.0f, 12.0f, -38.0f, 4.0f, 6.0f },
                                  { 22.4f, 282.0f, -16.0f, 12.0f, -38.0f, 4.0f, 3.5f },
                                  { 13.5f, 132.0f, -20.0f, 12.0f, -40.0f, 4.0f, 6.0f } };
        for (int b = 0; b < 3; ++b)
        {
            auto& band = bands[b];
            band.env = 0;
            band.atk = std::exp (-1.0f / (cfg[b][0] * 0.001f * sr));
            band.rel = std::exp (-1.0f / (cfg[b][1] * 0.001f * sr));
            band.thrAbove = cfg[b][2]; band.ratioAbove = cfg[b][3];
            band.thrBelow = cfg[b][4]; band.ratioBelow = cfg[b][5];
            band.makeup = cfg[b][6];
        }
    }

    static float gainFor (const Band& b, float levelDb)
    {
        float g = 0.0f;
        if (levelDb > b.thrAbove)
            g = (b.thrAbove - levelDb) * (1.0f - 1.0f / b.ratioAbove);
        else if (levelDb < b.thrBelow)
        {
            g = (b.thrBelow - levelDb) * (1.0f - 1.0f / b.ratioBelow);
            // don't pump up silence / noise floor
            const float fade = std::clamp ((levelDb + 75.0f) / 20.0f, 0.0f, 1.0f);
            g = std::min (g, 24.0f) * fade;
        }
        return g + b.makeup;
    }

    void process (float& l, float& r, float depth)
    {
        if (depth <= 0.0f) return;
        float in[2] = { l, r };
        float lo[2], mid[2], hi[2];
        for (int c = 0; c < 2; ++c)
        {
            float rest;
            xLow[c].split (in[c], lo[c], rest);
            xHigh[c].split (rest, mid[c], hi[c]);
            float a, b;
            apLow[c].split (lo[c], a, b); // phase-align low band with the 2.5k split
            lo[c] = a + b;
        }
        float* bandsIO[3] = { lo, mid, hi };
        float outL = 0, outR = 0;
        for (int i = 0; i < 3; ++i)
        {
            auto& b = bands[i];
            const float peak = std::max (std::abs (bandsIO[i][0]), std::abs (bandsIO[i][1]));
            const float coef = peak > b.env ? b.atk : b.rel;
            b.env = coef * b.env + (1.0f - coef) * peak;
            const float g = dbToGain (gainFor (b, gainToDb (b.env)));
            outL += bandsIO[i][0] * g;
            outR += bandsIO[i][1] * g;
        }
        // phase-align the dry path with the same all-pass response as the band split
        float dry[2];
        for (int c = 0; c < 2; ++c)
        {
            float a, b, d, e;
            dryAp88[c].split (in[c], a, b);
            dryAp2500[c].split (a + b, d, e);
            dry[c] = d + e;
        }
        const float trim = 0.7f;
        l = dry[0] + depth * (outL * trim - dry[0]);
        r = dry[1] + depth * (outR * trim - dry[1]);
    }
};

// ---------------------------------------------------------------- Small stereo reverb (Freeverb topology)
struct Reverb
{
    struct CombF { std::vector<float> b; int i = 0; float store = 0; };
    struct AP    { std::vector<float> b; int i = 0; };
    CombF combs[2][8];
    AP aps[2][4];

    void prepare (float sr)
    {
        static const int cl[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static const int al[4] = { 556, 441, 341, 225 };
        const float s = sr / 44100.0f;
        for (int c = 0; c < 2; ++c)
        {
            const int spread = c * 23;
            for (int i = 0; i < 8; ++i) { combs[c][i].b.assign ((size_t) ((cl[i] + spread) * s), 0.0f); combs[c][i].i = 0; combs[c][i].store = 0; }
            for (int i = 0; i < 4; ++i) { aps[c][i].b.assign ((size_t) ((al[i] + spread) * s), 0.0f); aps[c][i].i = 0; }
        }
    }

    void process (float inL, float inR, float& outL, float& outR)
    {
        const float input = (inL + inR) * 0.015f;
        const float fb = 0.84f, damp = 0.35f;
        float out[2] = { 0, 0 };
        for (int c = 0; c < 2; ++c)
        {
            for (auto& cb : combs[c])
            {
                float y = cb.b[(size_t) cb.i];
                cb.store = y * (1 - damp) + cb.store * damp;
                cb.b[(size_t) cb.i] = input + cb.store * fb;
                if (++cb.i >= (int) cb.b.size()) cb.i = 0;
                out[c] += y;
            }
            for (auto& ap : aps[c])
            {
                float bo = ap.b[(size_t) ap.i];
                float y = -out[c] + bo;
                ap.b[(size_t) ap.i] = out[c] + bo * 0.5f;
                if (++ap.i >= (int) ap.b.size()) ap.i = 0;
                out[c] = y;
            }
        }
        outL = out[0]; outR = out[1];
    }
};

// ---------------------------------------------------------------- Output safety clipper (soft knee, ceiling ~ -0.3 dBFS)
inline float softClip (float x)
{
    const float knee = 0.75f, ceil = 0.966f;
    const float a = std::abs (x);
    if (a <= knee) return x;
    const float over = (a - knee) / (ceil - knee);
    const float y = knee + (ceil - knee) * std::tanh (over);
    return std::copysign (y, x);
}

} // namespace wub
