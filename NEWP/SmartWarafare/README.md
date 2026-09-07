# Smart Scan Strategy for Electronic Warfare (Demo)

SIH25055 — ML-based Electronic Support (ES) receiver scan scheduler, demonstrated
against a simulated RF environment with synthetic emitters (fixed, periodic-hopping,
and frequency-agile).

## What it does

- `backend/simulator.py` — synthetic RF environment: multiple frequency bands, each
  with emitters that transmit on a fixed band, hop periodically, or agile-hop randomly.
- `backend/scheduler.py` — scan strategies:
  - Open-loop baselines: `round_robin`, `random`
  - Online ML schedulers: `epsilon_greedy`, `ucb1` (learn per-band hit rates from
    sensor feedback and bias future scans toward active bands)
  - `ml_predictor` — offline-trained classifier (see below) that predicts per-band
    activity probability from each band's own scan history and scans the most
    likely band each step.
- `backend/features.py` — shared feature engineering (used identically by data
  generation, training, and the live `ml_predictor` scheduler to avoid train/serve
  skew).
- `backend/generate_training_data.py` — runs many randomized simulated episodes
  under a round-robin sweep and records receiver-observable features + noisy
  sensor outcomes to `backend/data/training_data.csv`.
- `backend/train_model.py` — trains a RandomForest classifier on that dataset and
  saves it to `backend/models/band_activity_model.joblib`.
- `backend/app.py` — FastAPI service running the simulation loop and computing
  figures of merit: Pd, Pfa, sensitivity, hit/intercept rate, interception ratio,
  avg reward, % correct predictions, avg intercept time & error.
- `frontend/` — React (Vite) dashboard: live spectrum grid, metrics cards, reward /
  hit-rate charts, and a per-emitter table comparing true vs. observed intercept timing.

## Generate training data & train the ML scheduler

```bash
cd backend
python3 -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
python3 generate_training_data.py   # writes data/training_data.csv (~36k rows)
python3 train_model.py              # writes models/band_activity_model.joblib
```

Once trained, select `ml_predictor` as the scheduler in the dashboard (or via
`compare_schedulers.py`) to use it. Without a trained model, `ml_predictor` falls
back to random exploration.

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
   (online learning) and `ml_predictor` (offline-trained ML) — the learning
   schedulers converge to a higher interception ratio and lower average
   intercept time as they learn which bands are busiest, with `ml_predictor`
   reaching the highest hit rate once trained.
