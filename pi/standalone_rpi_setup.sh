#!/usr/bin/env bash
set -euo pipefail

# Standalone Raspberry Pi hub setup for the Mini Germination Chamber.
# Run on Raspberry Pi OS Lite/Desktop, not Home Assistant OS:
#   sudo bash standalone_rpi_setup.sh

AP_SSID="${AP_SSID:-REDACTED}"
AP_PASSWORD="${AP_PASSWORD:-REDACTED}"
AP_ADDRESS="${AP_ADDRESS:-192.168.50.1/24}"
MQTT_USER="${MQTT_USER:-iota}"
MQTT_PASSWORD="REDACTED"
PROJECT_DIR="${PROJECT_DIR:-/opt/germination-chamber}"
CAPTURE_INTERVAL_SECONDS="${CAPTURE_INTERVAL_SECONDS:-300}"

if [[ "${EUID}" -ne 0 ]]; then
  echo "Run as root: sudo bash $0" >&2
  exit 1
fi

if [[ "${#AP_PASSWORD}" -lt 8 ]]; then
  echo "AP_PASSWORD must be at least 8 characters for WPA2." >&2
  exit 1
fi

export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install -y \
  avahi-daemon \
  curl \
  libnss-mdns \
  mosquitto \
  mosquitto-clients \
  network-manager \
  python3 \
  python3-venv \
  python3-pip \
  rsync

systemctl enable --now NetworkManager
systemctl enable --now avahi-daemon

WIFI_DEVICE="$(nmcli -t -f DEVICE,TYPE,STATE device | awk -F: '$2=="wifi"{print $1; exit}')"
if [[ -z "${WIFI_DEVICE}" ]]; then
  echo "No Wi-Fi device found." >&2
  exit 1
fi

if nmcli connection show REDACTEDAP >/dev/null 2>&1; then
  nmcli connection delete REDACTEDAP
fi

nmcli connection add \
  type wifi \
  ifname "${WIFI_DEVICE}" \
  con-name REDACTEDAP \
  autoconnect yes \
  ssid "${AP_SSID}"

nmcli connection modify REDACTEDAP \
  802-11-wireless.mode ap \
  802-11-wireless.band bg \
  ipv4.method shared \
  ipv4.addresses "${AP_ADDRESS}" \
  ipv6.method ignore \
  wifi-sec.key-mgmt wpa-psk \
  wifi-sec.psk "${AP_PASSWORD}"

nmcli connection up REDACTEDAP

install -d -m 0755 /etc/mosquitto/conf.d
cat >/etc/mosquitto/conf.d/germination.conf <<EOF
listener 1883 0.0.0.0
allow_anonymous false
password_file /etc/mosquitto/passwd
persistence true
persistence_location /var/lib/mosquitto/
EOF

touch /etc/mosquitto/passwd
mosquitto_passwd -b /etc/mosquitto/passwd "${MQTT_USER}" "${MQTT_PASSWORD}"
chown mosquitto:mosquitto /etc/mosquitto/passwd
chmod 0640 /etc/mosquitto/passwd
systemctl enable --now mosquitto
systemctl restart mosquitto

install -d -m 0755 "${PROJECT_DIR}"
if [[ -f ./capture_esp32_cam_images.py ]]; then
  rsync -a ./ "${PROJECT_DIR}/"
elif [[ -d ./project/pi ]]; then
  rsync -a ./project/pi/ "${PROJECT_DIR}/"
else
  echo "Copy project/pi files into ${PROJECT_DIR} manually after this script." >&2
fi

install -d -m 0755 "${PROJECT_DIR}/images" "${PROJECT_DIR}/logs"

cat >/etc/systemd/system/germcam-capture.service <<EOF
[Unit]
Description=Capture ESP32-CAM germination images
After=network-online.target
Wants=network-online.target

[Service]
Type=oneshot
WorkingDirectory=${PROJECT_DIR}
ExecStart=/usr/bin/python3 ${PROJECT_DIR}/capture_esp32_cam_images.py --output-dir ${PROJECT_DIR}/images --log ${PROJECT_DIR}/logs/capture_events.jsonl
EOF

cat >/etc/systemd/system/germcam-capture.timer <<EOF
[Unit]
Description=Capture ESP32-CAM images every ${CAPTURE_INTERVAL_SECONDS} seconds

[Timer]
OnBootSec=2min
OnUnitActiveSec=${CAPTURE_INTERVAL_SECONDS}
Persistent=true

[Install]
WantedBy=timers.target
EOF

systemctl daemon-reload
systemctl enable --now germcam-capture.timer

cat <<EOF

Standalone Pi hub setup complete.

Wi-Fi AP:
  SSID: ${AP_SSID}
  Password: ${AP_PASSWORD}
  Pi IP: ${AP_ADDRESS%/*}

MQTT:
  Host: ${AP_ADDRESS%/*}
  Port: 1883
  User: ${MQTT_USER}
  Password: ${MQTT_PASSWORD}

Next:
  1. Configure ESP32 SD files to use WIFI_SSID=${AP_SSID} and MQTT_HOST=${AP_ADDRESS%/*}.
  2. Connect the ESP32-CAM by USB-C for 5V power, flashing, and Serial Monitor logs.
     Camera images and MQTT still travel over the Pi Wi-Fi AP, not over USB-C.
  3. Keep Ethernet connected if you want LAN/SSH/internet access while the Pi runs the AP.
  4. Install Home Assistant separately if this Pi is not already running it.
  5. Test MQTT with:
     mosquitto_sub -h ${AP_ADDRESS%/*} -u ${MQTT_USER} -P '${MQTT_PASSWORD}' -t 'germination/#' -v
  6. Test camera capture after the ESP32 joins Wi-Fi:
     python3 ${PROJECT_DIR}/capture_esp32_cam_images.py --output-dir ${PROJECT_DIR}/images --log ${PROJECT_DIR}/logs/capture_events.jsonl
EOF
