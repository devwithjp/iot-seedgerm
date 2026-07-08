# ESP32-CAM first setup

This project is a mini germination chamber. The pitch puts the Raspberry Pi at the centre for MQTT, Home Assistant, storage, and image analysis. With the current hardware, set up the two ESP32-CAM boards first as fixed camera nodes. Each ESP32-CAM publishes node status over MQTT and exposes images over HTTP for the Pi/Home Assistant to pull.

## Recommended first architecture

- `germcam-1`: ESP32-CAM looking at tray/zone 1.
- `germcam-2`: ESP32-CAM looking at tray/zone 2 or a second angle.
- Raspberry Pi: runs Mosquitto, receives ESP32-CAM status topics, polls each camera's `/jpg` endpoint, stores timestamped images, publishes analysis results to MQTT/Home Assistant, and runs germination detection.

Keep the soil moisture sensors and actuators separate for the first camera bring-up. ESP32-CAM boards have awkward spare ADC pins because the camera consumes most GPIO and Wi-Fi conflicts with ADC2. If your soil sensors are analog capacitive probes, the cleaner path is either a Pi ADC board such as ADS1115, or a non-camera ESP32 used only for sensors/actuators.

Do not send every camera image through MQTT at first. It can work for small snapshots, but large binary payloads make Mosquitto/Home Assistant less reliable. Publish health/status and image URLs over MQTT, then fetch images over HTTP.

## Hardware needed for flashing

- Freenove ESP32-WROVER CAM module. The current firmware uses the WROVER_KIT/Freenove pin map.
- FTDI/USB-to-serial adapter set to `5V`.
- Jumper wires.
- Stable 5V supply. Weak USB power causes camera brownouts.

## Wiring for flashing

| FTDI | ESP32-CAM |
| --- | --- |
| 5V | 5V |
| GND | GND |
| TX | U0R |
| RX | U0T |
| GND | GPIO0 |

`GPIO0` must be connected to `GND` only while flashing. Remove that link and press reset to boot normally after upload.

## Arduino IDE setup

1. Install Arduino IDE.
2. Add ESP32 boards:
   - Open `File > Preferences`.
   - Add this Boards Manager URL:
     `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   - Open `Tools > Board > Boards Manager`.
   - Install `esp32` by Espressif Systems.
3. Install the MQTT library:
   - Open `Sketch > Include Library > Manage Libraries`.
   - Install `PubSubClient` by Nick O'Leary.
4. Open [esp32_cam_node.ino](../esp32/esp32_cam_node/esp32_cam_node.ino).
5. Edit:
   - `WIFI_SSID`
   - `WIFI_PASSWORD`
   - `NODE_NAME`
   - `MQTT_HOST`
   - `MQTT_USER` and `MQTT_PASSWORD`, if your Mosquitto broker requires them
6. For board 1 use `NODE_NAME = "germcam-1"`.
7. For board 2 use `NODE_NAME = "germcam-2"`.

You can also put these settings on the ESP32-CAM microSD card as `germcam.cfg`, then the same flashed firmware can be reused. See [ESP32_CAM_SD_CONFIG.md](ESP32_CAM_SD_CONFIG.md).

## Arduino IDE board settings

- Board: `ESP32 Wrover Module`
- Upload Speed: `115200`
- CPU Frequency: `240MHz`
- Flash Frequency: `40MHz`
- Partition Scheme: `Huge APP`
- PSRAM: `Enabled`

## Flash and test

1. Connect `GPIO0` to `GND`.
2. Press `RST`.
3. Click Upload in Arduino IDE.
4. When upload finishes, remove the `GPIO0` to `GND` wire.
5. Insert the prepared ESP32-CAM microSD card if you are using `germcam.cfg`.
6. Press `RST`.
7. Open Serial Monitor at `115200`.
8. Note the printed URL, for example `http://192.168.1.42/`.
9. Test these endpoints in a browser:
   - `/` live preview page
   - `/jpg` single image
   - `/stream` MJPEG stream
   - `/status` JSON health check

## Current board status notes

The firmware has been compiled and uploaded successfully to the connected board on `/dev/ttyUSB0`.

Serial Monitor confirmed that the microSD config is read:

```text
Loaded /germcam.cfg from microSD
Connecting to REDACTED
```

If connection to `REDACTED` keeps printing dots and never reaches `germcam-1 ready: http://...`, enable `Maximize Compatibility` on the iPhone Personal Hotspot. ESP32 boards need 2.4 GHz Wi-Fi.

The connected board has also been identified as:

```text
ESP32-D0WD-V3 revision v3.1
Dual core, 240 MHz
40 MHz crystal
4 MB flash
MAC f4:2d:c9:78:28:30
```

With the Freenove/WROVER pin map, `/status` reports `camera_ready:true` and `/jpg` returns a real JPEG.

Current iPhone hotspot test addresses:

```text
germcam-1: http://172.20.10.5/
Laptop on hotspot: 172.20.10.4
Raspberry Pi / Home Assistant / Mosquitto: 10.42.0.75
```

Because the ESP32 is on the iPhone hotspot and the Pi is currently on the wired `10.42.0.0/24` subnet, MQTT can be forwarded through the laptop. See [IPHONE_HOTSPOT_MQTT_FORWARDER.md](IPHONE_HOTSPOT_MQTT_FORWARDER.md).

## MQTT topics

Each ESP32-CAM publishes under:

```text
germination/<node-name>/availability
germination/<node-name>/status
germination/<node-name>/jpg_url
germination/<node-name>/stream_url
```

For board 1:

```text
germination/germcam-1/availability
germination/germcam-1/status
germination/germcam-1/jpg_url
germination/germcam-1/stream_url
```

For board 2:

```text
germination/germcam-2/availability
germination/germcam-2/status
germination/germcam-2/jpg_url
germination/germcam-2/stream_url
```

Test from the Raspberry Pi:

```bash
mosquitto_sub -h localhost -t 'germination/#' -v
```

## Chamber mounting notes

- Mount each ESP32-CAM outside the wettest airflow path.
- Keep a clear acrylic window or lens shield between the tray and electronics if condensation appears.
- Fix the camera angle and distance before collecting training images.
- Use consistent lighting when capturing images for germination detection.
- Avoid using the onboard flash LED as the main grow light; it is harsh, heats locally, and changes image appearance.

## Next step after both cameras work

On the Raspberry Pi, use the capture script:

```bash
python3 pi/capture_esp32_cam_images.py
```

It stores timestamped JPEGs and logs each capture attempt. See [RPI_IMAGE_CAPTURE.md](RPI_IMAGE_CAPTURE.md).

If `.local` hostnames do not resolve, use the IP addresses printed in Serial Monitor or reserve fixed IP addresses in your router.
