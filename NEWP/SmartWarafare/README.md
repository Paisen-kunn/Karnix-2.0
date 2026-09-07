# Smart Scan Strategy for Electronic Warfare (Demo)

SIH25055 — ML-based Electronic Support (ES) receiver scan scheduler, demonstrated
against a simulated RF environment with synthetic emitters (fixed, periodic-hopping,
and frequency-agile).

## What it does

- `backend/simulator.py` — synthetic RF environment: multiple frequency bands, each
  with emitters that transmit on a fixed band, hop periodically, or agile-hop randomly.
- `backend/scheduler.py` — scan strategies:
  - Open-loop baselines: `round_robin`, `random`
  - ML-based closed-loop schedulers: `epsilon_greedy`, `ucb1` (learn per-band hit
    rates from sensor feedback and bias future scans toward active bands)
- `backend/app.py` — FastAPI service running the simulation loop and computing
  figures of merit: Pd, Pfa, sensitivity, hit/intercept rate, interception ratio,
  avg reward, % correct predictions, avg intercept time & error.
- `frontend/` — React (Vite) dashboard: live spectrum grid, metrics cards, reward /
  hit-rate charts, and a per-emitter table comparing true vs. observed intercept timing.

## Run the backend

```bash
cd backend
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
uvicorn app:app --reload --port 8000
```

## Run the frontend

```bash
cd frontend
npm install
npm run dev
```

Open the printed Vite URL (default http://localhost:5173). The dashboard talks to
the backend at `http://localhost:8000`.

## Using the dashboard

1. Set number of bands / emitters and pick a scheduler, then **Reset**.
2. **Step** advances the simulation by N steps at a time; **Play** auto-steps.
3. Compare `round_robin`/`random` (open-loop) against `epsilon_greedy`/`ucb1`
   (learning schedulers) — the ML schedulers converge to a higher interception
   ratio and lower average intercept time as they learn which bands are busiest.
