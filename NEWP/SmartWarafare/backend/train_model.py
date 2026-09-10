"""Train the band-activity classifier used by MLPredictorScheduler.

Reads one or more training CSVs -- data/training_data.csv from
generate_training_data.py (synthetic) and/or data/training_data_hf.csv from
convert_hf_radar_dataset.py (real Turing Synthetic Radar Dataset pulses) --
and fits a RandomForest classifier that predicts, from a band's own
past-scan features, the probability the band is transmitting right now.
Saves the fitted model to models/band_activity_model.joblib.

Usage:
    python3 generate_training_data.py                 # once, to (re)build the synthetic dataset
    python3 train_model.py
    # or, combined with real HF radar data:
    python3 train_model.py --data data/training_data.csv data/training_data_hf.csv
"""
import argparse
import csv
import os

import joblib
import numpy as np
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, roc_auc_score
from sklearn.model_selection import train_test_split

from features import FEATURE_NAMES

HERE = os.path.dirname(__file__)
DATA_PATH = os.path.join(HERE, "data", "training_data.csv")
MODEL_PATH = os.path.join(HERE, "models", "band_activity_model.joblib")


def load_dataset(paths) -> tuple:
    if isinstance(paths, str):
        paths = [paths]
    X, y = [], []
    for path in paths:
        with open(path, newline="") as f:
            reader = csv.DictReader(f)
            for row in reader:
                X.append([float(row[name]) for name in FEATURE_NAMES])
                y.append(int(row["label"]))
    return np.array(X, dtype=float), np.array(y, dtype=int)


def train(data_path=DATA_PATH, model_path: str = MODEL_PATH) -> None:
    paths = [data_path] if isinstance(data_path, str) else list(data_path)
    missing = [p for p in paths if not os.path.exists(p)]
    if missing:
        raise SystemExit(
            f"Missing training data file(s): {missing} -- run generate_training_data.py "
            "and/or convert_hf_radar_dataset.py first."
        )

    X, y = load_dataset(paths)
    print(f"Loaded {len(X)} rows from: {', '.join(paths)}")
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42, stratify=y
    )

    clf = RandomForestClassifier(
        n_estimators=200, max_depth=8, min_samples_leaf=20, random_state=42, n_jobs=-1
    )
    clf.fit(X_train, y_train)

    preds = clf.predict(X_test)
    proba = clf.predict_proba(X_test)[:, 1]
    acc = accuracy_score(y_test, preds)
    auc = roc_auc_score(y_test, proba)
    print(f"Train samples={len(X_train)}  test samples={len(X_test)}")
    print(f"Test accuracy={acc:.3f}  AUC={auc:.3f}")

    importances = dict(zip(FEATURE_NAMES, clf.feature_importances_))
    for name, imp in sorted(importances.items(), key=lambda kv: -kv[1]):
        print(f"  {name:24s} {imp:.3f}")

    os.makedirs(os.path.dirname(model_path), exist_ok=True)
    joblib.dump(clf, model_path)
    print(f"Saved model -> {model_path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--data", nargs="+", default=[DATA_PATH], help="One or more training CSVs to concatenate.")
    parser.add_argument("--model-path", default=MODEL_PATH)
    args = parser.parse_args()
    train(data_path=args.data, model_path=args.model_path)
