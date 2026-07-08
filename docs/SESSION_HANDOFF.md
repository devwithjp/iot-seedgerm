# Session handoff - 2026-07-07

## Current status

Update on 2026-07-08: the ESP32-CAM combined camera + soil test is now running on
one board with the capacitive soil sensor on GPIO32.

The combined firmware was flashed successfully to:

```text
esp32/esp32_cam_node/esp32_cam_node.ino
```

The ESP32-CAM now boots with the inserted microSD card and loads:

```text
/germcam.cfg
```

Current combined config fields on the card:

```text
SOIL_ENABLED=1
SOIL_PIN=32
DRY_RAW=3200
WET_RAW=1400
```

Confirmed serial boot after flashing:

```text
Loaded /germcam.cfg from microSD
Soil sensor enabled on GPIO32
Connecting to Khushali’s iPhone.
germcam-1 ready: http://172.20.10.5/
mDNS ready: http://germcam-1.local/
Connecting to MQTT 172.20.10.7:1883
```

Confirmed direct HTTP `/status` from a client on the same hotspot:

```json
{"node":"germcam-1","ip":"172.20.10.5","rssi":-48,"uptime_ms":2527910,"free_heap":179972,"camera_ready":true,"config_source":"sd","jpg_url":"http://172.20.10.5/jpg","stream_url":"http://172.20.10.5/stream","soil_enabled":true,"soil_pin":32,"soil_raw":4095,"soil_moisture_percent":0,"dry_raw":3200,"wet_raw":1400}
```

Confirmed MQTT topics from the combined node when the laptop was on the same
hotspot:

```text
germination/germcam-1/availability online
germination/germcam-1/status
germination/germcam-1/jpg_url
germination/germcam-1/stream_url
germination/germcam-1/soil_raw
germination/germcam-1/soil_moisture_percent
```

Confirmed camera still works with the combined firmware:

```text
GET http://172.20.10.5/jpg -> valid JPEG
```

The current remaining hardware issue is not coexistence, but analog saturation.
The latest stable readings reached full scale:

```text
soil GPIO32 raw=4095 moisture=0%
```

This means camera + GPIO32 soil can coexist on one ESP32-CAM board, but the soil
signal still needs calibration and possibly wiring cleanup if it remains pegged at
4095.

Home Assistant package updates were made so the combined node can expose these new
entities under `Germcam 1`:

```text
Germcam 1 Soil Raw Moisture
Germcam 1 Soil Moisture
```

Package file:

```text
homeassistant/packages/germcam_mqtt.yaml
```

An on-demand Pi-side helper was added:

```text
pi/poll_esp32_status_to_mqtt.py
```

It fetches:

```text
http://172.20.10.5/status
```

and republishes:

```text
germination/germcam-1/status
germination/germcam-1/soil_raw
germination/germcam-1/soil_moisture_percent
```

Current blocker on that helper path:

```text
mosquitto_pub -> Connection Refused: not authorised
MQTT connect failed, rc=5
```

So if the helper is used, the Pi broker credentials or Mosquitto permissions need
to be rechecked first.

Update on 2026-07-08: a local config file has been added at `project/config/germsensor.cfg` with `SOIL_PIN=32`. The ESP32 microSD card was not mounted on this Linux machine, so copy that file to the card as `/germsensor.cfg` before rebooting the node.

Latest GPIO32 test after moving the sensor back:

```text
soil GPIO32 raw=1137 moisture=100%
adc scan: GPIO32=1134 GPIO33=0 GPIO34=0 GPIO35=0 GPIO36=0 GPIO39=0
```

A short structured test then fell back toward zero:

```text
pre_calibration_gpio32_test: count=10 min=0 max=14 mean=1.6 median=0.0 stdev=4.4
```

This confirms GPIO32 can see the sensor output, but the reading is not stable enough to calibrate yet.

Latest soil sensor continuation:

```text
soil GPIO33 raw=0 moisture=100%
adc scan: GPIO32=0 GPIO33=0 GPIO34=0 GPIO35=0 GPIO36=0 GPIO39=0
```

The firmware auto-selected GPIO33 during the latest read because the configured signal was dead, but the settled scan still showed all checked ADC pins at 0. Calibration CSV summary:

```text
soil_calibration.csv dry_stability: count=69 min=0 max=415 mean=19.1 median=0.0 nonzero=6
soil_calibration.csv wet_test:      count=30 min=0 max=0   mean=0.0  median=0.0 nonzero=0
```

