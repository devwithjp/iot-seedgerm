#!/usr/bin/env python3
"""Fetch ESP32 status JSON over HTTP and publish selected fields to MQTT."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


def fetch_status(url: str, timeout: float) -> dict[str, object]:
    request = Request(url, headers={"User-Agent": "germination-poll-test/1.0"})
    with urlopen(request, timeout=timeout) as response:
        return json.loads(response.read().decode("utf-8"))


def publish(
    host: str,
    port: int,
    username: str,
    password: str,
    topic: str,
    payload: str,
    retain: bool,
) -> None:
    command = [
        "mosquitto_pub",
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
        "-m",
        payload,
    ]
    if retain:
        command.append("-r")

    subprocess.run(command, check=True)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Poll ESP32 /status and publish soil values to MQTT."
    )
    parser.add_argument(
        "--status-url",
        default="http://172.20.10.5/status",
        help="ESP32 /status endpoint. Default: http://172.20.10.5/status",
    )
    parser.add_argument(
        "--mqtt-host",
        default="172.20.10.7",
        help="MQTT broker host. Default: 172.20.10.7",
    )
    parser.add_argument(
        "--mqtt-port",
        type=int,
        default=1883,
        help="MQTT broker port. Default: 1883",
    )
    parser.add_argument(
        "--mqtt-user",
        default="iota",
        help="MQTT username. Default: iota",
    )
    parser.add_argument(
        "--mqtt-password",
        default="REDACTED",
        help="MQTT password. Default: REDACTED",
    )
    parser.add_argument(
        "--topic-prefix",
        default="germination/germcam-1",
        help="Topic prefix. Default: germination/germcam-1",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=8.0,
        help="HTTP timeout in seconds. Default: 8",
    )
    parser.add_argument(
        "--retain",
        action="store_true",
        help="Publish retained MQTT messages.",
    )
    return parser


def main() -> int:
    args = build_parser().parse_args()

    try:
        status = fetch_status(args.status_url, args.timeout)
    except (HTTPError, URLError, TimeoutError, OSError, json.JSONDecodeError) as exc:
        print(f"fetch failed: {exc}", file=sys.stderr)
        return 1

    missing = [
        field
        for field in ("soil_raw", "soil_moisture_percent", "soil_enabled", "soil_pin")
        if field not in status
    ]
    if missing:
        print(f"status missing required fields: {', '.join(missing)}", file=sys.stderr)
        return 1

    try:
        publish(
            args.mqtt_host,
            args.mqtt_port,
            args.mqtt_user,
            args.mqtt_password,
            f"{args.topic_prefix}/status",
            json.dumps(status, separators=(",", ":")),
            args.retain,
        )
        publish(
            args.mqtt_host,
            args.mqtt_port,
            args.mqtt_user,
            args.mqtt_password,
            f"{args.topic_prefix}/soil_raw",
            str(status["soil_raw"]),
            args.retain,
        )
        publish(
            args.mqtt_host,
            args.mqtt_port,
            args.mqtt_user,
            args.mqtt_password,
            f"{args.topic_prefix}/soil_moisture_percent",
            str(status["soil_moisture_percent"]),
            args.retain,
        )
    except subprocess.CalledProcessError as exc:
        print(f"mqtt publish failed: {exc}", file=sys.stderr)
        return exc.returncode or 1

    print(
        "published "
        f"soil_raw={status['soil_raw']} "
        f"soil_moisture_percent={status['soil_moisture_percent']} "
        f"from {args.status_url} to {args.topic_prefix}",
        flush=True,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
