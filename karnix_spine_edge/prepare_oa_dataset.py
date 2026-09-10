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

SOURCE_URL = "https://www.kaggle.com/api/v1/datasets/download/shashwatwork/knee-osteoarthritis-dataset-with-severity"
SOURCE_PAGE = "https://www.kaggle.com/datasets/shashwatwork/knee-osteoarthritis-dataset-with-severity"
LICENSE = "CC BY 4.0"
SEED = 20260909


def image_members(archive, split, grade):
    prefix = f"{split}/{grade}/"
    return sorted(
        name for name in archive.namelist()
        if name.startswith(prefix)
        and name.lower().endswith((".png", ".jpg", ".jpeg"))
    )


def choose_samples(archive, split, per_class, rng):
    normal = image_members(archive, split, 0)
    oa = []
    for grade in (2, 3, 4):
        oa.extend((name, grade) for name in image_members(archive, split, grade))
    normal_samples = [(name, 0, 0) for name in normal]
    oa_samples = [(name, 1, grade) for name, grade in oa]
    rng.shuffle(normal_samples)
    rng.shuffle(oa_samples)
    if len(normal_samples) < per_class or len(oa_samples) < per_class:
        raise RuntimeError(f"Not enough {split} images for {per_class} samples per class")
    selected = normal_samples[:per_class] + oa_samples[:per_class]
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
        writer.writerow(["path", "label", "grade", "split", "source_member"])
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
            for index, (member, label, grade) in enumerate(selected):
                class_name = "no_oa" if label == 0 else "oa"
                filename = f"{class_name}_{index:04d}_grade{grade}.pgm"
                destination = output_dir / split / class_name / filename
                convert_member(archive, member, destination)
                relative = destination.relative_to(output_dir.parent.parent)
                rows.append((str(relative), label, grade, split, member))
            manifest = output_dir / f"{split}.csv"
            write_manifest(manifest, rows)
            manifests[split] = rows

    screen_dir = output_dir / "screen"
    screen_dir.mkdir()
    for row in manifests["test"][:20]:
        source = output_dir.parent.parent / row[0]
        shutil.copy2(source, screen_dir / source.name)

    counts = Counter((row[3], row[1]) for rows in manifests.values() for row in rows)
    attribution = output_dir / "SOURCE.txt"
    attribution.write_text(
        "Knee Osteoarthritis Dataset with Severity Grading\n"
        f"Source page: {SOURCE_PAGE}\n"
        f"Download API: {SOURCE_URL}\n"
        f"License reported by Kaggle API: {LICENSE}\n"
        f"Deterministic selection seed: {SEED}\n"
        f"Prepared images: {count}\n"
        "Labels: 0 = KL grade 0 (No OA), 1 = KL grades 2-4 (Definite OA)\n"
        "KL grade 1 is excluded because it is radiographically doubtful OA.\n"
        f"Train: {counts[('train', 0)]} No OA, {counts[('train', 1)]} OA\n"
        f"Test: {counts[('test', 0)]} No OA, {counts[('test', 1)]} OA\n",
        encoding="utf-8",
    )

    print(f"Prepared {count} images in {output_dir}")
    print(f"Train manifest: {output_dir / 'train.csv'} ({train_count} images)")
    print(f"Test manifest:  {output_dir / 'test.csv'} ({test_count} images)")


def main():
    parser = argparse.ArgumentParser(description="Prepare a balanced OA subset for Karnix Spine Edge")
    parser.add_argument("--archive", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--count", type=int, default=600)
    args = parser.parse_args()

    if not args.archive.is_file():
        raise SystemExit(f"Dataset archive not found: {args.archive}")
    prepare(args.archive.resolve(), args.output.resolve(), args.count)


if __name__ == "__main__":
    main()
