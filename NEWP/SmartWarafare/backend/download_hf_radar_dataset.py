"""Download a slice of the Turing Synthetic Radar Dataset (TSRD) from
Hugging Face -- the real-world-style dataset referenced in the SIH problem
statement (JC Wise / Alan Turing Institute radar emitter database):

    https://huggingface.co/datasets/alan-turing-institute/turing-synthetic-radar-dataset

The dataset is ~70GB and *gated*: you must accept its terms on the
Hugging Face website before it can be downloaded, even with a token. Steps:

    1. Log in at https://huggingface.co with your Hugging Face account.
    2. Open the dataset page above and click "Agree and access repository".
    3. Create a read-scoped access token: https://huggingface.co/settings/tokens
    4. In this shell:  huggingface-cli login   (or `export HF_TOKEN=hf_xxx`)
    5. Run this script.

We only pull a handful of files from the "scan" split (the receiver-sweep
mode, which matches this project's scanning-receiver scenario) since the
full dataset is far larger than needed for this demo.

Usage:
    python3 download_hf_radar_dataset.py --split val_scan --num-files 5
"""
import argparse
import os
import sys

from huggingface_hub import HfApi, hf_hub_download
from huggingface_hub.utils import GatedRepoError, HfHubHTTPError

REPO_ID = "alan-turing-institute/turing-synthetic-radar-dataset"
OUT_DIR = os.path.join(os.path.dirname(__file__), "data", "hf_radar_raw")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--split", default="val_scan", choices=["train_scan", "val_scan", "test_scan"],
                         help="Which scan-mode split to pull files from (val_scan is smallest).")
    parser.add_argument("--num-files", type=int, default=5, help="How many pulse-train files to download.")
    parser.add_argument("--out-dir", default=OUT_DIR)
    args = parser.parse_args()

    token = os.environ.get("HF_TOKEN")  # falls back to the huggingface-cli login cache if unset
    api = HfApi(token=token)

    print(f"Listing files in {REPO_ID} ...")
    try:
        all_files = api.list_repo_files(REPO_ID, repo_type="dataset")
    except (GatedRepoError, HfHubHTTPError) as e:
        _print_gated_help(e)
        sys.exit(1)

    candidates = sorted(f for f in all_files if f.startswith(f"scan/{args.split}/"))
    if not candidates:
        print(f"No files found under scan/{args.split}/")
        sys.exit(1)

    todo = candidates[: args.num_files]
    os.makedirs(args.out_dir, exist_ok=True)
    print(f"Downloading {len(todo)} of {len(candidates)} files from scan/{args.split}/ -> {args.out_dir}")

    downloaded = []
    for name in todo:
        try:
            path = hf_hub_download(
                repo_id=REPO_ID, repo_type="dataset", filename=name,
                local_dir=args.out_dir, token=token,
            )
            downloaded.append(path)
            print(f"  ok  {name}")
        except (GatedRepoError, HfHubHTTPError) as e:
            _print_gated_help(e)
            sys.exit(1)

    print(f"Downloaded {len(downloaded)} files. Next: python3 convert_hf_radar_dataset.py")


def _print_gated_help(e: Exception) -> None:
    print(f"\nCould not access {REPO_ID}: {e}\n")
    print(
        "This dataset is gated. To fix:\n"
        "  1. Log in at https://huggingface.co and open:\n"
        f"     https://huggingface.co/datasets/{REPO_ID.split('/', 1)[1]}\n"
        "     (via https://huggingface.co/datasets/alan-turing-institute/turing-synthetic-radar-dataset)\n"
        "  2. Click 'Agree and access repository'.\n"
        "  3. Create a read token at https://huggingface.co/settings/tokens\n"
        "  4. Run `huggingface-cli login` (paste the token) or `export HF_TOKEN=hf_xxx`\n"
        "  5. Re-run this script.\n"
    )


if __name__ == "__main__":
    main()
