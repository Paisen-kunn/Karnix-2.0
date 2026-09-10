#!/usr/bin/env python3

import argparse
import csv
import io
import random
import shutil
import zipfile
from collections import Counter
from pathlib import Path

from PIL import Image, ImageOps

SOURCE_URL = "https://www.kaggle.com/api/v1/datasets/download/abdullahkhan70/lumbar-spinal-mri-dataset"
SOURCE_PAGE = "https://www.kaggle.com/datasets/abdullahkhan70/lumbar-spinal-mri-dataset"
SOURCE_DOI = "10.34740/KAGGLE/DSV/14332076"
LICENSE = "CC0 Public Domain"
SEED = 20260909


def members_for(archive, split, class_name):
    marker = f"/{split}/{class_name}/".lower()
    return sorted(
        name for name in archive.namelist()
        if marker in name.lower()
        and name.lower().endswith((".jpg", ".jpeg", ".png"))
    )


def choose_samples(archive, split, per_class, rng):
    normal = [(name, 0, "No Stenosis") for name in members_for(archive, split, "No Stenosis")]
    herniated = [(name, 1, "Herniated Disc") for name in members_for(archive, split, "Herniated Disc")]
    thecal = [(name, 1, "Thecal Sac") for name in members_for(archive, split, "Thecal Sac")]
    pathology = herniated + thecal
    rng.shuffle(normal)
    rng.shuffle(pathology)
    if len(normal) < per_class or len(pathology) < per_class:
        raise RuntimeError(f"Not enough {split} MRI images for {per_class} samples per class")
    selected = normal[:per_class] + pathology[:per_class]
    rng.shuffle(selected)
    return selected


def convert_member(archive, member, destination):
    with archive.open(member) as source:
        with Image.open(io.BytesIO(source.read())) as image:
            image = ImageOps.exif_transpose(image).convert("L")
            image = ImageOps.pad(image, (64, 64), method=Image.Resampling.LANCZOS, color=0)
            destination.parent.mkdir(parents=True, exist_ok=True)
            image.save(destination, format="PPM")


def write_manifest(path, rows):
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(["path", "label", "diagnosis", "split", "source_member"])
        writer.writerows(rows)


def prepare(archive_path, output_dir, count):
    if count < 100 or count % 10 != 0:
        raise ValueError("Image count must be at least 100 and divisible by 10")

    train_count = count * 8 // 10
    test_count = count - train_count
    rng = random.Random(SEED)

    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True)

    manifests = {}
    with zipfile.ZipFile(archive_path) as archive:
        for split, split_count in (("train", train_count), ("test", test_count)):
            selected = choose_samples(archive, split, split_count // 2, rng)
            rows = []
            for index, (member, label, diagnosis) in enumerate(selected):
                class_name = "no_stenosis" if label == 0 else "stenosis"
                subtype = diagnosis.lower().replace(" ", "_")
                filename = f"{class_name}_{index:04d}_{subtype}.pgm"
                destination = output_dir / split / class_name / filename
                convert_member(archive, member, destination)
                relative = destination.relative_to(output_dir.parent.parent)
                rows.append((str(relative), label, diagnosis, split, member))
            write_manifest(output_dir / f"{split}.csv", rows)
            manifests[split] = rows

    screen_dir = output_dir / "screen"
    screen_dir.mkdir()
    normal_screen = [row for row in manifests["test"] if row[1] == 0][:5]
    pathology_screen = [row for row in manifests["test"] if row[1] == 1][:5]
    screen_rows = normal_screen + pathology_screen
    for row in screen_rows:
        source = output_dir.parent.parent / row[0]
        shutil.copy2(source, screen_dir / source.name)
    write_manifest(output_dir / "screen.csv", screen_rows)

    counts = Counter((row[3], row[1]) for rows in manifests.values() for row in rows)
    subtypes = Counter(row[2] for rows in manifests.values() for row in rows)
    (output_dir / "SOURCE.txt").write_text(
        "Diagnosis Lumbar Spinal Patients MRI Dataset\n"
        f"Source page: {SOURCE_PAGE}\n"
        f"Download API: {SOURCE_URL}\n"
        f"DOI: {SOURCE_DOI}\n"
        f"License reported by Kaggle: {LICENSE}\n"
        f"Deterministic selection seed: {SEED}\n"
        f"Prepared images: {count}\n"
        "Binary labels: 0 = No Stenosis; 1 = Herniated Disc or Thecal Sac Compression\n"
        f"Train: {counts[('train', 0)]} No Stenosis, {counts[('train', 1)]} Pathology\n"
        f"Test: {counts[('test', 0)]} No Stenosis, {counts[('test', 1)]} Pathology\n"
        f"Selected subtypes: {dict(subtypes)}\n",
        encoding="utf-8",
    )

    print(f"Prepared {count} lumbar MRI images in {output_dir}")
    print(f"Train manifest: {output_dir / 'train.csv'} ({train_count} images)")
    print(f"Test manifest:  {output_dir / 'test.csv'} ({test_count} images)")


def main():
    parser = argparse.ArgumentParser(description="Prepare balanced lumbar MRI data for Karnix")
    parser.add_argument("--archive", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--count", type=int, default=600)
    args = parser.parse_args()
    if not args.archive.is_file():
        raise SystemExit(f"MRI archive not found: {args.archive}")
    prepare(args.archive.resolve(), args.output.resolve(), args.count)


if __name__ == "__main__":
    main()
