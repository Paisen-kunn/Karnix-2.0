// Synthetic RF environment for the Smart Scan EW demo (C++ port).
// Mirrors backend/simulator.py: emitters transmit on a fixed band, hop
// periodically, or agile-hop randomly; ground truth is reported per step.
#pragma once
#include <algorithm>
#include <map>
#include <random>
#include <string>
#include <vector>

struct Emitter {
    int id;
    std::string name;
    std::string mode;      // "fixed" | "periodic" | "agile"
    std::vector<int> bands;
    int period;
    double duty_cycle;
    int offset;

    // Returns the band index this emitter is transmitting on, or -1 if silent.
    int active_band(int t, std::mt19937 &rng) const {
        std::uniform_real_distribution<double> uni(0.0, 1.0);
        if (mode == "fixed") {
            return uni(rng) < duty_cycle ? bands[0] : -1;
        }
        if (mode == "periodic") {
            int idx = ((t + offset) / period) % static_cast<int>(bands.size());
            return uni(rng) < duty_cycle ? bands[idx] : -1;
        }
        if (mode == "agile") {
            if (uni(rng) < duty_cycle) {
                std::uniform_int_distribution<int> pick(0, static_cast<int>(bands.size()) - 1);
                return bands[pick(rng)];
            }
            return -1;
        }
        return -1;
    }
};

class RFEnvironment {
public:
    int num_bands;
    std::vector<Emitter> emitters;

    RFEnvironment(int num_bands_, int num_emitters, unsigned seed)
        : num_bands(num_bands_) {
        std::mt19937 rng(seed);
        static const std::vector<std::string> names = {
            "Air-Search Radar", "Fire-Control Radar", "Comms Link", "Nav Beacon",
            "SAM Illuminator", "IFF Transponder", "Jammer", "Recon Link",
        };
        static const std::vector<std::string> modes = {"fixed", "periodic", "periodic", "agile"};
        static const std::vector<int> periods = {2, 3, 4, 5, 8};

        std::uniform_int_distribution<int> mode_pick(0, static_cast<int>(modes.size()) - 1);
        std::uniform_int_distribution<int> period_pick(0, static_cast<int>(periods.size()) - 1);
        std::uniform_real_distribution<double> duty_pick(0.5, 0.95);

        for (int i = 0; i < num_emitters; ++i) {
            std::string mode = modes[mode_pick(rng)];
            int n_bands = mode == "fixed" ? 1 : 2 + (rng() % 3); // 2..4
            n_bands = std::min(n_bands, num_bands);

            std::vector<int> pool(num_bands);
            for (int b = 0; b < num_bands; ++b) pool[b] = b;
            std::shuffle(pool.begin(), pool.end(), rng);
            std::vector<int> bands(pool.begin(), pool.begin() + n_bands);

            int period = mode == "fixed" ? 1 : periods[period_pick(rng)];
            double duty_cycle = duty_pick(rng);
            std::uniform_int_distribution<int> offset_pick(0, period - 1);

            emitters.push_back(Emitter{
                i, names[i % names.size()] + "-" + std::to_string(i), mode, bands,
                period, duty_cycle, offset_pick(rng),
            });
        }
    }

    // Returns band -> list of emitter ids transmitting on it this step.
    std::map<int, std::vector<int>> step(int t, std::mt19937 &rng) const {
        std::map<int, std::vector<int>> active;
        for (const auto &e : emitters) {
            int b = e.active_band(t, rng);
            if (b >= 0) active[b].push_back(e.id);
        }
        return active;
    }
};
