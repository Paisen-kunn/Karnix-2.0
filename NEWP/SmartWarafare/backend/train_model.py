"""Train the band-activity classifier used by MLPredictorScheduler.

Reads data/training_data.csv (see generate_training_data.py) and fits a
RandomForest classifier that predicts, from a band's own past-scan
features, the probability the band is transmitting right now. Saves the
fitted model to models/band_activity_model.joblib.

Usage:
    python3 generate_training_data.py   # once, to (re)build the dataset
    python3 train_model.py
"""
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


def load_dataset(path: str):
    X, y = [], []
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            X.append([float(row[name]) for name in FEATURE_NAMES])
            y.append(int(row["label"]))
    return np.array(X, dtype=float), np.array(y, dtype=int)


def train(data_path: str = DATA_PATH, model_path: str = MODEL_PATH) -> None:
    if not os.path.exists(data_path):
        raise SystemExit(f"No training data at {data_path} -- run generate_training_data.py first.")

    X, y = load_dataset(data_path)
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
    train()
