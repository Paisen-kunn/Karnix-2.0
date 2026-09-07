"""Flask JSON API backing the React dashboard.

Accepts a raw sonar image (+ optional ping metadata JSON), runs the detection pipeline,
and returns detections plus URLs for the overlay image and downloadable reports.

Security notes: uploads are size/type validated, given random UUID-based filenames, and
processed inside an isolated per-job directory (no user-controlled paths ever reach the
filesystem), to avoid path traversal / arbitrary file writes.

Supports both classical CV and YOLO detection backends.
"""
from __future__ import annotations

import json
import sys
import uuid
from pathlib import Path

import cv2
import numpy as np
from flask import Flask, jsonify, request, send_file
from flask_cors import CORS
from werkzeug.exceptions import BadRequest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from NEWP.underwater.src.pipeline import run_pipeline  # noqa: E402
from NEWP.underwater.src.reporting.report_generator import to_csv, to_json  # noqa: E402

ALLOWED_IMAGE_EXT = {".png", ".jpg", ".jpeg", ".tif", ".tiff", ".bmp"}
MAX_UPLOAD_BYTES = 25 * 1024 * 1024  # 25 MB

JOBS_DIR = Path(__file__).resolve().parent / "jobs"
JOBS_DIR.mkdir(parents=True, exist_ok=True)

app = Flask(__name__)
app.config["MAX_CONTENT_LENGTH"] = MAX_UPLOAD_BYTES
CORS(app)


def _job_dir(job_id: str) -> Path:
    """Resolve a job directory from a server-generated UUID only (never user input)."""
    return JOBS_DIR / job_id


@app.post("/api/analyze")
def analyze():
    if "image" not in request.files:
        raise BadRequest("Missing 'image' file field")
    image_file = request.files["image"]
    ext = Path(image_file.filename or "").suffix.lower()
    if ext not in ALLOWED_IMAGE_EXT:
        raise BadRequest(f"Unsupported image type '{ext}'. Allowed: {sorted(ALLOWED_IMAGE_EXT)}")

    image_bytes = np.frombuffer(image_file.read(), dtype=np.uint8)
    image_bgr = cv2.imdecode(image_bytes, cv2.IMREAD_COLOR)
    if image_bgr is None:
        raise BadRequest("Could not decode image")

    metadata = None
    if "metadata" in request.files and request.files["metadata"].filename:
        try:
            metadata = json.loads(request.files["metadata"].read().decode("utf-8"))
        except (json.JSONDecodeError, UnicodeDecodeError) as exc:
            raise BadRequest(f"Invalid metadata JSON: {exc}") from exc

    range_m = float(request.form.get("range_m", 30.0))
    min_confidence = float(request.form.get("min_confidence", 0.35))
    detector = request.form.get("detector", "yolo").lower()

    if detector not in ("yolo", "classical"):
        raise BadRequest(f"Invalid detector '{detector}'. Use 'yolo' or 'classical'")

    result = run_pipeline(
        image_bgr, metadata, range_m=range_m,
        min_confidence=min_confidence, source_image=image_file.filename,
        detector=detector,
    )

    job_id = uuid.uuid4().hex
    job_dir = _job_dir(job_id)
    job_dir.mkdir(parents=True, exist_ok=True)
    cv2.imwrite(str(job_dir / "overlay.png"), result["overlay_bgr"])
    (job_dir / "report.json").write_text(to_json(result["records"]))
    (job_dir / "report.csv").write_text(to_csv(result["records"]))

    return jsonify({
        "job_id": job_id,
        "detector": detector,
        "detections": result["records"],
        "raw_count": result["raw_count"],
        "kept_count": result["kept_count"],
        "overlay_url": f"/api/jobs/{job_id}/overlay.png",
        "report_json_url": f"/api/jobs/{job_id}/report.json",
        "report_csv_url": f"/api/jobs/{job_id}/report.csv",
    })


@app.get("/api/jobs/<job_id>/overlay.png")
def get_overlay(job_id: str):
    path = _job_dir(job_id) / "overlay.png"
    if not path.is_file():
        return jsonify({"error": "not found"}), 404
    return send_file(path, mimetype="image/png")


@app.get("/api/jobs/<job_id>/report.json")
def get_report_json(job_id: str):
    path = _job_dir(job_id) / "report.json"
    if not path.is_file():
        return jsonify({"error": "not found"}), 404
    return send_file(path, mimetype="application/json", as_attachment=True, download_name="anomaly_report.json")


@app.get("/api/jobs/<job_id>/report.csv")
def get_report_csv(job_id: str):
    path = _job_dir(job_id) / "report.csv"
    if not path.is_file():
        return jsonify({"error": "not found"}), 404
    return send_file(path, mimetype="text/csv", as_attachment=True, download_name="anomaly_report.csv")


@app.get("/api/health")
def health():
    return jsonify({"status": "ok"})


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5001, debug=True)
