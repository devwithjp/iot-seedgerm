#!/usr/bin/env python3
"""Run an end-to-end health test for the ESP32-CAM germination node."""

from __future__ import annotations

import argparse
import json
import socket
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from tempfile import NamedTemporaryFile
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


def ok(message: str) -> None:
    print(f"PASS {message}")


def fail(message: str) -> None:
    print(f"FAIL {message}")


def warn(message: str) -> None:
    print(f"WARN {message}")


def fetch_json(url: str, timeout: float) -> dict[str, object]:
    request = Request(url, headers={"User-Agent": "germcam-test/1.0"})
    with urlopen(request, timeout=timeout) as response:
        data = response.read()
    return json.loads(data.decode("utf-8"))


def fetch_jpeg(url: str, timeout: float) -> bytes:
    request = Request(url, headers={"User-Agent": "germcam-test/1.0"})
    with urlopen(request, timeout=timeout) as response:
        content_type = response.headers.get("Content-Type", "")
        data = response.read()

    if not data.startswith(b"\xff\xd8"):
        raise ValueError("response did not start with JPEG magic bytes")
    if content_type and "jpeg" not in content_type.lower() and "jpg" not in content_type.lower():
        raise ValueError(f"unexpected content type: {content_type}")
    return data


def write_atomic(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with NamedTemporaryFile("wb", delete=False, dir=path.parent, prefix=f".{path.name}.") as tmp:
        tmp.write(data)
        tmp_path = Path(tmp.name)
    tmp_path.replace(path)


def check_tcp(host: str, port: int, timeout: float) -> None:
    with socket.create_connection((host, port), timeout=timeout):
        return


def run_mosquitto_sub(
    host: str,
    port: int,
    username: str,
    password: str,
    topic: str,
    count: int,
    timeout: int,
) -> subprocess.CompletedProcess[str]:
    command = [
        "mosquitto_sub",
        "-h",
        host,
        "-p",
        str(port),
        "-u",
        username,
        "-P",
        password,
        "-t",
        topic,
        "-v",
        "-C",
        str(count),
    ]
    return subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=timeout,
        check=False,
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Test ESP32-CAM HTTP, JPEG, MQTT, and forwarder health.")
    parser.add_argument("--node", default="germcam-1")
    parser.add_argument("--camera-host", default="172.20.10.5")
    parser.add_argument("--mqtt-host", default="10.42.0.75", help="Real broker host used by test subscriber.")
    parser.add_argument("--mqtt-port", type=int, default=1883)
    parser.add_argument("--forwarder-host", default="172.20.10.4")
    parser.add_argument("--forwarder-port", type=int, default=1883)
    parser.add_argument("--mqtt-user", default="iota")
    parser.add_argument("--mqtt-password", default="REDACTED")
    parser.add_argument("--timeout", type=float, default=8.0)
    parser.add_argument("--mqtt-timeout", type=int, default=12)
    parser.add_argument("--output-dir", type=Path, default=Path("images"))
    return parser


def main() -> int:
    args = build_parser().parse_args()
    failures = 0
    status_url = f"http://{args.camera_host}/status"
    jpg_url = f"http://{args.camera_host}/jpg"

    print(f"Testing {args.node} at {args.camera_host}")
    print(f"Broker: {args.mqtt_host}:{args.mqtt_port}")
    print(f"Forwarder: {args.forwarder_host}:{args.forwarder_port}")

    try:
        status = fetch_json(status_url, args.timeout)
        ok(f"HTTP status endpoint returned JSON: {status_url}")
        print(json.dumps(status, indent=2, sort_keys=True))
        if status.get("node") != args.node:
            failures += 1
            fail(f"status node expected {args.node!r}, got {status.get('node')!r}")
        if status.get("camera_ready") is not True:
            failures += 1
            fail("camera_ready is not true")
        if status.get("ip") != args.camera_host:
            warn(f"status IP is {status.get('ip')!r}, expected {args.camera_host!r}")
    except (HTTPError, URLError, TimeoutError, ValueError, json.JSONDecodeError, OSError) as exc:
        failures += 1
        fail(f"HTTP status check failed: {exc}")

    try:
        image = fetch_jpeg(jpg_url, args.timeout)
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        day = datetime.now().strftime("%Y-%m-%d")
        target = args.output_dir / day / args.node / f"{args.node}_test_{timestamp}.jpg"
        write_atomic(target, image)
        ok(f"JPEG capture saved {target} ({len(image)} bytes)")
    except (HTTPError, URLError, TimeoutError, ValueError, OSError) as exc:
        failures += 1
        fail(f"JPEG capture failed: {exc}")

    try:
        check_tcp(args.forwarder_host, args.forwarder_port, args.timeout)
        ok(f"forwarder TCP port is reachable at {args.forwarder_host}:{args.forwarder_port}")
    except OSError as exc:
        failures += 1
        fail(f"forwarder TCP check failed: {exc}")

    topic = f"germination/{args.node}/#"
    try:
        mqtt = run_mosquitto_sub(
            args.mqtt_host,
            args.mqtt_port,
            args.mqtt_user,
            args.mqtt_password,
            topic,
            count=4,
            timeout=args.mqtt_timeout,
        )
        if mqtt.returncode == 0 and mqtt.stdout.strip():
            ok(f"MQTT received messages from {topic}")
            print(mqtt.stdout.strip())
        else:
            failures += 1
            fail("MQTT subscriber did not receive expected messages")
            if mqtt.stderr.strip():
                print(mqtt.stderr.strip(), file=sys.stderr)
    except (subprocess.TimeoutExpired, FileNotFoundError) as exc:
        failures += 1
        fail(f"MQTT subscriber failed: {exc}")

    if failures:
        print(f"RESULT failed checks: {failures}")
        return 1

    print("RESULT all checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
