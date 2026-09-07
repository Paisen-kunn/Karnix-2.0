#!/usr/bin/env python3
"""CLI script to download and convert open-source sonar datasets from Awesome-Sonar-Image-Resources.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

# Ensure project root is in sys.path
PROJECT_ROOT = Path(__file__).resolve().parent.parent
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from src.training.dataset_downloader import DATASET_REPOS, download_and_prepare_dataset


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Download and format benchmark sonar datasets from Awesome-Sonar-Image-Resources"
    )
    parser.add_argument(
        "--dataset",
        choices=["all"] + list(DATASET_REPOS.keys()),
        default="sctd",
        help="Dataset key to download or 'all'",
    )
    parser.add_argument(
        "--data-dir",
        default="data",
        help="Base directory to store downloaded and formatted datasets",
    )

    args = parser.parse_args()

    target_dir = PROJECT_ROOT / args.data_dir

    if args.dataset == "all":
        for key in DATASET_REPOS:
            print(f"\n--- Downloading {key} ---")
            download_and_prepare_dataset(key, data_dir=target_dir)
    else:
        download_and_prepare_dataset(args.dataset, data_dir=target_dir)

    print("\nDataset preparation finished successfully!")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
