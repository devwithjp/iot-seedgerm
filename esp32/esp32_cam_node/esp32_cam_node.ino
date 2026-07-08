/*
  Mini Germination Chamber - ESP32-CAM node

  Target board: Freenove ESP32-WROVER CAM / WROVER_KIT pin map.
  First-stage purpose: verify camera hardware, fixed framing, and Wi-Fi access.

  Endpoints:
    /        simple help page
    /status  JSON status
    /jpg     one JPEG frame
    /stream  MJPEG stream
*/

#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>

#if __has_include("local_secrets.h")
#include "local_secrets.h"
#endif

#ifndef DEFAULT_MQTT_USER
#define DEFAULT_MQTT_USER ""
#endif

#ifndef DEFAULT_MQTT_PASSWORD
#define DEFAULT_MQTT_PASSWORD ""
#endif

// Defaults can be overridden by /germcam.cfg on a FAT-formatted microSD card.
char WIFI_SSID[64] = "REDACTED";
char WIFI_PASSWORD[64] = "REDACTED";
char NODE_NAME[32] = "germcam-1";  // Use germcam-2 on the second ESP32-CAM.
char MQTT_HOST[64] = "192.168.1.10";  // Mosquitto IP address or hotspot forwarder.
uint16_t MQTT_PORT = 1883;
char MQTT_USER[64] = DEFAULT_MQTT_USER;      // Leave empty if Mosquitto allows anonymous LAN clients.
char MQTT_PASSWORD[64] = DEFAULT_MQTT_PASSWORD;  // Leave empty if Mosquitto allows anonymous LAN clients.
bool sdConfigLoaded = false;
bool cameraReady = false;

WebServer server(80);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long lastMqttPublish = 0;
unsigned long lastMqttConnectAttempt = 0;

// Freenove ESP32-WROVER CAM / WROVER_KIT pin map.
// Freenove's camera tutorial selects CAMERA_MODEL_WROVER_KIT.
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 21
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 19
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 5
#define Y2_GPIO_NUM 4
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

static const char *STREAM_BOUNDARY = "frame";

void copyConfigValue(char *target, size_t targetSize, const String &value) {
  value.toCharArray(target, targetSize);
}

void applyConfigLine(String line) {
  int commentAt = line.indexOf('#');
  if (commentAt >= 0) {
    line = line.substring(0, commentAt);
  }

  line.trim();
  if (line.length() == 0) {
    return;
  }

  int equalsAt = line.indexOf('=');
  if (equalsAt <= 0) {
    Serial.printf("Ignoring config line: %s\n", line.c_str());
    return;
  }

  String key = line.substring(0, equalsAt);
  String value = line.substring(equalsAt + 1);
  key.trim();
  value.trim();

  if (key == "WIFI_SSID") {
    if (value == "REDACTED") {
      value = "Khushali’s iPhone";
    }
    copyConfigValue(WIFI_SSID, sizeof(WIFI_SSID), value);
  } else if (key == "WIFI_PASSWORD") {
    copyConfigValue(WIFI_PASSWORD, sizeof(WIFI_PASSWORD), value);
  } else if (key == "NODE_NAME") {
    copyConfigValue(NODE_NAME, sizeof(NODE_NAME), value);
  } else if (key == "MQTT_HOST") {
    if (value == "10.42.0.75") {
      value = "172.20.10.4";
    }
    copyConfigValue(MQTT_HOST, sizeof(MQTT_HOST), value);
  } else if (key == "MQTT_PORT") {
    MQTT_PORT = static_cast<uint16_t>(value.toInt());
  } else if (key == "MQTT_USER") {
    copyConfigValue(MQTT_USER, sizeof(MQTT_USER), value);
  } else if (key == "MQTT_PASSWORD") {
    copyConfigValue(MQTT_PASSWORD, sizeof(MQTT_PASSWORD), value);
  } else {
    Serial.printf("Unknown config key: %s\n", key.c_str());
  }
}

void loadConfigFromSd() {
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("microSD not mounted; using sketch defaults");
    return;
  }

  File config = SD_MMC.open("/germcam.cfg", FILE_READ);
  if (!config) {
    Serial.println("No /germcam.cfg on microSD; using sketch defaults");
    return;
  }

  while (config.available()) {
    applyConfigLine(config.readStringUntil('\n'));
  }
  config.close();
  sdConfigLoaded = true;
  Serial.println("Loaded /germcam.cfg from microSD");
}

void applySecretFallbacks() {
  if (strlen(MQTT_USER) == 0 && strlen(DEFAULT_MQTT_USER) > 0) {
    copyConfigValue(MQTT_USER, sizeof(MQTT_USER), DEFAULT_MQTT_USER);
  }

  if (strlen(MQTT_PASSWORD) == 0 && strlen(DEFAULT_MQTT_PASSWORD) > 0) {
    copyConfigValue(MQTT_PASSWORD, sizeof(MQTT_PASSWORD), DEFAULT_MQTT_PASSWORD);
  }
}

void sendNoCacheHeaders() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
}

