# ESP32 soil moisture node

Use this for the separate ESP32 reading the analog capacitive soil moisture sensor. The current target sensor is the capacitive soil moisture sensor v2.0 wired to GPIO32.

Do not connect the analog sensor to an ESP32-CAM while the camera is active. Use a separate ESP32 soil node so the camera pin map and analog readings do not conflict.

## Wiring

```text
Sensor GND  -> ESP32 GND
Sensor VCC  -> ESP32 3V3
Sensor AOUT -> ESP32 GPIO32
```

Use 3.3 V power for the sensor so its analog output stays in the ESP32-safe range.
GPIO32 is an ADC1 pin, which is the right family to use while Wi-Fi is active.

## Firmware

Upload:

```text
esp32/esp32_soil_node/esp32_soil_node.ino
```

The node reads GPIO32 by default, serves `/status` over HTTP, and publishes MQTT readings every 30 seconds. If the configured pin is effectively dead, the firmware scans GPIO32, GPIO33, GPIO34, GPIO35, GPIO36, and GPIO39, then switches to the strongest detected ADC signal.

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
MQTT_HOST=172.20.10.7
MQTT_PORT=1883
MQTT_USER=iota
MQTT_PASSWORD=REDACTED
SOIL_PIN=32
DRY_RAW=3200
WET_RAW=1400
```

`MQTT_HOST=172.20.10.7` is the Raspberry Pi / Home Assistant / Mosquitto broker on REDACTED hotspot. This removes the laptop forwarder from the soil sensor path.

The ready-to-copy config for this setup is:

```text
config/germsensor_iphone_hotspot.cfg
```

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

For the current wiring, `soil_pin` should report `32` in `/status`, Serial
Monitor, and MQTT. If it reports a different pin, check that `SOIL_PIN=32` is in
`germsensor.cfg` and that the sensor AOUT wire is actually on GPIO32.

Use the helper to collect calibration readings from Serial Monitor:

```bash
python3 pi/calibrate_soil_sensor.py --label dry --expected-pin 32 --seconds 60
python3 pi/calibrate_soil_sensor.py --label wet --expected-pin 32 --seconds 60
```

Then choose representative dry and wet raw values from:

```text
data/calibration/soil_calibration.csv
```

Update `DRY_RAW` and `WET_RAW` in `germsensor.cfg`. A capacitive sensor usually reports a higher value in dry air/substrate and a lower value in wet substrate, so the defaults start as:

```text
DRY_RAW=3200
WET_RAW=1400
```

## Test

Serial Monitor at 115200 should print lines like:

```text
Loaded /germsensor.cfg from microSD
germsoil-1 ready: http://<ip>/
soil raw=2430 moisture=42%
```

From the laptop or Pi:

```bash
mosquitto_sub -h 172.20.10.7 -u iota -P 'REDACTED' -t 'germination/germsoil-1/#' -v
```

Or open:

```text
http://<soil-node-ip>/status
```
