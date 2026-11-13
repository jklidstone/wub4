// WUB4 — band-limited wavetable bank, generated at load time (no sample assets).
#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <functional>
#include <vector>

namespace wub
{

constexpr int kTableSize  = 2048;   // samples per frame (power of two)
constexpr int kFrames     = 32;     // frames per table
constexpr int kMips       = 11;     // mip level m keeps harmonics 1..(1024 >> m)
constexpr int kRenderSize = 8192;   // oversized render for time-domain generators
constexpr int kMaxHarm    = 1023;
constexpr int kNumTables  = 6;

namespace detail
{
    using cd = std::complex<double>;

    inline void fft (std::vector<cd>& a, bool inverse)
    {
        const size_t n = a.size();
        for (size_t i = 1, j = 0; i < n; ++i)
        {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap (a[i], a[j]);
        }
        for (size_t len = 2; len <= n; len <<= 1)
        {
            const double ang = 2.0 * M_PI / (double) len * (inverse ? 1.0 : -1.0);
            const cd wl (std::cos (ang), std::sin (ang));
            for (size_t i = 0; i < n; i += len)
            {
                cd w (1.0);
                for (size_t k = 0; k < len / 2; ++k)
                {
                    cd u = a[i + k], v = a[i + k + len / 2] * w;
                    a[i + k] = u + v;
                    a[i + k + len / 2] = u - v;
                    w *= wl;
                }
            }
        }
        if (inverse)
            for (auto& x : a) x /= (double) n;
    }

    inline double frac (double x) { return x - std::floor (x); }
}

// Harmonic spectrum of one frame: c[n] for n = 1..kMaxHarm, signal = sum Re(c[n] e^{j 2pi n t}).
using Spectrum = std::vector<std::complex<double>>;

struct Wavetable
{
    std::vector<float> data; // [mip][frame][kTableSize + 1]  (one guard sample)

    const float* get (int mip, int frame) const
    {
        return data.data() + ((size_t) mip * kFrames + (size_t) frame) * (kTableSize + 1);
    }
    float* get (int mip, int frame)
    {
        return data.data() + ((size_t) mip * kFrames + (size_t) frame) * (kTableSize + 1);
    }

    // Builds all mip levels from a per-frame spectrum generator.
    void build (const std::function<Spectrum (double)>& spectrumFor)
    {
        using namespace detail;
        data.assign ((size_t) kMips * kFrames * (kTableSize + 1), 0.0f);
        std::vector<cd> buf (kTableSize);

        for (int f = 0; f < kFrames; ++f)
        {
            const double p = (double) f / (double) (kFrames - 1);
            Spectrum s = spectrumFor (p);
            float norm = 1.0f;

            for (int m = 0; m < kMips; ++m)
            {
                const int limit = std::min (kMaxHarm, 1024 >> m);
                std::fill (buf.begin(), buf.end(), cd (0.0));
                for (int n = 1; n <= limit; ++n)
                {
                    buf[(size_t) n] = s[(size_t) n] * (double) kTableSize * 0.5;
                    buf[(size_t) (kTableSize - n)] = std::conj (buf[(size_t) n]);
                }
                fft (buf, true);
                float* out = get (m, f);

                if (m == 0)
                {
                    double peak = 1e-9;
                    for (int i = 0; i < kTableSize; ++i) peak = std::max (peak, std::abs (buf[(size_t) i].real()));
                    norm = (float) (1.0 / peak);
                }
                for (int i = 0; i < kTableSize; ++i) out[i] = (float) buf[(size_t) i].real() * norm;
                out[kTableSize] = out[0];
            }
        }
    }
};

// Spectrum from a time-domain function y(t), t in [0,1), rendered at kRenderSize.
inline Spectrum spectrumFromTime (const std::function<double (double)>& y)
{
    using namespace detail;
    std::vector<cd> buf (kRenderSize);
    for (int i = 0; i < kRenderSize; ++i) buf[(size_t) i] = y ((double) i / kRenderSize);
    fft (buf, false);
    Spectrum s (kMaxHarm + 1, cd (0.0));
    for (int n = 1; n <= kMaxHarm; ++n) s[(size_t) n] = buf[(size_t) n] * (2.0 / kRenderSize);
    return s;
}

// Square spectrum (odd sine harmonics), helper for spectral tables.
inline std::complex<double> sineHarmonic (double amp) { return { 0.0, -amp }; }

class WavetableBank
{
public:
    static const WavetableBank& get()
    {
        static const WavetableBank bank;
        return bank;
    }

    const Wavetable& operator[] (int i) const { return tables[(size_t) std::clamp (i, 0, kNumTables - 1)]; }

private:
    std::vector<Wavetable> tables;

