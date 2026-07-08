/*
  Mini Germination Chamber - ESP32 soil moisture node

  Wiring for a separate ESP32 board:
    Soil sensor GND  -> ESP32 GND
    Soil sensor VCC  -> ESP32 3V3
    Soil sensor AOUT -> ESP32 GPIO32

  Endpoints:
    /        simple help page
    /status  JSON status with raw and calibrated moisture values
*/

#include "FS.h"
#include "SD.h"
#include "SD_MMC.h"
#include "SPI.h"
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

char WIFI_SSID[64] = "Khushali’s iPhone";
char WIFI_PASSWORD[64] = "REDACTED";
char NODE_NAME[32] = "germsoil-1";
char MQTT_HOST[64] = "192.168.1.10";
uint16_t MQTT_PORT = 1883;
char MQTT_USER[64] = DEFAULT_MQTT_USER;
char MQTT_PASSWORD[64] = DEFAULT_MQTT_PASSWORD;

int SOIL_PIN = 32;
int DRY_RAW = 3200;
int WET_RAW = 1400;
int SD_CS_PIN = 5;
int SD_SCK_PIN = 18;
int SD_MISO_PIN = 19;
int SD_MOSI_PIN = 23;
bool sdConfigLoaded = false;

WebServer server(80);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long lastMqttPublish = 0;
unsigned long lastMqttConnectAttempt = 0;
unsigned long lastSerialPrint = 0;
bool wifiReady = false;
const int ADC_SCAN_PINS[] = {32, 33, 34, 35, 36, 39};
const int ADC_DEAD_RAW = 10;
const int ADC_SIGNAL_RAW = 50;

void copyConfigValue(char *target, size_t targetSize, const String &value) {
  value.toCharArray(target, targetSize);
}

int analogReadAverage(int pin);

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
    if (value != "REDACTED") {
      copyConfigValue(WIFI_PASSWORD, sizeof(WIFI_PASSWORD), value);
    }
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
  } else if (key == "SOIL_PIN") {
    SOIL_PIN = value.toInt();
  } else if (key == "DRY_RAW") {
    DRY_RAW = value.toInt();
  } else if (key == "WET_RAW") {
    WET_RAW = value.toInt();
  } else {
    Serial.printf("Unknown config key: %s\n", key.c_str());
  }
}

bool loadConfigFile(fs::FS &fs, const char *sourceName) {
  File config = fs.open("/germsensor.cfg", FILE_READ);
  if (!config) {
    Serial.printf("No /germsensor.cfg on %s\n", sourceName);
    return false;
  }

  while (config.available()) {
    applyConfigLine(config.readStringUntil('\n'));
  }
  config.close();
  sdConfigLoaded = true;
  Serial.printf("Loaded /germsensor.cfg from %s\n", sourceName);
  return true;
}

void loadConfigFromSd() {
  if (SD_MMC.begin("/sdcard", true)) {
    if (loadConfigFile(SD_MMC, "SD_MMC")) {
      return;
    }
  } else {
    Serial.println("SD_MMC not mounted");
  }

  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (SD.begin(SD_CS_PIN, SPI)) {
    if (loadConfigFile(SD, "SPI SD")) {
      return;
    }
  } else {
    Serial.println("SPI SD not mounted");
  }

  Serial.println("microSD config not loaded; using sketch defaults");
}

void applySecretFallbacks() {
  if (strlen(MQTT_USER) == 0 && strlen(DEFAULT_MQTT_USER) > 0) {
    copyConfigValue(MQTT_USER, sizeof(MQTT_USER), DEFAULT_MQTT_USER);
  }

  if (strlen(MQTT_PASSWORD) == 0 && strlen(DEFAULT_MQTT_PASSWORD) > 0) {
    copyConfigValue(MQTT_PASSWORD, sizeof(MQTT_PASSWORD), DEFAULT_MQTT_PASSWORD);
  }
}

int soilRaw() {
  return analogReadAverage(SOIL_PIN);
}

int analogReadAverage(int pin) {
  const int samples = 16;
  uint32_t total = 0;
  analogRead(pin);
  delay(5);
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
    delay(2);
  }
  return total / samples;
}

void autoSelectSoilPin() {
  int configuredRaw = analogReadAverage(SOIL_PIN);
  int bestPin = SOIL_PIN;
  int bestRaw = configuredRaw;

  for (int pin : ADC_SCAN_PINS) {
    int raw = analogReadAverage(pin);
    if (raw > bestRaw) {
      bestRaw = raw;
      bestPin = pin;
    }
  }

  if (configuredRaw <= ADC_DEAD_RAW && bestPin != SOIL_PIN && bestRaw >= ADC_SIGNAL_RAW) {
    Serial.printf("Configured GPIO%d reads %d; using GPIO%d detected at %d\n",
                  SOIL_PIN, configuredRaw, bestPin, bestRaw);
    SOIL_PIN = bestPin;
    analogSetPinAttenuation(SOIL_PIN, ADC_11db);
  }
}

