# ESP32-CAM microSD config

The ESP32-CAM firmware can read `/germcam.cfg` from a FAT-formatted microSD card. This avoids reflashing the board just to change Wi-Fi, MQTT, or node-name settings.

## Card contents

The card should contain:

```text
germcam.cfg
README.txt
```

Example `germcam.cfg`:

```text
NODE_NAME=germcam-1
WIFI_SSID=REDACTED
WIFI_PASSWORD=REDACTED
MQTT_HOST=10.42.0.75
MQTT_PORT=1883
MQTT_USER=
MQTT_PASSWORD=
```

## Required edits

Before inserting the card into the ESP32-CAM, edit:

- `WIFI_SSID`
- `WIFI_PASSWORD`
- `NODE_NAME`

Use `NODE_NAME=germcam-1` for the first camera and `NODE_NAME=germcam-2` for the second camera.

`MQTT_HOST=10.42.0.75` matches the Raspberry Pi/Home Assistant box detected on this machine. If the ESP32 is on REDACTED hotspot and the Pi is not on that same Wi-Fi subnet, use the laptop forwarder address instead:

```text
MQTT_HOST=172.20.10.4
```

See [IPHONE_HOTSPOT_MQTT_FORWARDER.md](IPHONE_HOTSPOT_MQTT_FORWARDER.md).

If Mosquitto requires authentication, fill in `MQTT_USER` and `MQTT_PASSWORD`. `MQTT connect failed, rc=5` in Serial Monitor means the ESP32 reached the broker, but the broker rejected the supplied credentials.

## Boot checklist

1. Safely eject the microSD card from the computer.
2. Insert it into the ESP32-CAM.
3. Make sure `GPIO0` is not connected to `GND`.
4. Press `RST`.
5. Open Serial Monitor at `115200`.

Expected output includes:

```text
Loaded /germcam.cfg from microSD
Connecting to <your-wifi>
germcam-1 ready: http://<camera-ip>/
```

If it says `microSD not mounted` or `No /germcam.cfg`, the firmware uses the hardcoded sketch defaults instead.

## Status endpoint

The `/status` JSON reports whether the config came from the card:

```json
{
  "config_source": "sd"
}
```