void handleRoot() {
  String body;
  body += "<!doctype html><html><head><title>";
  body += NODE_NAME;
  body += "</title></head><body>";
  body += "<h1>";
  body += NODE_NAME;
  body += "</h1>";
  body += "<ul>";
  body += "<li><a href=\"/jpg\">/jpg</a> single capture</li>";
  body += "<li><a href=\"/stream\">/stream</a> live MJPEG stream</li>";
  body += "<li><a href=\"/status\">/status</a> node status</li>";
  body += "</ul>";
  body += "<img src=\"/stream\" style=\"max-width:100%;height:auto;\">";
  body += "</body></html>";
  server.send(200, "text/html", body);
}

void handleStatus() {
  String json = statusJson();
  sendNoCacheHeaders();
  server.send(200, "application/json", json);
}

String topic(const char *suffix) {
  String out = "germination/";
  out += NODE_NAME;
  out += "/";
  out += suffix;
  return out;
}

String statusJson() {
  String json;
  json += "{";
  json += "\"node\":\"";
  json += NODE_NAME;
  json += "\",\"ip\":\"";
  json += WiFi.localIP().toString();
  json += "\",\"rssi\":";
  json += WiFi.RSSI();
  json += ",\"uptime_ms\":";
  json += millis();
  json += ",\"free_heap\":";
  json += ESP.getFreeHeap();
  json += ",\"camera_ready\":";
  json += cameraReady ? "true" : "false";
  json += ",\"config_source\":\"";
  json += sdConfigLoaded ? "sd" : "sketch";
  json += "\",\"jpg_url\":\"http://";
  json += WiFi.localIP().toString();
  json += "/jpg\",\"stream_url\":\"http://";
  json += WiFi.localIP().toString();
  json += "/stream\"";
  json += "}";
  return json;
}

void handleJpg() {
  if (!cameraReady) {
    server.send(503, "text/plain", "camera not ready");
    return;
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    server.send(503, "text/plain", "camera capture failed");
    return;
  }

  sendNoCacheHeaders();
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.send_P(200, "image/jpeg", reinterpret_cast<const char *>(fb->buf), fb->len);
  esp_camera_fb_return(fb);
}

void handleStream() {
  if (!cameraReady) {
    server.send(503, "text/plain", "camera not ready");
    return;
  }

  WiFiClient client = server.client();

  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=";
  response += STREAM_BOUNDARY;
  response += "\r\nCache-Control: no-cache\r\nPragma: no-cache\r\n\r\n";
  server.sendContent(response);

  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      break;
    }

    client.printf("--%s\r\n", STREAM_BOUNDARY);
    client.print("Content-Type: image/jpeg\r\n");
    client.printf("Content-Length: %u\r\n\r\n", fb->len);
    client.write(fb->buf, fb->len);
    client.print("\r\n");

    esp_camera_fb_return(fb);
    delay(100);
  }
}

bool configureCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  sensor->set_framesize(sensor, FRAMESIZE_SVGA);
  sensor->set_quality(sensor, 12);
  sensor->set_brightness(sensor, 0);
  sensor->set_contrast(sensor, 0);
  sensor->set_saturation(sensor, 0);
  return true;
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(NODE_NAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.printf("%s ready: http://%s/\n", NODE_NAME, WiFi.localIP().toString().c_str());
}

void publishMqttStatus() {
  if (!mqtt.connected()) {
    return;
  }

  String payload = statusJson();
  String jpgUrl = "http://" + WiFi.localIP().toString() + "/jpg";
  String streamUrl = "http://" + WiFi.localIP().toString() + "/stream";
  mqtt.publish(topic("status").c_str(), payload.c_str(), true);
  mqtt.publish(topic("jpg_url").c_str(), jpgUrl.c_str(), true);
  mqtt.publish(topic("stream_url").c_str(), streamUrl.c_str(), true);
}

void connectMqtt() {
  if (mqtt.connected()) {
    return;
  }

  if (millis() - lastMqttConnectAttempt < 10000) {
    return;
  }
  lastMqttConnectAttempt = millis();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  Serial.printf("Connecting to MQTT %s:%u\n", MQTT_HOST, MQTT_PORT);

  String availabilityTopic = topic("availability");
  bool connected;
  if (strlen(MQTT_USER) > 0) {
    connected = mqtt.connect(NODE_NAME, MQTT_USER, MQTT_PASSWORD,
                             availabilityTopic.c_str(), 1, true, "offline");
  } else {
    connected = mqtt.connect(NODE_NAME, availabilityTopic.c_str(), 1, true, "offline");
  }

  if (!connected) {
    Serial.printf("MQTT connect failed, rc=%d\n", mqtt.state());
    return;
  }

  mqtt.publish(availabilityTopic.c_str(), "online", true);
  publishMqttStatus();
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  delay(1000);

  loadConfigFromSd();
  applySecretFallbacks();
  cameraReady = configureCamera();
  if (!cameraReady) {
    Serial.println("Camera not ready; HTTP status page will still start");
  }
  connectWifi();
  connectMqtt();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/jpg", HTTP_GET, handleJpg);
  server.on("/stream", HTTP_GET, handleStream);
  server.begin();
}

void loop() {
  server.handleClient();

  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt.connected()) {
      connectMqtt();
    }
    mqtt.loop();
  }

  if (millis() - lastMqttPublish > 30000) {
    publishMqttStatus();
    lastMqttPublish = millis();
  }
}
