# Camera functionality next steps

## Camera functionality

Current intended operating path:

```text
ESP32-CAM
  -> REDACTED hotspot
  -> Raspberry Pi / Home Assistant / Mosquitto
  -> Home Assistant dashboard, storage, and later image analysis
```

Current known addresses:

```text
ESP32-CAM: 172.20.10.5
Raspberry Pi / Home Assistant / MQTT: 172.20.10.7
Home Assistant UI: http://172.20.10.7:8123
ESP32 snapshot URL: http://172.20.10.5/jpg
ESP32 status URL: http://172.20.10.5/status
```

The laptop should not be part of the normal runtime path. It is only needed for
debugging, reflashing, editing the ESP32 microSD config, or checking logs.

### Picture process flow

1. ESP32-CAM boots.
2. ESP32-CAM reads `/germcam.cfg` from the microSD card.
3. ESP32-CAM joins `REDACTED`.
4. ESP32-CAM receives a hotspot IP address, currently `172.20.10.5`.
5. ESP32-CAM starts HTTP endpoints:
   - `http://172.20.10.5/status`
   - `http://172.20.10.5/jpg`
   - `http://172.20.10.5/stream`
6. ESP32-CAM publishes MQTT status directly to the Pi broker at `172.20.10.7:1883`.
7. Home Assistant receives MQTT topics:
   - `germination/germcam-1/availability`
   - `germination/germcam-1/status`
   - `germination/germcam-1/jpg_url`
   - `germination/germcam-1/stream_url`
8. Home Assistant or a Pi-side capture script pulls the image from:

   ```text
   http://172.20.10.5/jpg
   ```

9. The Pi stores the image locally and Home Assistant displays the latest image.
10. Later processing can run germination detection against the saved image folder.

### Immediate checks

From any phone or laptop connected to `REDACTED`, open:

```text
http://172.20.10.5/status
http://172.20.10.5/jpg
http://172.20.10.7:8123
```

In Home Assistant, confirm:

```text
Germcam 1 Online = on
Germcam 1 JPEG URL = http://172.20.10.5/jpg
```

If those work, the camera is talking directly to the Raspberry Pi and the laptop
is no longer required.

### Home Assistant dashboard step

Add a camera card in Home Assistant using:

```text
Still Image URL: http://172.20.10.5/jpg
Stream Source URL: http://172.20.10.5/stream
```

If the ESP32 IP changes after reboot, update the card URL or use the MQTT
published URL from `germination/germcam-1/jpg_url`.

### Pi-side capture step

Run captures from the Raspberry Pi, not the laptop:

```bash
python3 capture_esp32_cam_images.py \
  --camera germcam-1=http://172.20.10.5/jpg \
  --output-dir images \
  --log logs/capture_events.jsonl
```

Expected output path:

```text
images/YYYY-MM-DD/germcam-1/germcam-1_YYYYMMDD_HHMMSS.jpg
```

Once a manual capture works, schedule it every 5-10 minutes with a Home
Assistant automation, cron, or a systemd timer depending on which Pi OS path is
used.

### Recommended next implementation steps

1. Enable Terminal/SSH access on the Raspberry Pi or Home Assistant so capture
   commands can be run directly on the Pi.
2. Copy `pi/capture_esp32_cam_images.py` to the Pi if it is not already there.
3. Run one manual capture from the Pi using `http://172.20.10.5/jpg`.
4. Confirm the image file appears under the Pi image directory.
5. Add a Home Assistant dashboard camera card for the latest snapshot or stream.
6. Add scheduled capture every 5-10 minutes.
7. Add germination analysis against the saved image folder.
8. Move from iPhone hotspot to a dedicated small router before final demo if
   longer-term stability is needed.

### More stable network option

The iPhone hotspot is fine for a field demo, but a small router is more reliable
for repeat testing.

Recommended router layout:

```text
Router Wi-Fi
  - Raspberry Pi / Home Assistant / MQTT
  - ESP32-CAM
  - Control laptop or phone
```

Reserve fixed DHCP addresses:

```text
Raspberry Pi: 192.168.50.10
ESP32-CAM:   192.168.50.20
```

Then update the ESP32-CAM SD config:

```text
NODE_NAME=germcam-1
WIFI_SSID=REDACTED
WIFI_PASSWORD=REDACTED
MQTT_HOST=192.168.50.10
MQTT_PORT=1883
MQTT_USER=iota
MQTT_PASSWORD=REDACTED
```
