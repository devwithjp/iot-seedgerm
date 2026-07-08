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
  - Ethernet: optional LAN/SSH/internet uplink
  - Wi-Fi access point: 192.168.50.1
  - Mosquitto MQTT broker: 192.168.50.1:1883
  - Home Assistant dashboard
  - Scheduled image capture
```

If an ESP32-CAM is plugged into the Pi by USB Type-C, use that cable for 5V
power, flashing, and Serial Monitor logs. With the current firmware, images and
MQTT do not travel over USB; the ESP32-CAM still joins the Pi Wi-Fi AP and the
Pi pulls images from `http://germcam-1.local/jpg` or the camera IP address.

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
- Ethernet remains usable for SSH, LAN access, or internet access
- Pi AP address `192.168.50.1`
- Mosquitto listener on `0.0.0.0:1883`
- MQTT username/password
- Avahi/mDNS support for `germcam-1.local` and `germcam-2.local`
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

## ESP32-CAM over USB Type-C

Connect the ESP32-CAM to a Pi USB port using a data-capable USB-C cable. On the
Pi, check that the serial device appears:

```bash
ls -l /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
dmesg | tail -40
```

For flashing from the Pi, install Arduino CLI or use Arduino IDE from another
computer. Typical board settings are listed in
[ESP32_CAM_SETUP.md](ESP32_CAM_SETUP.md). For serial logs from the Pi:

```bash
sudo apt-get install -y minicom
minicom -D /dev/ttyUSB0 -b 115200
```

If the board appears as `/dev/ttyACM0`, use that path instead. After boot,
Serial Monitor should print:

```text
Loaded /germcam.cfg from microSD
Connecting to REDACTED
germcam-1 ready: http://192.168.50.x/
mDNS ready: http://germcam-1.local/
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
