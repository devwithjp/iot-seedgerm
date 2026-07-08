# iPhone hotspot MQTT forwarder

Use this when the ESP32-CAM is on REDACTED hotspot but the Raspberry Pi/Home Assistant broker is still reachable from this laptop on the wired `10.42.0.0/24` subnet.

Current detected addresses:

```text
ESP32-CAM: 172.20.10.5
Laptop on iPhone hotspot: 172.20.10.4
iPhone gateway: 172.20.10.1
Raspberry Pi / Home Assistant / Mosquitto: 10.42.0.75
```

The ESP32 publishes MQTT to `172.20.10.4:1883`. This laptop forwards that TCP connection to the Pi broker at `10.42.0.75:1883`.

## Start the forwarder

```bash
python3 pi/mqtt_tcp_forwarder.py \
  --listen-host 172.20.10.4 \
  --target-host 10.42.0.75
```

Leave that process running while testing MQTT from the ESP32.

## Start it in the background

```bash
setsid python3 pi/mqtt_tcp_forwarder.py \
  --listen-host 172.20.10.4 \
  --target-host 10.42.0.75 \
  >/tmp/germcam_mqtt_forwarder.log 2>&1 < /dev/null &
```

Check it:

```bash
ss -ltnp | grep ':1883'
tail -f /tmp/germcam_mqtt_forwarder.log
```

Stop it:

```bash
pkill -f 'pi/mqtt_tcp_forwarder.py'
```

## MQTT authentication

The forwarder only moves TCP traffic. It does not bypass Mosquitto authentication.

If the ESP32 serial log shows:

```text
MQTT connect failed, rc=5
```

then the network path is working, but Mosquitto rejected the username/password. Put the broker credentials in `/germcam.cfg`:

```text
MQTT_HOST=172.20.10.4
MQTT_PORT=1883
MQTT_USER=<mqtt-user>
MQTT_PASSWORD=REDACTED
```

Alternatively, allow anonymous LAN clients in the Mosquitto broker configuration if that matches the project security model.
