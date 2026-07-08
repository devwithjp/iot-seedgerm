#!/usr/bin/env python3
"""
Capture timestamped JPEG frames from ESP32-CAM HTTP endpoints.

The script is intentionally dependency-free so it can run on a Raspberry Pi
with the standard Python installation.
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from datetime import datetime
from pathlib import Path
from tempfile import NamedTemporaryFile
from typing import Iterable
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


DEFAULT_CAMERAS = (
    ("germcam-1", "http://germcam-1.local/jpg"),
    ("germcam-2", "http://germcam-2.local/jpg"),
)


def parse_camera(value: str) -> tuple[str, str]:
    if "=" not in value:
        raise argparse.ArgumentTypeError("camera must be formatted as name=url")

    name, url = value.split("=", 1)
    name = name.strip()
    url = url.strip()

    if not name:
        raise argparse.ArgumentTypeError("camera name cannot be empty")
    if not url.startswith(("http://", "https://")):
        raise argparse.ArgumentTypeError("camera URL must start with http:// or https://")

    return name, url


def fetch_jpeg(url: str, timeout: float) -> bytes:
    request = Request(url, headers={"User-Agent": "germination-chamber-capture/1.0"})
    with urlopen(request, timeout=timeout) as response:
        content_type = response.headers.get("Content-Type", "")
        data = response.read()

    if not data.startswith(b"\xff\xd8"):
        raise ValueError(f"response from {url} is not a JPEG")
    if content_type and "jpeg" not in content_type.lower() and "jpg" not in content_type.lower():
        raise ValueError(f"response from {url} has unexpected content type: {content_type}")

    return data


def write_atomic(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with NamedTemporaryFile("wb", delete=False, dir=path.parent, prefix=f".{path.name}.") as tmp:
        tmp.write(data)
        tmp_path = Path(tmp.name)
    tmp_path.replace(path)


def append_log(log_path: Path, event: dict[str, object]) -> None:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("a", encoding="utf-8") as log_file:
        log_file.write(json.dumps(event, separators=(",", ":")) + "\n")


def capture_once(
    cameras: Iterable[tuple[str, str]],
    output_dir: Path,
    timeout: float,
    log_path: Path,
) -> int:
    now = datetime.now()
    day_dir = output_dir / now.strftime("%Y-%m-%d")
    timestamp = now.strftime("%Y%m%d_%H%M%S")
    failures = 0

    for name, url in cameras:
        event: dict[str, object] = {
            "time": now.isoformat(timespec="seconds"),
            "camera": name,
            "url": url,
        }

        try:
            image = fetch_jpeg(url, timeout)
            target = day_dir / name / f"{name}_{timestamp}.jpg"
            write_atomic(target, image)
            event.update({"ok": True, "path": str(target), "bytes": len(image)})
            print(f"{name}: saved {target} ({len(image)} bytes)")
        except (HTTPError, URLError, TimeoutError, ValueError, OSError) as exc:
            failures += 1
            event.update({"ok": False, "error": str(exc)})
            print(f"{name}: failed: {exc}", file=sys.stderr)

        append_log(log_path, event)

    return failures


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Capture images from ESP32-CAM /jpg endpoints."
    )
    parser.add_argument(
        "--camera",
        action="append",
        type=parse_camera,
        dest="cameras",
        metavar="NAME=URL",
        help="Camera endpoint. Can be repeated. Defaults to germcam-1.local and germcam-2.local.",
    )
    parser.add_argument(
        "--output-dir",
        default="images",
        type=Path,
        help="Directory for captured JPEG files. Default: images",
    )
    parser.add_argument(
        "--log",
        default=Path("logs/capture_events.jsonl"),
        type=Path,
        help="JSONL capture log path. Default: logs/capture_events.jsonl",
    )
    parser.add_argument(
        "--timeout",
        default=10.0,
        type=float,
        help="HTTP timeout per camera in seconds. Default: 10",
    )
    parser.add_argument(
        "--interval",
        default=0,
        type=int,
        help="Repeat capture every N seconds. Default: 0 means capture once and exit.",
    )
    return parser


def main() -> int:
    args = build_parser().parse_args()
    cameras = tuple(args.cameras or DEFAULT_CAMERAS)

    if args.interval <= 0:
        return min(capture_once(cameras, args.output_dir, args.timeout, args.log), 1)

    while True:
        capture_once(cameras, args.output_dir, args.timeout, args.log)
        time.sleep(args.interval)


if __name__ == "__main__":
    raise SystemExit(main())