Do not set final `DRY_RAW` / `WET_RAW` yet. The sensor output or jumper contact is still not stable enough for calibration.

Standalone Pi progress has been prepared locally, but not copied to the Pi because `homeassistant.local` / `raspberrypi.local` did not resolve from the laptop and Ethernet showed `NO-CARRIER`.

Calibration tooling has been added:

```text
project/pi/calibrate_soil_sensor.py
project/docs/SOIL_SENSOR_CALIBRATION_PLAN.md
```

Update on 2026-07-08: the firmware was flashed with settled ADC averaging and active-pin logging. After that change, the trustworthy scan showed every checked ADC pin at 0:

```text
soil GPIO32 raw=0 moisture=100%
adc scan: GPIO32=0 GPIO33=0 GPIO34=0 GPIO35=0 GPIO36=0 GPIO39=0
```

The firmware now logs the active GPIO and uses settled ADC averages for the scan. It also auto-selects the strongest scanned ADC pin when the configured pin is effectively dead, but no valid analog signal is currently present on the scanned pins.

The separate ESP32 soil moisture node has been flashed and is running:

```text
esp32/esp32_soil_node/esp32_soil_node.ino
```

The node successfully loads config from the inserted microSD card:

```text
Loaded /germsensor.cfg from SD_MMC
```

Earlier, the node connected to the iPhone hotspot:

```text
germsoil-1 ready: http://172.20.10.6/
```

Earlier, MQTT worked through the laptop forwarder:

```text
172.20.10.4:1883 -> 127.0.0.1:1883
```

Confirmed MQTT topics from the earlier connected run:

```text
germination/germsoil-1/availability online
germination/germsoil-1/status
germination/germsoil-1/soil_raw
germination/germsoil-1/soil_moisture_percent
```

Example received status from the earlier connected run:

```json
{"node":"germsoil-1","ip":"172.20.10.6","rssi":-37,"config_source":"sd","soil_pin":34,"soil_raw":0,"soil_moisture_percent":100}
```

## Important commands

Read ESP32 serial:

```bash
python3 -c "import serial,time
s=serial.Serial('/dev/ttyUSB0',115200,timeout=0.5)
end=time.time()+30
while time.time()<end:
    b=s.readline()
    if b:
        print(b.decode('utf-8','replace').rstrip())
s.close()"
```

Start MQTT forwarder to local Mosquitto:

```bash
setsid python3 pi/mqtt_tcp_forwarder.py \
  --listen-host 172.20.10.4 \
  --target-host 127.0.0.1 \
  >/tmp/germcam_mqtt_forwarder.log 2>&1 < /dev/null &
```

Check soil MQTT:

```bash
mosquitto_sub -h 127.0.0.1 -p 1883 -u iota -P REDACTED \
  -t 'germination/germsoil-1/#' -v
```

Compile/upload soil firmware:

```bash
/tmp/arduino-cli/arduino-cli compile --fqbn esp32:esp32:esp32 esp32/esp32_soil_node/esp32_soil_node.ino
/tmp/arduino-cli/arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 esp32/esp32_soil_node/esp32_soil_node.ino
```

## Remaining issue

The soil sensor analog value was wrong before the auto-select firmware change:

```text
soil raw=0 moisture=100%
adc scan: GPIO32=600 GPIO33=370 GPIO34=0 GPIO35=0 GPIO36=0 GPIO39=0
```

The latest GPIO32 test means the firmware is running and GPIO32 can receive the analog signal, but the signal is unstable and drops to 0.

Expected sensor wiring:

```text
Sensor GND  -> ESP32 GND
Sensor VCC  -> ESP32 3V3
Sensor AOUT -> ESP32 GPIO32
```

Next hardware step:

1. Check that the sensor is powered from 3.3 V.
2. Check that `AOUT`, not `DOUT`, is wired to GPIO32.
3. Measure sensor `AOUT` against `GND` with a multimeter if available. It should not be 0 V when the sensor is powered.
4. Re-read serial. A valid analog signal should appear as a nonzero value in the settled `adc scan`.

## Notes

The old forwarder target `10.42.0.75:1883` was refusing connections during this session. Local Mosquitto on `127.0.0.1:1883` worked, so the active forwarder was changed to target localhost.

The ESP32 firmware currently has the hotspot password as a fallback and ignores `WIFI_PASSWORD=REDACTED` from the SD card.
