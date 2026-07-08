#!/usr/bin/env python3
"""Forward TCP MQTT traffic from the iPhone hotspot subnet to the Pi broker."""

from __future__ import annotations

import argparse
import socket
import threading


def pipe(src: socket.socket, dst: socket.socket) -> None:
    try:
        while True:
            data = src.recv(4096)
            if not data:
                break
            dst.sendall(data)
    except OSError:
        pass
    finally:
        for sock in (src, dst):
            try:
                sock.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            try:
                sock.close()
            except OSError:
                pass


def serve(listen_host: str, listen_port: int, target_host: str, target_port: int) -> None:
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((listen_host, listen_port))
    server.listen(20)
    print(
        f"forwarding {listen_host}:{listen_port} -> {target_host}:{target_port}",
        flush=True,
    )

    while True:
        client, addr = server.accept()
        print(f"connection from {addr[0]}:{addr[1]}", flush=True)
        upstream = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            upstream.connect((target_host, target_port))
        except OSError as exc:
            print(f"upstream connect failed: {exc}", flush=True)
            client.close()
            upstream.close()
            continue

        threading.Thread(target=pipe, args=(client, upstream), daemon=True).start()
        threading.Thread(target=pipe, args=(upstream, client), daemon=True).start()


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Forward ESP32 MQTT traffic from this laptop to the Pi broker."
    )
    parser.add_argument("--listen-host", default="172.20.10.4")
    parser.add_argument("--listen-port", type=int, default=1883)
    parser.add_argument("--target-host", default="10.42.0.75")
    parser.add_argument("--target-port", type=int, default=1883)
    args = parser.parse_args()

    serve(args.listen_host, args.listen_port, args.target_host, args.target_port)


if __name__ == "__main__":
    main()
