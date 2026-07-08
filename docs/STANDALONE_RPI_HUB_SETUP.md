# Standalone Raspberry Pi hub setup

This setup removes the laptop and iPhone hotspot from the normal system path.

## Target architecture

```text
ESP32-CAM nodes
ESP32 soil node
        |
        | Wi-Fi: REDACTED
        v
Raspberry Pi
  - Wi-Fi access point: 192.168.50.1
  - Mosquitto MQTT broker: 192.168.50.1:1883
  - Home Assistant dashboard
  - Scheduled image capture
```

The ESP32 devices should publish directly to the Pi:

```text
MQTT_HOST=192.168.50.1
MQTT_PORT=1883
```

## Recommended OS choice

Use Raspberry Pi OS Lite/Desktop if the Pi must be the standalone hub and Wi-Fi access point.

Home Assistant OS is good when the Pi is connected to an existing router, but it is not the best fit when the same Pi must also act as the project Wi-Fi access point and run custom Python capture scripts.

## Pi setup

Copy the project `pi` folder onto the Raspberry Pi, then run:

```bash
cd /path/to/project/pi
sudo AP_SSID=REDACTED \
  AP_PASSWORD=REDACTED \
  MQTT_USER=iota \
  MQTT_PASSWORD=REDACTED \
  bash standalone_rpi_setup.sh
```

The script configures:

- Wi-Fi AP named `REDACTED`
- Pi AP address `192.168.50.1`
- Mosquitto listener on `0.0.0.0:1883`
- MQTT username/password
- `germcam-capture.timer` for scheduled image capture

Check services:

```bash
systemctl status NetworkManager
systemctl status mosquitto
systemctl status germcam-capture.timer
```

Listen for MQTT:

```bash
mosquitto_sub -h 192.168.50.1 -u iota -P 'REDACTED' -t 'germination/#' -v
```

## ESP32-CAM SD config

Each ESP32-CAM microSD card should use the Pi AP and broker:

```text
WIFI_SSID=REDACTED
WIFI_PASSWORD=REDACTED
MQTT_HOST=192.168.50.1
MQTT_PORT=1883
MQTT_USER=iota
MQTT_PASSWORD=REDACTED
```

Keep each node name unique:

```text
NODE_NAME=germcam-1
```

```text
NODE_NAME=germcam-2
```

## ESP32 soil node SD config

Use:

```text
NODE_NAME=germsoil-1
WIFI_SSID=REDACTED
WIFI_PASSWORD=REDACTED
MQTT_HOST=192.168.50.1
MQTT_PORT=1883
MQTT_USER=iota
MQTT_PASSWORD=REDACTED
SOIL_PIN=32
DRY_RAW=3200
WET_RAW=1400
```

Adjust `DRY_RAW` and `WET_RAW` after calibration.

## Home Assistant

Install Home Assistant on the Pi after the AP/MQTT setup. Two workable options:

1. Home Assistant Container on Raspberry Pi OS.
2. Home Assistant Core in a Python virtual environment.

Once Home Assistant is running, enable packages by adding this to `configuration.yaml`:

```yaml
homeassistant:
  packages: !include_dir_named packages
```

Copy:

```text
project/homeassistant/packages/germcam_mqtt.yaml
```

to:

```text
/config/packages/germcam_mqtt.yaml
```

Restart Home Assistant.

Add MQTT integration:

```text
Settings > Devices & services > Add integration > MQTT
Broker: 192.168.50.1
Port: 1883
Username: iota
Password: REDACTED
```

## Definition of done

The system is standalone when all of these work with the laptop turned off:

- ESP32 devices connect to `REDACTED`.
- Pi receives MQTT on `germination/#`.
- Home Assistant shows soil moisture and camera health entities.
- Image capture files appear under the Pi project directory:

  ```text
  /opt/germination-chamber/images
  ```

- The dashboard still works after rebooting the Pi.
