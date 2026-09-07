"""Synthetic RF environment for the Smart Scan EW demo.

Generates emitters with fixed / periodic-hopping / frequency-agile
behaviour and reports ground-truth transmission activity per band
for each discrete time step.
"""
import random
from dataclasses import dataclass, field
from typing import Dict, List, Optional


@dataclass
class Emitter:
    id: int
    name: str
    mode: str          # "fixed" | "periodic" | "agile"
    bands: List[int]   # bands this emitter is able to use
    period: int         # hop period in steps (periodic) / avg gap (agile)
    duty_cycle: float  # probability it is actually "on" during its slot
    offset: int = 0

    def active_band(self, t: int) -> Optional[int]:
        if self.mode == "fixed":
            return self.bands[0] if random.random() < self.duty_cycle else None
        if self.mode == "periodic":
            idx = ((t + self.offset) // self.period) % len(self.bands)
            return self.bands[idx] if random.random() < self.duty_cycle else None
        if self.mode == "agile":
            if random.random() < self.duty_cycle:
                return random.choice(self.bands)
            return None
        return None


EMITTER_NAMES = [
    "Air-Search Radar", "Fire-Control Radar", "Comms Link", "Nav Beacon",
    "SAM Illuminator", "IFF Transponder", "Jammer", "Recon Link",
]


class RFEnvironment:
    def __init__(self, num_bands: int = 16, num_emitters: int = 5, seed: Optional[int] = None):
        self.rng = random.Random(seed)
        random.seed(seed) if seed is not None else None
        self.num_bands = num_bands
        self.emitters: List[Emitter] = []
        modes = ["fixed", "periodic", "periodic", "agile"]
        for i in range(num_emitters):
            mode = self.rng.choice(modes)
            n_bands = 1 if mode == "fixed" else self.rng.randint(2, min(4, num_bands))
            bands = self.rng.sample(range(num_bands), n_bands)
            period = self.rng.choice([2, 3, 4, 5, 8]) if mode != "fixed" else 1
            duty_cycle = round(self.rng.uniform(0.5, 0.95), 2)
            self.emitters.append(Emitter(
                id=i,
                name=f"{EMITTER_NAMES[i % len(EMITTER_NAMES)]}-{i}",
                mode=mode,
                bands=bands,
                period=period,
                duty_cycle=duty_cycle,
                offset=self.rng.randint(0, period - 1),
            ))

    def step(self, t: int) -> Dict[int, List[int]]:
        """Returns {band_index: [emitter_ids transmitting on it this step]}."""
        active: Dict[int, List[int]] = {}
        for e in self.emitters:
            b = e.active_band(t)
            if b is not None:
                active.setdefault(b, []).append(e.id)
        return active

    def describe_emitters(self):
        return [
            {
                "id": e.id,
                "name": e.name,
                "mode": e.mode,
                "bands": e.bands,
                "period": e.period,
                "duty_cycle": e.duty_cycle,
            }
            for e in self.emitters
        ]