int soilPercentFromRaw(int raw) {
  if (DRY_RAW == WET_RAW) {
    return 0;
  }

  int percent = map(raw, DRY_RAW, WET_RAW, 0, 100);
  return constrain(percent, 0, 100);
}

String topic(const char *suffix) {
  String out = "germination/";
  out += NODE_NAME;
  out += "/";
  out += suffix;
  return out;
}

String statusJson() {
  int raw = soilRaw();
  int moisture = soilPercentFromRaw(raw);

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
  json += ",\"config_source\":\"";
  json += sdConfigLoaded ? "sd" : "sketch";
  json += "\",\"soil_pin\":";
  json += SOIL_PIN;
  json += ",\"soil_raw\":";
  json += raw;
  json += ",\"soil_moisture_percent\":";
  json += moisture;
  json += ",\"dry_raw\":";
  json += DRY_RAW;
  json += ",\"wet_raw\":";
  json += WET_RAW;
  json += "}";
  return json;
}

void sendNoCacheHeaders() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
}

void handleRoot() {
  String body;
  body += "<!doctype html><html><head><title>";
  body += NODE_NAME;
  body += "</title></head><body><h1>";
  body += NODE_NAME;
  body += "</h1><ul><li><a href=\"/status\">/status</a> soil status</li></ul></body></html>";
  server.send(200, "text/html", body);
}

void handleStatus() {
  sendNoCacheHeaders();
  server.send(200, "application/json", statusJson());
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("Connecting to %s", WIFI_SSID);
  unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 20000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  if (WiFi.status() != WL_CONNECTED) {
    wifiReady = false;
    Serial.println("WiFi not connected; continuing with serial soil readings only");
    return;
  }

  wifiReady = true;
  Serial.printf("%s ready: http://%s/\n", NODE_NAME, WiFi.localIP().toString().c_str());
}

void publishStatus() {
  String statusTopic = topic("status");
  String rawTopic = topic("soil_raw");
  String moistureTopic = topic("soil_moisture_percent");
  String availabilityTopic = topic("availability");

  int raw = soilRaw();
  int moisture = soilPercentFromRaw(raw);

  mqtt.publish(availabilityTopic.c_str(), "online", true);
  mqtt.publish(statusTopic.c_str(), statusJson().c_str(), true);
  mqtt.publish(rawTopic.c_str(), String(raw).c_str(), true);
  mqtt.publish(moistureTopic.c_str(), String(moisture).c_str(), true);

  Serial.printf("soil GPIO%d raw=%d moisture=%d%%\n", SOIL_PIN, raw, moisture);
}

void connectMqtt() {
  if (!wifiReady) {
    return;
  }

  if (mqtt.connected()) {
    return;
  }

  unsigned long now = millis();
  if (now - lastMqttConnectAttempt < 5000) {
    return;
  }
  lastMqttConnectAttempt = now;

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

  if (connected) {
    Serial.println("MQTT connected");
    publishStatus();
  } else {
    Serial.printf("MQTT connect failed, rc=%d\n", mqtt.state());
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  analogReadResolution(12);
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);
  for (int pin : ADC_SCAN_PINS) {
    analogSetPinAttenuation(pin, ADC_11db);
  }

  loadConfigFromSd();
  applySecretFallbacks();
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);
  for (int pin : ADC_SCAN_PINS) {
    analogSetPinAttenuation(pin, ADC_11db);
  }
  autoSelectSoilPin();

  Serial.printf("Soil sensor pin: GPIO%d\n", SOIL_PIN);
  publishStatus();

  connectWifi();

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.begin();

  mqtt.setBufferSize(512);
  connectMqtt();
}

void loop() {
  if (wifiReady) {
    server.handleClient();
  }

  connectMqtt();
  mqtt.loop();

  if (millis() - lastSerialPrint >= 2000) {
    lastSerialPrint = millis();
    int raw = soilRaw();
    int moisture = soilPercentFromRaw(raw);
    Serial.printf("soil GPIO%d raw=%d moisture=%d%%\n", SOIL_PIN, raw, moisture);
    Serial.print("adc scan:");
    for (int pin : ADC_SCAN_PINS) {
      Serial.printf(" GPIO%d=%d", pin, analogReadAverage(pin));
    }
    Serial.println();
  }

  if (millis() - lastMqttPublish >= 30000) {
    lastMqttPublish = millis();
    if (mqtt.connected()) {
      publishStatus();
    }
  }
}
