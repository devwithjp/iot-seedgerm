# Soil sensor calibration plan

## Config

Copy this file to the ESP32 microSD card as `/germsensor.cfg`:

```text
project/config/germsensor.cfg
```

The important setting for the current wiring is:

```text
SOIL_PIN=32
```

After reboot, serial should report `Soil sensor pin: GPIO32` and recurring readings should start with `soil GPIO32`.

## Collection

Run each condition for 60 seconds and keep the sensor still while collecting.

Dry reference:

```bash
python3 project/pi/calibrate_soil_sensor.py \
  --label dry \
  --expected-pin 32 \
  --seconds 60
```

Wet reference:

```bash
python3 project/pi/calibrate_soil_sensor.py \
  --label wet \
  --expected-pin 32 \
  --seconds 60
```

Optional in-tray check after setting dry/wet values:

```bash
python3 project/pi/calibrate_soil_sensor.py \
  --label tray \
  --expected-pin 32 \
  --seconds 60
```

Rows are appended to:

```text
project/data/calibration/soil_calibration.csv
```

## Procedure

1. Confirm the sensor is powered from ESP32 `3V3` and shares ESP32 `GND`.
2. Confirm sensor `AOUT` is connected to GPIO32.
3. Reboot the ESP32 after copying the updated `germsensor.cfg`.
4. Collect the dry reference with the probe in air or dry substrate.
5. Collect the wet reference with the probe in saturated substrate, not direct water unless that is the intended maximum.
6. Use the dry run mean as `DRY_RAW`.
7. Use the wet run mean as `WET_RAW`.
8. Edit `project/config/germsensor.cfg`, copy it back to the microSD card, and reboot.
9. Run the tray check and confirm the reported moisture percent is plausible.

For this capacitive analog sensor, dry is usually a higher raw ADC value and wet is usually lower. If your collected data shows the opposite, keep the measured values anyway; the firmware mapping supports either direction as long as `DRY_RAW` and `WET_RAW` are set from the actual readings.
