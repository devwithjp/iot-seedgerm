# ESP32 soil moisture node

Use this for the separate ESP32 reading the analog capacitive soil moisture sensor. Do not connect the analog sensor to GPIO34 on an ESP32-CAM while the camera is active, because GPIO34 is a camera data pin on the Freenove ESP32-WROVER CAM pinout.

## Wiring

```text
Sensor GND  -> ESP32 GND
Sensor VCC  -> ESP32 3V3
Sensor AOUT -> ESP32 GPIO34, or GPIO32 if GPIO34 reads 0
```

Use 3.3 V power for the sensor so its analog output stays in the ESP32-safe range.

## Firmware

Upload:

```text
esp32/esp32_soil_node/esp32_soil_node.ino
```

The node reads the configured soil pin, serves `/status` over HTTP, and publishes MQTT readings every 30 seconds. If the configured pin is effectively dead, the firmware scans GPIO32, GPIO33, GPIO34, GPIO35, GPIO36, and GPIO39, then switches to the strongest detected ADC signal.

## microSD config

The firmware reads this file from a FAT-formatted card:

```text
germsensor.cfg
```

Example:

```text
NODE_NAME=germsoil-1
WIFI_SSID=REDACTED
WIFI_PASSWORD=REDACTED
MQTT_HOST=172.20.10.4
MQTT_PORT=1883
MQTT_USER=<mqtt-user>
MQTT_PASSWORD=REDACTED
SOIL_PIN=34
DRY_RAW=3200
WET_RAW=1400
```

`MQTT_HOST=172.20.10.4` matches the current iPhone-hotspot forwarder setup. If the ESP32 and Raspberry Pi are later moved to the same network, use the Pi broker IP instead.

## MQTT topics

```text
germination/germsoil-1/availability
germination/germsoil-1/status
germination/germsoil-1/soil_raw
germination/germsoil-1/soil_moisture_percent
```

## Calibration

Start with raw readings:

1. Leave the sensor dry or in dry substrate and record `soil_raw`.
2. Put the sensor in wet substrate and record `soil_raw`.
3. Set `DRY_RAW` and `WET_RAW` in `germsensor.cfg`.

The firmware maps those two values to 0-100%:

```text
DRY_RAW -> 0%
WET_RAW -> 100%
```

If serial output shows `Configured GPIO34 reads 0; using GPIO32 detected at <value>`, update `SOIL_PIN=32` in `germsensor.cfg` when convenient. The firmware will still run without that config change.

## Test

Serial Monitor at 115200 should print lines like:

```text
Loaded /germsensor.cfg from microSD
germsoil-1 ready: http://<ip>/
soil raw=2430 moisture=42%
```

From the laptop or Pi:

```bash
mosquitto_sub -h 10.42.0.75 -u iota -P '<password>' -t 'germination/germsoil-1/#' -v
```

Or open:

```text
http://<soil-node-ip>/status
```
