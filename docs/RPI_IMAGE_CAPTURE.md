# Raspberry Pi image capture

Use this after both ESP32-CAM boards are flashed and reachable on the network. The Raspberry Pi pulls JPEG images over HTTP and stores timestamped files locally. MQTT should continue to carry status and URLs, not image payloads.

## Files

```text
pi/capture_esp32_cam_images.py
```

The script uses only Python standard library modules.

## Quick test

From the project directory on the Raspberry Pi:

```bash
python3 pi/capture_esp32_cam_images.py
```

By default it captures:

```text
http://germcam-1.local/jpg
http://germcam-2.local/jpg
```

The ESP32-CAM firmware advertises these names with mDNS, and the standalone Pi
setup installs Avahi/mDNS support. If mDNS is not available on your network,
use the IP address printed in the ESP32 Serial Monitor.

Images are written under:

```text
images/YYYY-MM-DD/<camera-name>/<camera-name>_YYYYMMDD_HHMMSS.jpg
```

Each attempt is logged as JSON lines:

```text
logs/capture_events.jsonl
```

## If `.local` names do not work

Use the IP addresses printed in the ESP32-CAM Serial Monitor:

```bash
python3 pi/capture_esp32_cam_images.py \
  --camera germcam-1=http://192.168.1.42/jpg \
  --camera germcam-2=http://192.168.1.43/jpg
```

For a real trial, reserve fixed IP addresses for both cameras in your router so the URLs do not change.

## Capture repeatedly

Capture every 10 minutes:

```bash
python3 pi/capture_esp32_cam_images.py --interval 600
```

Stop it with `Ctrl+C`.

## Cron option

For scheduled captures without keeping a terminal open, run:

```bash
crontab -e
```

Add this line to capture every 10 minutes:

```cron
*/10 * * * * cd /home/jp/Desktop/IOT/project && python3 pi/capture_esp32_cam_images.py >> logs/capture_cron.log 2>&1
```

Adjust the project path if the files are copied to a different directory on the Raspberry Pi.

## Definition of done

- `images/` contains timestamped JPEGs from `germcam-1`.
- `images/` contains timestamped JPEGs from `germcam-2`.
- `logs/capture_events.jsonl` records successful capture events.
- Failed capture events show a useful error instead of silently skipping data.
