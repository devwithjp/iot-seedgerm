# Home Assistant MQTT dashboard

Use this after:

- Home Assistant is reachable at `http://homeassistant.local:8123`.
- Mosquitto is installed and running.
- Both ESP32-CAM nodes are publishing `germination/#` MQTT topics.

Official docs checked on 7 July 2026:

- MQTT integration: <https://www.home-assistant.io/integrations/mqtt/>
- MQTT sensor: <https://www.home-assistant.io/integrations/sensor.mqtt/>
- MQTT binary sensor: <https://www.home-assistant.io/integrations/binary_sensor.mqtt/>
- Generic Camera: <https://www.home-assistant.io/integrations/generic/>

## Add MQTT entities

Copy this project file:

```text
homeassistant/packages/germcam_mqtt.yaml
```

to the Home Assistant config directory:

```text
/config/packages/germcam_mqtt.yaml
```

If packages are not enabled yet, add this to `/config/configuration.yaml`:

```yaml
homeassistant:
  packages: !include_dir_named packages
```

Restart Home Assistant.

After restart, these entities should exist:

```text
binary_sensor.germcam_1_online
binary_sensor.germcam_2_online
sensor.germcam_1_rssi
sensor.germcam_2_rssi
sensor.germcam_1_jpeg_url
sensor.germcam_2_jpeg_url
sensor.germcam_1_stream_url
sensor.germcam_2_stream_url
```

## Add camera previews

In Home Assistant:

1. Go to `Settings > Devices & services`.
2. Select `Add integration`.
3. Search for `Generic Camera`.
4. Add `Germcam 1`:
   - Still Image URL: `http://germcam-1.local/jpg`
   - Stream Source URL: `http://germcam-1.local/stream`
5. Add `Germcam 2`:
   - Still Image URL: `http://germcam-2.local/jpg`
   - Stream Source URL: `http://germcam-2.local/stream`

If `.local` names do not resolve, use each camera IP address instead.

## Add dashboard cards

Use this project file as a starting point:

```text
homeassistant/dashboard_germcam_cards.yaml
```

Paste the cards into a manual dashboard view after the Generic Camera entities exist.

If Home Assistant creates different camera entity IDs, update:

```text
camera.germcam_1
camera.germcam_2
```

to match the actual entity IDs shown in `Settings > Devices & services > Entities`.

## Test MQTT from the Pi

Listen for camera topics:

```bash
mosquitto_sub -h localhost -t 'germination/#' -v
```

Expected messages include:

```text
germination/germcam-1/availability online
germination/germcam-1/status {"node":"germcam-1","ip":"192.168.1.42","rssi":-55,"uptime_ms":30122,"free_heap":123456,"jpg_url":"http://192.168.1.42/jpg","stream_url":"http://192.168.1.42/stream"}
germination/germcam-1/jpg_url http://192.168.1.42/jpg
germination/germcam-1/stream_url http://192.168.1.42/stream
```

## Definition of done

- Home Assistant shows both camera nodes as online.
- RSSI and heap values update every 30 seconds.
- Generic Camera previews show both ESP32-CAM images.
- The dashboard shows camera previews and health entities in one view.
