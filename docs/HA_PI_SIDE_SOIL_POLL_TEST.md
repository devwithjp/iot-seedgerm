# HA Pi-side soil poll test

Use this when you want Home Assistant or the Raspberry Pi to explicitly poll the
ESP32 for a fresh soil reading instead of waiting for the normal MQTT publish
interval.

## Script

```text
pi/poll_esp32_status_to_mqtt.py
```

It does this:

1. GET `http://172.20.10.5/status`
2. Parse `soil_raw` and `soil_moisture_percent`
3. Publish them to the existing MQTT topics:

```text
germination/germcam-1/status
germination/germcam-1/soil_raw
germination/germcam-1/soil_moisture_percent
```

## Manual test

From the Pi or Home Assistant terminal:

```bash
python3 pi/poll_esp32_status_to_mqtt.py
```

Expected output:

```text
published soil_raw=<value> soil_moisture_percent=<value> from http://172.20.10.5/status to germination/germcam-1
```

## Useful options

Use retained MQTT messages:

```bash
python3 pi/poll_esp32_status_to_mqtt.py --retain
```

Point to a different ESP32:

```bash
python3 pi/poll_esp32_status_to_mqtt.py --status-url http://172.20.10.6/status
```

Publish under a different topic prefix:

```bash
python3 pi/poll_esp32_status_to_mqtt.py --topic-prefix germination/germcam-test
```

## Notes

- This script runs on the Pi side, not on the ESP32.
- It is only a test/helper path. The normal system design is still:

  ```text
  ESP32 -> MQTT broker -> Home Assistant
  ```

- Use this when you want an explicit on-demand fetch path from the Pi or Home
  Assistant side.
