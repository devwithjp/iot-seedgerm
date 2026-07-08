# Home Assistant OS install for Raspberry Pi

This guide is for the Mini Germination Chamber Raspberry Pi hub.

The official Home Assistant Raspberry Pi guide recommends Home Assistant OS for Raspberry Pi 4 or 5. It includes Home Assistant Core, Supervisor, add-ons, and updates.

Official guide checked on 7 July 2026: <https://www.home-assistant.io/installation/raspberrypi/>

## Downloaded image

The official Home Assistant Raspberry Pi guide currently links Home Assistant OS `18.1` for Raspberry Pi 4 and Raspberry Pi 5. I downloaded the official Raspberry Pi 4 64-bit image here:

```text
downloads/haos_rpi4-64-18.1.img.xz
```

File size:

```text
321M
```

SHA256:

```text
e86baac3b6d714833d747ca96df32103da116561c5467871d8e88da5af4279fd
```

Use this file if your board is a Raspberry Pi 4.

If your board is a Raspberry Pi 5, do not use the Pi 4 image. Download the Pi 5 image from the official Home Assistant page instead.

## What you need

- Raspberry Pi 4 or Raspberry Pi 5.
- Official or reliable power supply.
- microSD card, at least 32 GB.
- SD card reader.
- Ethernet cable.
- Laptop/desktop computer.

Ethernet is strongly recommended for first boot. Wi-Fi can be configured later.

## Option A: Recommended method using Raspberry Pi Imager

This is the easiest method.

1. Install Raspberry Pi Imager:

   <https://www.raspberrypi.com/software/>

2. Insert the microSD card into your computer.

3. Open Raspberry Pi Imager.

4. Click `Choose Device`.

5. Select your Raspberry Pi model.

6. Click `Choose OS`.

7. Select:

   ```text
   Other specific-purpose OS > Home automation > Home Assistant
   ```

8. Choose the Home Assistant OS version for your hardware:

   - Raspberry Pi 4: choose the Raspberry Pi 4 image.
   - Raspberry Pi 5: choose the Raspberry Pi 5 image.

9. Click `Choose Storage`.

10. Select your microSD card.

11. Click `Next`, then `Write`.

12. Wait until writing and verification finish.

13. Eject the microSD card.

## Option B: Use the downloaded image file

Use this if Raspberry Pi Imager cannot download Home Assistant itself.

1. Open Raspberry Pi Imager.

2. Click `Choose OS`.

3. Scroll down and select:

   ```text
   Use custom
   ```

4. Select this file:

   ```text
   /home/jp/Desktop/IOT/project/downloads/haos_rpi4-64-18.1.img.xz
   ```

5. Click `Choose Storage`.

6. Select your microSD card.

7. Click `Next`, then `Write`.

8. Wait until writing and verification finish.

9. Eject the microSD card.

## First boot

1. Insert the microSD card into the Raspberry Pi.

2. Plug Ethernet into the Pi.

3. Connect the Pi power supply.

4. Wait a few minutes. On a Raspberry Pi 4 or 5, Home Assistant should usually become reachable within about a minute; wait up to 5 minutes before troubleshooting.

5. From your laptop browser, open:

   ```text
   http://homeassistant.local:8123
   ```

If that does not work, try:

```text
http://homeassistant:8123
```

Or find the Pi IP address from your router and open:

```text
http://<PI_IP_ADDRESS>:8123
```

Example:

```text
http://192.168.1.10:8123
```

## First Home Assistant setup

1. Create the owner account.

2. Set location/time zone.

3. Skip integrations for now unless it automatically finds useful devices.

4. Once inside Home Assistant, install Mosquitto:

   ```text
   Settings > Add-ons > Add-on Store > Mosquitto broker > Install
   ```

5. Start Mosquitto.

6. Enable:

   - `Start on boot`
   - `Watchdog`

## MQTT plan for the germination chamber

The ESP32 nodes should publish to topics like:

```text
germination/germcam-1/status
germination/germcam-1/availability
germination/germcam-1/jpg_url
germination/germcam-1/stream_url
```

So the Pi/Home Assistant can show:

- camera online/offline status
- signal strength
- image URL
- stream URL
- later: soil moisture, light, temperature, fan/heater status

For the dashboard configuration, see [HOME_ASSISTANT_MQTT_DASHBOARD.md](HOME_ASSISTANT_MQTT_DASHBOARD.md).

## Troubleshooting

If `homeassistant.local:8123` does not load after 5 minutes:

1. Make sure Ethernet is plugged in.

2. Check your router for a new device named `homeassistant`.

3. Try the IP address directly:

   ```text
   http://<PI_IP_ADDRESS>:8123
   ```

4. If still not working, re-flash the SD card.

5. If you have a monitor and HDMI cable, connect the Pi to see boot messages.

## Warning

Writing the image to the SD card erases everything on that card.
