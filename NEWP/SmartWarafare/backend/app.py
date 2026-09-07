"""FastAPI backend for the Smart Scan EW dashboard.

Wraps the RF environment + scan scheduler in a single in-memory
Simulation session and exposes reset / step / state endpoints consumed
by the React dashboard.
"""
import random
from typing import Dict, List, Optional

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel

from NEWP.SmartWarafare.backend.scheduler import SCHEDULERS
from NEWP.SmartWarafare.backend.simulator import RFEnvironment

app = FastAPI(title="Smart Scan EW API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# Sensor characteristics (fixed for the demo receiver).
SENSOR_PD = 0.92    # prob. of detecting a real transmission on the scanned band
SENSOR_PFA = 0.03   # prob. of a false alarm when the scanned band is silent

MAX_HISTORY = 200


class Simulation:
    def __init__(self, num_bands=16, num_emitters=6, scheduler_type="ucb1", seed: Optional[int] = None):
        self.num_bands = num_bands
        self.scheduler_type = scheduler_type
        self.env = RFEnvironment(num_bands=num_bands, num_emitters=num_emitters, seed=seed)
        self.scheduler = SCHEDULERS[scheduler_type](num_bands)
        self.t = 0
        self.tp = self.fp = self.tn = self.fn = 0
        self.cumulative_reward = 0.0
        self.total_transmission_events = 0
        self.intercepted_events = 0
        self.last_intercept_t: Dict[int, int] = {}
        self.intercept_intervals: Dict[int, List[int]] = {e.id: [] for e in self.env.emitters}
        self.history: List[dict] = []

    def step_once(self) -> dict:
        t = self.t
        active = self.env.step(t)  # band -> [emitter ids]
        self.total_transmission_events += len(active)

        band = self.scheduler.choose(t)
        emitters_here = active.get(band, [])
        band_active = len(emitters_here) > 0

        if band_active:
            detected = random.random() < SENSOR_PD
            if detected:
                result = "TP"
                self.tp += 1
                self.intercepted_events += 1
                for eid in emitters_here:
                    if eid in self.last_intercept_t:
                        self.intercept_intervals[eid].append(t - self.last_intercept_t[eid])
                    self.last_intercept_t[eid] = t
                reward = 1.0
            else:
                result = "FN"
                self.fn += 1
                reward = -0.3
        else:
            false_alarm = random.random() < SENSOR_PFA
            if false_alarm:
                result = "FP"
                self.fp += 1
                reward = -0.5
            else:
                result = "TN"
                self.tn += 1
                reward = -0.05

        self.scheduler.update(band, reward)
        self.cumulative_reward += reward
        self.t += 1

        step_record = {
            "t": t,
            "active_bands": sorted(active.keys()),
            "scanned_band": band,
            "result": result,
            "reward": reward,
        }
        self.history.append(step_record)
        if len(self.history) > MAX_HISTORY:
            self.history.pop(0)
        return step_record

    def metrics(self) -> dict:
        pd = self.tp / (self.tp + self.fn) if (self.tp + self.fn) else 0.0
        pfa = self.fp / (self.fp + self.tn) if (self.fp + self.tn) else 0.0
        total_scans = max(self.t, 1)
        hit_rate = self.tp / total_scans
        avg_reward = self.cumulative_reward / total_scans
        interception_ratio = (
            self.intercepted_events / self.total_transmission_events
            if self.total_transmission_events else 0.0
        )
        all_intervals = [v for vals in self.intercept_intervals.values() for v in vals]
        avg_intercept_time = sum(all_intervals) / len(all_intervals) if all_intervals else None

        per_emitter = []
        errors = []
        for e in self.env.emitters:
            intervals = self.intercept_intervals[e.id]
            avg_interval = sum(intervals) / len(intervals) if intervals else None
            true_period = e.period if e.mode == "periodic" else None
            error = abs(avg_interval - true_period) if (avg_interval is not None and true_period) else None
            if error is not None:
                errors.append(error)
            per_emitter.append({
                "id": e.id,
                "name": e.name,
                "mode": e.mode,
                "period": e.period,
                "intercepts": len(intervals) + (1 if intervals or e.id in self.last_intercept_t else 0),
                "avg_intercept_interval": avg_interval,
                "intercept_time_error": error,
            })
        avg_intercept_time_error = sum(errors) / len(errors) if errors else None

        return {
            "t": self.t,
            "scheduler": self.scheduler_type,
            "pd": pd,
            "pfa": pfa,
            "sensitivity": pd,
            "hit_rate": hit_rate,
            "avg_reward": avg_reward,
            "cumulative_reward": self.cumulative_reward,
            "percent_correct_predictions": hit_rate * 100.0,
            "interception_ratio": interception_ratio,
            "avg_intercept_time": avg_intercept_time,
            "avg_intercept_time_error": avg_intercept_time_error,
            "counts": {"tp": self.tp, "fp": self.fp, "tn": self.tn, "fn": self.fn},
            "per_emitter": per_emitter,
        }


SIM: Optional[Simulation] = None


class ResetRequest(BaseModel):
    num_bands: int = 16
    num_emitters: int = 6
    scheduler_type: str = "ucb1"
    seed: Optional[int] = None


class StepRequest(BaseModel):
    steps: int = 1


@app.post("/api/reset")
def reset(req: ResetRequest):
    global SIM
    scheduler_type = req.scheduler_type if req.scheduler_type in SCHEDULERS else "ucb1"
    SIM = Simulation(
        num_bands=req.num_bands,
        num_emitters=req.num_emitters,
        scheduler_type=scheduler_type,
        seed=req.seed,
    )
    return {
        "ok": True,
        "num_bands": SIM.num_bands,
        "emitters": SIM.env.describe_emitters(),
        "schedulers": list(SCHEDULERS.keys()),
        "metrics": SIM.metrics(),
    }


@app.post("/api/step")
def step(req: StepRequest):
    global SIM
    if SIM is None:
        SIM = Simulation()
    n = max(1, min(req.steps, 500))
    records = [SIM.step_once() for _ in range(n)]
    return {"steps": records, "metrics": SIM.metrics()}


@app.get("/api/state")
def state():
    global SIM
    if SIM is None:
        SIM = Simulation()
    return {
        "num_bands": SIM.num_bands,
        "emitters": SIM.env.describe_emitters(),
        "schedulers": list(SCHEDULERS.keys()),
        "history": SIM.history[-50:],
        "metrics": SIM.metrics(),
    }
