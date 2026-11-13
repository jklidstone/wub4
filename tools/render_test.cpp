// Offline renderer for WUB4: plays a riddim pattern through every preset, writes WAVs and prints stats.
#include "../Source/dsp/Synth.h"
#include "../Source/dsp/Presets.h"

#include <chrono>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    if (! f) return;
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 2 * 2;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + dataBytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (dataBytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t a = (int16_t) std::lround (std::clamp (L[i], -1.0f, 1.0f) * 32767.0f);
        int16_t b = (int16_t) std::lround (std::clamp (R[i], -1.0f, 1.0f) * 32767.0f);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
}

struct Ev { int step; int len; int note; };

int main (int argc, char** argv)
{
    const std::string outDir = argc > 1 ? argv[1] : ".";
    const int sr = 44100;
    const double bpm = 140.0;
    const double stepSec = 60.0 / bpm / 4.0; // 16th notes
    const int bars = 4;

    // half-time riddim chop pattern (16 steps per bar)
    const std::vector<Ev> bar = { { 0, 4, 29 }, { 6, 2, 29 }, { 8, 3, 29 }, { 12, 2, 32 }, { 14, 2, 29 } };
    const std::vector<Ev> barB = { { 0, 3, 29 }, { 3, 3, 29 }, { 6, 2, 36 }, { 8, 4, 29 }, { 13, 3, 27 } };

    const int total = (int) (bars * 16 * stepSec * sr) + sr; // +1s tail
    std::printf ("%-22s %8s %8s %8s %8s\n", "preset", "peak", "rmsDb", "xRT", "nan");

    for (int pi = 0; pi < wub::kNumPresets; ++pi)
    {
        wub::Synth synth;
        synth.prepare (sr);
        synth.setBpm (bpm);
        synth.setParams (wub::kPresets[pi].toParams());

        std::vector<std::pair<int, int>> events; // sample, +note / -note-1
        for (int b = 0; b < bars; ++b)
            for (auto& e : (b % 2 ? barB : bar))
            {
                const int on = (int) (((b * 16) + e.step) * stepSec * sr);
                const int off = (int) (((b * 16) + e.step + e.len) * stepSec * sr) - 64;
                events.push_back ({ on, e.note });
                events.push_back ({ off, -e.note - 1 });
            }
        std::sort (events.begin(), events.end(), [] (auto& a, auto& b) { return a.first < b.first || (a.first == b.first && a.second < b.second); });

        std::vector<float> L ((size_t) total), R ((size_t) total);
        size_t ei = 0;
        int pos = 0;
        const auto t0 = std::chrono::steady_clock::now();
        while (pos < total)
        {
            int next = std::min (total, pos + 256);
            if (ei < events.size()) next = std::min (next, std::max (pos + 1, events[ei].first));
            while (ei < events.size() && events[ei].first <= pos)
            {
                if (events[ei].second >= 0) synth.noteOn (events[ei].second, 1.0f);
                else synth.noteOff (-events[ei].second - 1);
                ++ei;
            }
            synth.process (L.data() + pos, R.data() + pos, next - pos);
            pos = next;
        }
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();

        float peak = 0; double sum = 0; int nans = 0;
        for (int i = 0; i < total; ++i)
        {
            if (! std::isfinite (L[(size_t) i]) || ! std::isfinite (R[(size_t) i])) ++nans;
            peak = std::max (peak, std::max (std::abs (L[(size_t) i]), std::abs (R[(size_t) i])));
            sum += 0.5 * (L[(size_t) i] * L[(size_t) i] + R[(size_t) i] * R[(size_t) i]);
        }
        const double rmsDb = 10.0 * std::log10 (sum / total + 1e-12);
        const double audioSecs = (double) total / sr;
        std::printf ("%-22s %8.3f %8.1f %8.0f %8d\n", wub::kPresets[pi].name, peak, rmsDb, audioSecs / secs, nans);

        std::string name = wub::kPresets[pi].name;
        for (auto& c : name) if (c == ' ' || c == '(' || c == ')') c = '_';
        char buf[512];
        std::snprintf (buf, sizeof buf, "%s/%02d_%s.wav", outDir.c_str(), pi + 1, name.c_str());
        writeWav (buf, L, R, sr);
    }
    return 0;
}
