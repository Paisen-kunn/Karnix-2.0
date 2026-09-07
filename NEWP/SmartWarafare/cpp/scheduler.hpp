// Scan scheduling strategies (C++ port of backend/scheduler.py) plus
// higher-performance, non-stationary-aware ML schedulers that go beyond
// the Python reference implementation.
#pragma once
#include <algorithm>
#include <cmath>
#include <deque>
#include <memory>
#include <random>
#include <string>
#include <vector>

class BaseScheduler {
public:
    explicit BaseScheduler(int num_bands) : num_bands_(num_bands) {}
    virtual ~BaseScheduler() = default;
    virtual int choose(int t, std::mt19937 &rng) = 0;
    virtual void update(int band, double reward) {}

protected:
    int num_bands_;
};

// Open-loop baseline: sweeps every band in order, no learning.
class RoundRobinScheduler : public BaseScheduler {
public:
    using BaseScheduler::BaseScheduler;
    int choose(int t, std::mt19937 &) override { return t % num_bands_; }
};

// Open-loop baseline: uniformly random band each step.
class RandomScheduler : public BaseScheduler {
public:
    using BaseScheduler::BaseScheduler;
    int choose(int, std::mt19937 &rng) override {
        std::uniform_int_distribution<int> pick(0, num_bands_ - 1);
        return pick(rng);
    }
};

// ML scheduler: exploit best observed band, else explore with decaying epsilon.
class EpsilonGreedyScheduler : public BaseScheduler {
public:
    EpsilonGreedyScheduler(int num_bands, double epsilon = 0.25, double decay = 0.995)
        : BaseScheduler(num_bands), epsilon_(epsilon), decay_(decay),
          counts_(num_bands, 0), values_(num_bands, 0.0) {}

    int choose(int, std::mt19937 &rng) override {
        for (int b = 0; b < num_bands_; ++b)
            if (counts_[b] == 0) return b;

        std::uniform_real_distribution<double> uni(0.0, 1.0);
        epsilon_ = std::max(0.02, epsilon_ * decay_);
        if (uni(rng) < epsilon_) {
            std::uniform_int_distribution<int> pick(0, num_bands_ - 1);
            return pick(rng);
        }
        int best = 0;
        for (int b = 1; b < num_bands_; ++b)
            if (values_[b] > values_[best]) best = b;
        return best;
    }

    void update(int band, double reward) override {
        counts_[band] += 1;
        values_[band] += (reward - values_[band]) / counts_[band];
    }

private:
    double epsilon_, decay_;
    std::vector<int> counts_;
    std::vector<double> values_;
};

// UCB1 bandit: balances exploration/exploitation via confidence bounds.
class UCB1Scheduler : public BaseScheduler {
public:
    explicit UCB1Scheduler(int num_bands, double c = 1.4)
        : BaseScheduler(num_bands), c_(c), total_(0),
          counts_(num_bands, 0), values_(num_bands, 0.0) {}

    int choose(int, std::mt19937 &) override {
        for (int b = 0; b < num_bands_; ++b)
            if (counts_[b] == 0) return b;

        total_ += 1;
        int best = 0;
        double best_score = -1e18;
        for (int b = 0; b < num_bands_; ++b) {
            double score = values_[b] + c_ * std::sqrt(std::log(total_ + 1) / counts_[b]);
            if (score > best_score) {
                best_score = score;
                best = b;
            }
        }
        return best;
    }

    void update(int band, double reward) override {
        counts_[band] += 1;
        values_[band] += (reward - values_[band]) / counts_[band];
    }

private:
    double c_;
    long total_;
    std::vector<int> counts_;
    std::vector<double> values_;
};

// Bayesian bandit: models each band as a Beta(alpha, beta) success
// probability and samples from the posterior each step (probability
// matching). Converges faster than UCB1/epsilon-greedy in practice and
// naturally balances explore/exploit without tuned hyperparameters.
class ThompsonSamplingScheduler : public BaseScheduler {
public:
    explicit ThompsonSamplingScheduler(int num_bands)
        : BaseScheduler(num_bands), alpha_(num_bands, 1.0), beta_(num_bands, 1.0) {}

    int choose(int, std::mt19937 &rng) override {
        int best = 0;
        double best_sample = -1.0;
        for (int b = 0; b < num_bands_; ++b) {
            std::gamma_distribution<double> ga(alpha_[b], 1.0);
            std::gamma_distribution<double> gb(beta_[b], 1.0);
            double x = ga(rng);
            double y = gb(rng);
            double sample = x / (x + y); // Beta(alpha,beta) via two Gammas
            if (sample > best_sample) {
                best_sample = sample;
                best = b;
            }
        }
        return best;
    }

    // Reward is treated as a Bernoulli "was this a productive scan" signal.
    void update(int band, double reward) override {
        if (reward > 0) alpha_[band] += 1.0;
        else beta_[band] += 1.0;
    }

private:
    std::vector<double> alpha_, beta_;
};

// Sliding-window UCB: like UCB1 but only remembers the last `window`
// observations per band, so it can re-adapt when frequency-agile or
// mission-tasked emitters change behaviour mid-run (plain UCB1/epsilon-
// greedy estimates go stale and never "forget").
class SlidingWindowUCBScheduler : public BaseScheduler {
public:
    explicit SlidingWindowUCBScheduler(int num_bands, int window = 150, double c = 1.4)
        : BaseScheduler(num_bands), window_(window), c_(c), total_(0),
          history_(num_bands), sums_(num_bands, 0.0) {}

    int choose(int, std::mt19937 &) override {
        for (int b = 0; b < num_bands_; ++b)
            if (history_[b].empty()) return b;

        total_ += 1;
        int best = 0;
        double best_score = -1e18;
        for (int b = 0; b < num_bands_; ++b) {
            double n = static_cast<double>(history_[b].size());
            double mean = sums_[b] / n;
            double score = mean + c_ * std::sqrt(std::log(total_ + 1) / n);
            if (score > best_score) {
                best_score = score;
                best = b;
            }
        }
        return best;
    }

    void update(int band, double reward) override {
        auto &h = history_[band];
        h.push_back(reward);
        sums_[band] += reward;
        if (static_cast<int>(h.size()) > window_) {
            sums_[band] -= h.front();
            h.pop_front();
        }
    }

private:
    int window_;
    double c_;
    long total_;
    std::vector<std::deque<double>> history_;
    std::vector<double> sums_;
};

inline std::unique_ptr<BaseScheduler> make_scheduler(const std::string &name, int num_bands) {
    if (name == "round_robin") return std::make_unique<RoundRobinScheduler>(num_bands);
    if (name == "random") return std::make_unique<RandomScheduler>(num_bands);
    if (name == "epsilon_greedy") return std::make_unique<EpsilonGreedyScheduler>(num_bands);
    if (name == "ucb1") return std::make_unique<UCB1Scheduler>(num_bands);
    if (name == "thompson") return std::make_unique<ThompsonSamplingScheduler>(num_bands);
    return std::make_unique<SlidingWindowUCBScheduler>(num_bands);
}
