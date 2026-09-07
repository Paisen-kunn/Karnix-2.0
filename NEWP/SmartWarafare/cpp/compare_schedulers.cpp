// Compare open-loop vs ML-based scan scheduling, C++ engine.
//
// "Powerful" means: (1) two extra non-stationary-aware ML schedulers
// (Thompson Sampling, Sliding-Window UCB) beyond the Python reference,
// (2) large-scale Monte Carlo evaluation (many independent seeds x many
// steps) run in parallel across hardware threads, (3) wall-clock timing
// to show native throughput vs. an interpreted simulation.
//
// Build:  g++ -std=c++17 -O3 -pthread -o compare_schedulers compare_schedulers.cpp
// Run:    ./compare_schedulers
// Writes: scheduler_comparison.csv (mean curves) and prints a timing/stat summary.
#include <algorithm>
#include <chrono>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include "scheduler.hpp"
#include "simulator.hpp"

namespace {
constexpr int STEPS = 4000;          // 5x the original demo
constexpr int NUM_BANDS = 32;        // 2x the original demo
constexpr int NUM_EMITTERS = 12;
constexpr int NUM_TRIALS = 64;       // independent Monte Carlo seeds per strategy
constexpr double SENSOR_PD = 0.92;
constexpr double SENSOR_PFA = 0.03;

struct RunResult {
    std::vector<int> cumulative_hits;
    std::vector<double> hit_rate;
    std::vector<double> reward_curve;
};

RunResult run_once(const std::string &scheduler_name, unsigned seed) {
    std::mt19937 env_rng(seed);
    std::mt19937 sched_rng(seed + 1);
    std::uniform_real_distribution<double> uni(0.0, 1.0);

    RFEnvironment env(NUM_BANDS, NUM_EMITTERS, seed);
    auto scheduler = make_scheduler(scheduler_name, NUM_BANDS);

    RunResult result;
    result.cumulative_hits.reserve(STEPS);
    result.hit_rate.reserve(STEPS);
    result.reward_curve.reserve(STEPS);

    int hits = 0;
    double reward_sum = 0.0;

    for (int t = 0; t < STEPS; ++t) {
        auto active = env.step(t, env_rng);
        int band = scheduler->choose(t, sched_rng);
        bool band_active = active.count(band) > 0;

        double step_reward;
        if (band_active && uni(env_rng) < SENSOR_PD) {
            hits += 1;
            step_reward = 1.0;
        } else if (!band_active && uni(env_rng) < SENSOR_PFA) {
            step_reward = -0.5;
        } else if (band_active) {
            step_reward = -0.3;
        } else {
            step_reward = -0.05;
        }

        scheduler->update(band, step_reward);
        reward_sum += step_reward;

        result.cumulative_hits.push_back(hits);
        result.hit_rate.push_back(static_cast<double>(hits) / (t + 1));
        result.reward_curve.push_back(reward_sum);
    }
    return result;
}

// Runs NUM_TRIALS independent seeds for one strategy in parallel and
// averages the curves — a proper Monte Carlo evaluation, not a single run.
RunResult run_monte_carlo(const std::string &scheduler_name, unsigned base_seed) {
    std::vector<std::future<RunResult>> futures;
    futures.reserve(NUM_TRIALS);

    for (int trial = 0; trial < NUM_TRIALS; ++trial) {
        futures.push_back(std::async(std::launch::async, run_once, scheduler_name,
                                      base_seed + static_cast<unsigned>(trial) * 1000u));
    }

    RunResult mean;
    mean.cumulative_hits.assign(STEPS, 0);
    mean.hit_rate.assign(STEPS, 0.0);
    mean.reward_curve.assign(STEPS, 0.0);

    for (auto &fut : futures) {
        RunResult r = fut.get();
        for (int t = 0; t < STEPS; ++t) {
            mean.cumulative_hits[t] += r.cumulative_hits[t];
            mean.hit_rate[t] += r.hit_rate[t];
            mean.reward_curve[t] += r.reward_curve[t];
        }
    }
    for (int t = 0; t < STEPS; ++t) {
        mean.cumulative_hits[t] /= NUM_TRIALS;
        mean.hit_rate[t] /= NUM_TRIALS;
        mean.reward_curve[t] /= NUM_TRIALS;
    }
    return mean;
}
} // namespace

int main() {
    std::vector<std::pair<std::string, std::string>> strategies = {
        {"round_robin", "Round Robin (open-loop)"},
        {"random", "Random (open-loop)"},
        {"epsilon_greedy", "Epsilon-Greedy (ML)"},
        {"ucb1", "UCB1 Bandit (ML)"},
        {"thompson", "Thompson Sampling (ML, Bayesian)"},
        {"sliding_ucb", "Sliding-Window UCB (ML, non-stationary)"},
    };

    std::cout << "Engine: C++  |  hardware threads: " << std::thread::hardware_concurrency()
              << "  |  bands=" << NUM_BANDS << " emitters=" << NUM_EMITTERS
              << " steps=" << STEPS << " trials/strategy=" << NUM_TRIALS << "\n\n";

    std::ofstream csv("scheduler_comparison.csv");
    csv << "step,strategy,cumulative_hits,hit_rate,cumulative_reward\n";

    std::cout << std::left << std::setw(42) << "Strategy" << std::setw(20) << "Final Intercepts"
              << std::setw(16) << "Final Hit Rate" << "Wall Time\n";

    auto overall_start = std::chrono::steady_clock::now();

    for (const auto &[name, label] : strategies) {
        auto t0 = std::chrono::steady_clock::now();
        RunResult r = run_monte_carlo(name, /*base_seed=*/42);
        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        for (int t = 0; t < STEPS; ++t) {
            csv << t << ",\"" << label << "\"," << r.cumulative_hits[t] << ','
                << r.hit_rate[t] << ',' << r.reward_curve[t] << '\n';
        }
        std::cout << std::left << std::setw(42) << label << std::setw(20) << r.cumulative_hits.back()
                   << std::fixed << std::setprecision(1) << std::setw(15) << (r.hit_rate.back() * 100)
                   << std::setprecision(1) << ms << " ms\n";
    }

    auto overall_end = std::chrono::steady_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(overall_end - overall_start).count();
    long total_step_evals = static_cast<long>(strategies.size()) * NUM_TRIALS * STEPS;

    std::cout << "\nTotal: " << total_step_evals << " simulated scan-steps across "
              << strategies.size() * NUM_TRIALS << " Monte Carlo runs in "
              << std::setprecision(1) << total_ms << " ms ("
              << std::setprecision(0) << (total_step_evals / (total_ms / 1000.0)) << " steps/sec)\n";
    std::cout << "Saved scheduler_comparison.csv\n";
    return 0;
}