    WavetableBank()
    {
        using detail::frac;
        tables.resize (kNumTables);

        // 0: SQR*4 — hard-synced square, slave ratio sweeps 1x -> 4x across the table.
        //     Hollow, nasal "graintable square" sweep; the core riddim tone.
        tables[0].build ([] (double p)
        {
            const double r = 1.0 + 3.0 * p;
            return spectrumFromTime ([r] (double t) { return frac (t * r) < 0.5 ? 1.0 : -1.0; });
        });

        // 1: Metal Square — square with evenly spaced comb notches that tighten across the table.
        tables[1].build ([] (double p)
        {
            Spectrum s (kMaxHarm + 1, { 0.0, 0.0 });
            const double spacing = 0.18 * p;          // comb "delay" in cycles
            for (int n = 1; n <= kMaxHarm; n += 2)
            {
                const double comb = 0.5 + 0.5 * std::cos (2.0 * M_PI * n * spacing);
                const double amp  = (4.0 / M_PI) / n * (0.08 + 0.92 * comb);
                s[(size_t) n] = sineHarmonic (amp);
            }
            // add a little even-harmonic buzz as it gets more metallic
            for (int n = 2; n <= kMaxHarm; n += 2)
            {
                const double comb = 0.5 + 0.5 * std::cos (2.0 * M_PI * n * spacing);
                s[(size_t) n] = sineHarmonic (0.35 * p / n * comb);
            }
            return s;
        });

        // 2: PWM — pulse width 50% -> 3%.
        tables[2].build ([] (double p)
        {
            const double w = 0.5 - 0.47 * p;
            return spectrumFromTime ([w] (double t) { return t < w ? 1.0 : -1.0; });
        });

        // 3: Yoi Saw — saw through vowel formants, u -> o -> a -> e -> i across the table.
        tables[3].build ([] (double p)
        {
            static const double F[5][3] = { { 300, 870, 2240 }, { 570, 840, 2410 }, { 730, 1090, 2440 },
                                            { 530, 1840, 2480 }, { 270, 2290, 3010 } };
            const double x = p * 4.0;
            const int i0 = std::min (3, (int) x);
            const double fr = x - i0;
            double f[3];
            for (int k = 0; k < 3; ++k) f[k] = F[i0][k] + fr * (F[i0 + 1][k] - F[i0][k]);
            const double bw[3] = { 90, 110, 150 }, g[3] = { 1.0, 0.65, 0.35 };
            const double f0 = 55.0; // formants placed for a ~A1 bass note

            Spectrum s (kMaxHarm + 1, { 0.0, 0.0 });
            for (int n = 1; n <= kMaxHarm; ++n)
            {
                const double hz = n * f0;
                double env = 0.03;
                for (int k = 0; k < 3; ++k)
                {
                    const double d = (hz - f[k]) / bw[k];
                    env += g[k] / (1.0 + d * d);
                }
                s[(size_t) n] = sineHarmonic ((2.0 / M_PI) / n * env * 6.0);
            }
            return s;
        });

        // 4: FM Growl — sine phase-modulated at 2:1 with a touch of 1:1, index rising across the table.
        tables[4].build ([] (double p)
        {
            const double I = 7.0 * std::pow (p, 1.3);
            return spectrumFromTime ([I] (double t)
            {
                const double ph = 2.0 * M_PI * t;
                return std::sin (ph + I * std::sin (2.0 * ph + 0.5 * std::sin (ph)));
            });
        });

        // 5: Sync Saw — hard-synced saw, ratio 1x -> 6x. Screechy / wonky leads and talking basses.
        tables[5].build ([] (double p)
        {
            const double r = 1.0 + 5.0 * p;
            return spectrumFromTime ([r] (double t) { return 2.0 * frac (t * r) - 1.0; });
        });
    }
};

// Picks the mip level whose highest harmonic stays under Nyquist.
inline int mipForFrequency (double hz, double sampleRate)
{
    const double maxHarm = 0.5 * sampleRate / std::max (1.0, hz);
    int m = 0;
    while (m < kMips - 1 && (double) (1024 >> m) > maxHarm) ++m;
    return m;
}

inline float readTable (const Wavetable& wt, int mip, float position, double phase)
{
    const float fp = std::clamp (position, 0.0f, 1.0f) * (float) (kFrames - 1);
    const int f0 = std::min ((int) fp, kFrames - 1);
    const int f1 = std::min (f0 + 1, kFrames - 1);
    const float ff = fp - (float) f0;

    const double idx = phase * kTableSize;
    int i0 = (int) idx;
    const float fr = (float) (idx - i0);
    i0 &= (kTableSize - 1);

    const float* a = wt.get (mip, f0);
    const float* b = wt.get (mip, f1);
    const float va = a[i0] + fr * (a[i0 + 1] - a[i0]);
    const float vb = b[i0] + fr * (b[i0 + 1] - b[i0]);
    return va + ff * (vb - va);
}

} // namespace wub
