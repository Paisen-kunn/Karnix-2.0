"""Scan scheduling strategies for the Electronic Support receiver.

Open-loop baselines (RoundRobin, Random) sweep the spectrum blindly.
The closed-loop / "smart" schedulers (EpsilonGreedy, UCB1) learn per-band
hit-rate estimates from sensor feedback (hit / no-hit) and bias the scan
toward bands that historically carry more activity. MLPredictorScheduler
goes a step further and uses an offline-trained classifier (see
generate_training_data.py / train_model.py) to predict per-band activity
probability from each band's own scan history.
"""
import math
import os
import random
from typing import List, Optional

from features import extract_features


class BaseScheduler:
    name = "base"

    def __init__(self, num_bands: int):
        self.num_bands = num_bands

    def choose(self, t: int) -> int:
        raise NotImplementedError

    def update(self, band: int, reward: float, detected: Optional[bool] = None) -> None:
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

    def update(self, band: int, reward: float, detected: Optional[bool] = None) -> None:
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

    def update(self, band: int, reward: float, detected: Optional[bool] = None) -> None:
        self.counts[band] += 1
        n = self.counts[band]
        self.values[band] += (reward - self.values[band]) / n


class MLPredictorScheduler(BaseScheduler):
    """Learned scheduler: an offline-trained classifier predicts, from each
    band's own past scan outcomes and elapsed time, the probability it is
    currently transmitting; the receiver scans the highest-probability
    band. Requires models/band_activity_model.joblib (see
    generate_training_data.py + train_model.py); falls back to a
    round-robin warm-up and light random exploration otherwise."""
    name = "ml_predictor"

    _MODEL_PATH = os.path.join(os.path.dirname(__file__), "models", "band_activity_model.joblib")

    def __init__(self, num_bands: int, epsilon: float = 0.05):
        super().__init__(num_bands)
        self.epsilon = epsilon
        self.scan_history: List[List[int]] = [[] for _ in range(num_bands)]
        self.last_scan_t: List[Optional[int]] = [None] * num_bands
        self.last_hit_t: List[Optional[int]] = [None] * num_bands
        self._t_at_choose: Optional[int] = None
        self.model = self._load_model()

    @classmethod
    def _load_model(cls):
        if not os.path.exists(cls._MODEL_PATH):
            return None
        try:
            import joblib
            return joblib.load(cls._MODEL_PATH)
        except Exception:
            return None

    def choose(self, t: int) -> int:
        self._t_at_choose = t
        for b in range(self.num_bands):
            if self.last_scan_t[b] is None:
                return b
        if self.model is None or random.random() < self.epsilon:
            return random.randrange(self.num_bands)
        feats = [
            extract_features(t, self.scan_history[b], self.last_scan_t[b], self.last_hit_t[b])
            for b in range(self.num_bands)
        ]
        probs = self.model.predict_proba(feats)[:, 1]
        return max(range(self.num_bands), key=lambda b: probs[b])

    def update(self, band: int, reward: float, detected: Optional[bool] = None) -> None:
        hit = detected if detected is not None else reward > 0
        t = self._t_at_choose if self._t_at_choose is not None else len(self.scan_history[band])
        self.scan_history[band].append(1 if hit else 0)
        self.last_scan_t[band] = t
        if hit:
            self.last_hit_t[band] = t


SCHEDULERS = {
    "round_robin": RoundRobinScheduler,
    "random": RandomScheduler,
    "epsilon_greedy": EpsilonGreedyScheduler,
    "ucb1": UCB1Scheduler,
    "ml_predictor": MLPredictorScheduler,
}
