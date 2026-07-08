#!/usr/bin/env python3
"""Collect serial soil-moisture readings for dry/wet calibration."""

from __future__ import annotations

import argparse
import csv
import re
import statistics
import sys
import time
from pathlib import Path

import serial


SOIL_RE = re.compile(r"soil GPIO(?P<gpio>\d+) raw=(?P<raw>\d+) moisture=(?P<moisture>\d+)%")
SCAN_RE = re.compile(r"GPIO(?P<gpio>\d+)=(?P<raw>\d+)")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Record ESP32 soil sensor serial readings for calibration."
    )
    parser.add_argument("--port", default="/dev/ttyUSB0", help="Serial port to read.")
    parser.add_argument("--baud", type=int, default=115200, help="Serial baud rate.")
    parser.add_argument(
        "--seconds", type=float, default=60.0, help="How long to collect readings."
    )
    parser.add_argument(
        "--label",
        required=True,
        help="Calibration condition label, for example dry, wet, moist, air.",
    )
    parser.add_argument(
        "--expected-pin",
        type=int,
        default=34,
        help="Expected active GPIO. Warnings are printed if firmware reports another pin.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("project/data/calibration/soil_calibration.csv"),
        help="CSV file to append readings to.",
    )
    return parser.parse_args()


def summarize(values: list[int]) -> str:
    if not values:
        return "no active soil readings captured"

    parts = [
        f"count={len(values)}",
        f"min={min(values)}",
        f"max={max(values)}",
        f"mean={statistics.fmean(values):.1f}",
        f"median={statistics.median(values):.1f}",
    ]
    if len(values) > 1:
        parts.append(f"stdev={statistics.stdev(values):.1f}")
    return " ".join(parts)


def main() -> int:
    args = parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)

    fieldnames = [
        "timestamp",
        "label",
        "active_gpio",
        "soil_raw",
        "moisture_percent",
        "scan_gpio32",
        "scan_gpio33",
        "scan_gpio34",
        "scan_gpio35",
        "scan_gpio36",
        "scan_gpio39",
    ]

    soil_values: list[int] = []
    last_soil: dict[str, int] | None = None
    wrote_header = not args.output.exists() or args.output.stat().st_size == 0

    print(
        f"Collecting {args.label!r} readings from {args.port} for {args.seconds:g}s..."
    )
    print(f"Appending CSV rows to {args.output}")

    with serial.Serial(args.port, args.baud, timeout=0.5) as ser, args.output.open(
        "a", newline=""
    ) as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        if wrote_header:
            writer.writeheader()

        end_at = time.time() + args.seconds
        while time.time() < end_at:
            raw_line = ser.readline()
            if not raw_line:
                continue

            line = raw_line.decode("utf-8", "replace").strip()
            if not line:
                continue

            soil_match = SOIL_RE.search(line)
            if soil_match:
                active_gpio = int(soil_match.group("gpio"))
                soil_raw = int(soil_match.group("raw"))
                moisture = int(soil_match.group("moisture"))
                last_soil = {
                    "active_gpio": active_gpio,
                    "soil_raw": soil_raw,
                    "moisture_percent": moisture,
                }
                soil_values.append(soil_raw)
                if active_gpio != args.expected_pin:
                    print(
                        f"warning: firmware reports GPIO{active_gpio}, "
                        f"expected GPIO{args.expected_pin}",
                        file=sys.stderr,
                    )
                print(line)
                continue

            if line.startswith("adc scan:") and last_soil is not None:
                scan = {f"scan_gpio{pin}": "" for pin in (32, 33, 34, 35, 36, 39)}
                for match in SCAN_RE.finditer(line):
                    scan[f"scan_gpio{match.group('gpio')}"] = int(match.group("raw"))

                writer.writerow(
                    {
                        "timestamp": time.strftime(
                            "%Y-%m-%dT%H:%M:%S%z", time.localtime()
                        ),
                        "label": args.label,
                        **last_soil,
                        **scan,
                    }
                )
                csv_file.flush()
                print(line)

    print(f"{args.label}: {summarize(soil_values)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
