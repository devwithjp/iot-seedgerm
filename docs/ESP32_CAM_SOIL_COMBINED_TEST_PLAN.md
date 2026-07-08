# ESP32-CAM combined camera and soil test

## Camera and soil on one ESP32-CAM

This test checks whether the Freenove ESP32-WROVER CAM can run the camera and a
capacitive soil moisture sensor v2.0 on GPIO32 at the same time.

## Wiring

```text
Soil sensor GND  -> ESP32-CAM GND
Soil sensor VCC  -> ESP32-CAM 3V3
Soil sensor AOUT -> ESP32-CAM GPIO32
```

Use `AOUT`, not `DOUT`. Keep wires short for the first test.

## SD config

Use `config/germcam_1_iphone_hotspot.cfg` as `/germcam.cfg` on the ESP32-CAM
microSD card.

The combined-test fields are:

```text
SOIL_ENABLED=1
SOIL_PIN=32
DRY_RAW=3200
WET_RAW=1400
```

## Boot checks

Serial Monitor at `115200` should show:

```text
Loaded /germcam.cfg from microSD
Soil sensor enabled on GPIO32
Connecting to REDACTED
germcam-1 ready: http://172.20.10.x/
Connecting to MQTT 172.20.10.7:1883
soil GPIO32 raw=<value> moisture=<percent>%
```

## HTTP checks

From a device on the same hotspot:

```text
http://172.20.10.5/status
http://172.20.10.5/jpg
http://172.20.10.5/stream
```

`/status` should include:

```json
{
  "camera_ready": true,
  "soil_enabled": true,
  "soil_pin": 32,
  "soil_raw": 2430,
  "soil_moisture_percent": 42
}
```

## MQTT checks

```bash
mosquitto_sub -h 172.20.10.7 -u iota -P 'REDACTED' -t 'germination/germcam-1/#' -v
```

Expected topics:

```text
germination/germcam-1/availability
germination/germcam-1/status
germination/germcam-1/jpg_url
germination/germcam-1/stream_url
germination/germcam-1/soil_raw
germination/germcam-1/soil_moisture_percent
```

## Pass criteria

- `/status` returns both camera and soil fields.
- `/jpg` returns a valid JPEG.
- MQTT publishes `availability=online`.
- MQTT publishes `soil_raw` and `soil_moisture_percent`.
- Soil readings change between dry and wet conditions.
- The board does not brown out or reboot during repeated image captures.

## Stress test

Run for 15-30 minutes:

1. Pull `/jpg` every 30 seconds.
2. Open `/stream` for 2-5 minutes.
3. Watch Serial Monitor for brownout, camera capture failures, or resets.
4. Watch MQTT for continuous soil readings.

Use the one-chip design if the stress test passes. Fall back to a separate soil
node if GPIO32 reads are stuck, noisy, or camera capture becomes unreliable.
