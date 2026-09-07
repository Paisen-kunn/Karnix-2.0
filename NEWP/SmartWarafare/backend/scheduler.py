"""Scan scheduling strategies for the Electronic Support receiver.

Open-loop baselines (RoundRobin, Random) sweep the spectrum blindly.
The closed-loop / "smart" schedulers (EpsilonGreedy, UCB1) learn per-band
hit-rate estimates from sensor feedback (hit / no-hit) and bias the scan
toward bands that historically carry more activity.
"""
import math
import random
from typing import List


class BaseScheduler:
    name = "base"

    def __init__(self, num_bands: int):
        self.num_bands = num_bands

    def choose(self, t: int) -> int:
        raise NotImplementedError

    def update(self, band: int, reward: float) -> None:
        pass


class RoundRobinScheduler(BaseScheduler):
    """Classic open-loop sweep: visits every band in order, no learning."""
    name = "round_robin"

    def choose(self, t: int) -> int:
        return t % self.num_bands


class RandomScheduler(BaseScheduler):
    """Open-loop baseline: uniformly random band each step."""
    name = "random"

    def choose(self, t: int) -> int:
        return random.randrange(self.num_bands)


class EpsilonGreedyScheduler(BaseScheduler):
    """Simple ML scheduler: exploit the band with best observed reward,
    otherwise explore, with decaying exploration rate."""
    name = "epsilon_greedy"

    def __init__(self, num_bands: int, epsilon: float = 0.25, decay: float = 0.995):
        super().__init__(num_bands)
        self.epsilon = epsilon
        self.decay = decay
        self.counts: List[int] = [0] * num_bands
        self.values: List[float] = [0.0] * num_bands

    def choose(self, t: int) -> int:
        for b in range(self.num_bands):
            if self.counts[b] == 0:
                return b
        if random.random() < self.epsilon:
            self.epsilon = max(0.02, self.epsilon * self.decay)
            return random.randrange(self.num_bands)
        self.epsilon = max(0.02, self.epsilon * self.decay)
        best = max(range(self.num_bands), key=lambda b: self.values[b])
        return best

    def update(self, band: int, reward: float) -> None:
        self.counts[band] += 1
        n = self.counts[band]
        self.values[band] += (reward - self.values[band]) / n


class UCB1Scheduler(BaseScheduler):
    """Upper-Confidence-Bound bandit: balances exploration/exploitation
    using confidence intervals, well suited to non-stationary agile
    emitters since unexplored/under-sampled bands get re-visited."""
    name = "ucb1"

    def __init__(self, num_bands: int, c: float = 1.4):
        super().__init__(num_bands)
        self.c = c
        self.counts: List[int] = [0] * num_bands
        self.values: List[float] = [0.0] * num_bands
        self.total = 0

    def choose(self, t: int) -> int:
        for b in range(self.num_bands):
            if self.counts[b] == 0:
                return b
        self.total += 1
        scores = [
            self.values[b] + self.c * math.sqrt(math.log(self.total + 1) / self.counts[b])
            for b in range(self.num_bands)
        ]
        return max(range(self.num_bands), key=lambda b: scores[b])

    def update(self, band: int, reward: float) -> None:
        self.counts[band] += 1
        n = self.counts[band]
        self.values[band] += (reward - self.values[band]) / n


SCHEDULERS = {
    "round_robin": RoundRobinScheduler,
    "random": RandomScheduler,
    "epsilon_greedy": EpsilonGreedyScheduler,
    "ucb1": UCB1Scheduler,
}
