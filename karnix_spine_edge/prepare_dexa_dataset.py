#!/usr/bin/env python3

import argparse
import csv
import random
import shutil
from pathlib import Path

from PIL import Image, ImageOps

SEED = 20260909
IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".tif", ".tiff", ".bmp", ".pgm"}
NEGATIVE_NAMES = ("normal", "healthy", "no_osteoporosis")
POSITIVE_NAMES = ("osteoporosis", "osteoporotic", "low_bmd")


def find_class_dir(split_dir, candidates):
    by_name = {child.name.lower(): child for child in split_dir.iterdir() if child.is_dir()}
    for candidate in candidates:
        if candidate in by_name:
            return by_name[candidate]
    raise RuntimeError(
        f"Missing class directory in {split_dir}; expected one of {', '.join(candidates)}"
    )


def images_in(directory):
    return sorted(path for path in directory.rglob("*") if path.suffix.lower() in IMAGE_EXTENSIONS)


def convert_image(source, destination):
    with Image.open(source) as image:
        image = ImageOps.exif_transpose(image).convert("L")
        image = ImageOps.pad(image, (64, 64), method=Image.Resampling.LANCZOS, color=0)
        destination.parent.mkdir(parents=True, exist_ok=True)
        image.save(destination, format="PPM")


def prepare_split(source_dir, output_dir, split, count, rng):
    split_source = source_dir / split
    if not split_source.is_dir():
        raise RuntimeError(f"Missing DEXA split directory: {split_source}")
    negative_dir = find_class_dir(split_source, NEGATIVE_NAMES)
    positive_dir = find_class_dir(split_source, POSITIVE_NAMES)
    negative = images_in(negative_dir)
    positive = images_in(positive_dir)
    per_class = count // 2
    if len(negative) < per_class or len(positive) < per_class:
        raise RuntimeError(
            f"DEXA {split} requires {per_class} images per class; "
            f"found {len(negative)} negative and {len(positive)} positive"
        )
    rng.shuffle(negative)
    rng.shuffle(positive)
    selected = [(path, 0, "Normal BMD") for path in negative[:per_class]]
    selected += [(path, 1, "Osteoporosis") for path in positive[:per_class]]
    rng.shuffle(selected)

    rows = []
    for index, (source, label, diagnosis) in enumerate(selected):
        class_name = "normal_bmd" if label == 0 else "osteoporosis"
        destination = output_dir / split / class_name / f"{class_name}_{index:04d}.pgm"
        convert_image(source, destination)
        relative = destination.relative_to(output_dir.parent.parent)
        rows.append((str(relative), label, diagnosis, split, str(source)))
    with (output_dir / f"{split}.csv").open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(["path", "label", "diagnosis", "split", "source_file"])
        writer.writerows(rows)
    return rows


def main():
    parser = argparse.ArgumentParser(
        description="Prepare a genuine labeled DEXA image dataset for the Karnix C++ CNN"
    )
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--count", type=int, default=600)
    parser.add_argument("--source-name", required=True)
    parser.add_argument("--source-url", required=True)
    parser.add_argument("--license", required=True)
    args = parser.parse_args()

    if not args.source.is_dir():
        raise SystemExit(f"DEXA source directory not found: {args.source}")
    if args.count < 100 or args.count % 10 != 0:
        raise SystemExit("Count must be at least 100 and divisible by 10")

    output = args.output.resolve()
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)
    rng = random.Random(SEED)
    train_count = args.count * 8 // 10
    test_count = args.count - train_count
    train_rows = prepare_split(args.source.resolve(), output, "train", train_count, rng)
    test_rows = prepare_split(args.source.resolve(), output, "test", test_count, rng)

    screen = output / "screen"
    screen.mkdir()
    negative = [row for row in test_rows if row[1] == 0][:5]
    positive = [row for row in test_rows if row[1] == 1][:5]
    screen_rows = negative + positive
    for row in screen_rows:
        source = output.parent.parent / row[0]
        shutil.copy2(source, screen / source.name)
    with (output / "screen.csv").open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(["path", "label", "diagnosis", "split", "source_file"])
        writer.writerows(screen_rows)

    (output / "SOURCE.txt").write_text(
        f"Dataset: {args.source_name}\n"
        f"Source URL: {args.source_url}\n"
        f"License: {args.license}\n"
        f"Prepared images: {args.count}\n"
        f"Train images: {len(train_rows)}\n"
        f"Test images: {len(test_rows)}\n"
        "Required modality: genuine DXA/DEXA scan exports, not ordinary radiographs.\n"
        "Binary labels: 0 = Normal BMD; 1 = Osteoporosis/Low BMD.\n",
        encoding="utf-8",
    )
    print(f"Prepared {args.count} genuine DEXA images in {output}")


if __name__ == "__main__":
    main()
