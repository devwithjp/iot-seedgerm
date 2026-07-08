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
MQTT_HOST=192.168.50.1
MQTT_PORT=1883
MQTT_USER=iota
MQTT_PASSWORD=REDACTED
SOIL_ENABLED=0
SOIL_PIN=32
DRY_RAW=3200
WET_RAW=1400
```

## Required edits

Before inserting the card into the ESP32-CAM, edit:

- `WIFI_SSID`
- `WIFI_PASSWORD`
- `NODE_NAME`
- `MQTT_USER` and `MQTT_PASSWORD` if you changed the Pi Mosquitto credentials
- `SOIL_ENABLED`, `SOIL_PIN`, `DRY_RAW`, and `WET_RAW` if testing a soil sensor on the ESP32-CAM

Use `NODE_NAME=germcam-1` for the first camera and `NODE_NAME=germcam-2` for the second camera.

## Optional soil sensor on ESP32-CAM

For the combined camera + capacitive soil moisture v2.0 test, wire:

```text
Sensor GND  -> ESP32-CAM GND
Sensor VCC  -> ESP32-CAM 3V3
Sensor AOUT -> ESP32-CAM GPIO32
```

Then add this to `germcam.cfg`:

```text
SOIL_ENABLED=1
SOIL_PIN=32
DRY_RAW=3200
WET_RAW=1400
```

The camera firmware then adds these fields to `/status`:

```text
soil_enabled
soil_pin
soil_raw
soil_moisture_percent
dry_raw
wet_raw
```

It also publishes:

```text
germination/germcam-1/soil_raw
germination/germcam-1/soil_moisture_percent
```

For the standalone Raspberry Pi hub, use:

```text
WIFI_SSID=REDACTED
MQTT_HOST=192.168.50.1
```

Ready-made examples are in:

```text
config/germcam_1_standalone.cfg
config/germcam_2_standalone.cfg
```

Copy the matching file to the ESP32-CAM microSD card as `germcam.cfg`.

If you go back to the older iPhone hotspot test setup, and the ESP32 is on the
hotspot while the Pi is not on that same Wi-Fi subnet, use the laptop forwarder
address instead:

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
Connecting to REDACTED
germcam-1 ready: http://<camera-ip>/
mDNS ready: http://germcam-1.local/
```

If it says `microSD not mounted` or `No /germcam.cfg`, the firmware uses the hardcoded sketch defaults instead.

## Status endpoint

The `/status` JSON reports whether the config came from the card:

```json
{
  "config_source": "sd"
}
```
