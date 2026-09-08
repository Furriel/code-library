#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>

#include "bridge_config.h"

// The packet wraps one JSON line with a small header. The magic and version
// fields let the receiver reject unrelated or incompatible ESP-NOW frames.
struct BridgePacket {
  uint16_t magic;
  uint8_t version;
  uint8_t reserved;
  uint32_t sequence;
  uint16_t length;
  char payload[BRIDGE_MAX_JSON_BYTES];
};

static uint32_t txSequence = 0;
static uint32_t txFrames = 0;
static uint32_t rxFrames = 0;
static uint32_t txErrors = 0;
static uint32_t rxErrors = 0;
static uint32_t lastHeartbeatMs = 0;
static String serialBuffer;

// Broadcast is the safest default for a first test. Use the PEER command at
// runtime to select one receiver without recompiling the firmware.
static uint8_t peerAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static void printJson(const JsonDocument &doc) {
  serializeJson(doc, Serial);
  Serial.println();
}

static void printError(const char *code, const char *message) {
  StaticJsonDocument<192> doc;
  doc["type"] = "bridge_error";
  doc["role"] = BRIDGE_ROLE_NAME;
  doc["code"] = code;
  doc["message"] = message;
  printJson(doc);
}

static bool isJsonObject(const String &text) {
  String value = text;
  value.trim();
  return value.startsWith("{") && value.endsWith("}");
}

static bool addPeer() {
  if (esp_now_is_peer_exist(peerAddress)) return true;

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, peerAddress, sizeof(peerAddress));
  peer.channel = 0;
  peer.encrypt = false;

  return esp_now_add_peer(&peer) == ESP_OK;
}

static bool sendJson(const char *json) {
  const size_t length = strnlen(json, BRIDGE_MAX_JSON_BYTES + 1);
  if (length == 0 || length >= BRIDGE_MAX_JSON_BYTES) {
    txErrors++;
    printError("PAYLOAD_SIZE", "JSON payload is too large");
    return false;
  }

  BridgePacket packet{};
  packet.magic = BRIDGE_PACKET_MAGIC;
  packet.version = BRIDGE_PACKET_VERSION;
  packet.sequence = txSequence++;
  packet.length = static_cast<uint16_t>(length);
  memcpy(packet.payload, json, length);
  packet.payload[length] = '\0';

  const esp_err_t result = esp_now_send(
    peerAddress,
    reinterpret_cast<const uint8_t *>(&packet),
    sizeof(packet)
  );

  if (result != ESP_OK) {
    txErrors++;
    printError("ESPNOW_SEND", "esp_now_send failed");
    return false;
  }

  txFrames++;
  return true;
}

static void onEspNowSend(const uint8_t *macAddress, esp_now_send_status_t status) {
  (void)macAddress;
  if (status != ESP_NOW_SEND_SUCCESS) txErrors++;
}

static void onEspNowReceive(const uint8_t *macAddress, const uint8_t *data, int dataLength) {
  (void)macAddress;

  if (dataLength != static_cast<int>(sizeof(BridgePacket))) {
    rxErrors++;
    return;
  }

  BridgePacket packet{};
  memcpy(&packet, data, sizeof(packet));

  if (packet.magic != BRIDGE_PACKET_MAGIC ||
      packet.version != BRIDGE_PACKET_VERSION ||
      packet.length == 0 ||
      packet.length >= BRIDGE_MAX_JSON_BYTES) {
    rxErrors++;
    return;
  }

  packet.payload[packet.length] = '\0';
  if (!isJsonObject(String(packet.payload))) {
    rxErrors++;
    return;
  }

  // Received JSON is printed unchanged. This makes a PC application able to
  // use the bridge as a transparent line-based transport.
  Serial.println(packet.payload);
  rxFrames++;
}

static bool parseMacAddress(const String &text, uint8_t output[6]) {
  if (text.length() != 17) return false;

  for (int i = 0; i < 6; ++i) {
    const int pos = i * 3;
    if (i < 5 && text[pos + 2] != ':') return false;

    char pair[3] = {text[pos], text[pos + 1], '\0'};
    char *end = nullptr;
    const long value = strtol(pair, &end, 16);
    if (*end != '\0' || value < 0 || value > 255) return false;

    output[i] = static_cast<uint8_t>(value);
  }

  return true;
}

static void printStatus() {
  StaticJsonDocument<256> doc;
  doc["type"] = "bridge_status";
  doc["role"] = BRIDGE_ROLE_NAME;
  doc["tx_frames"] = txFrames;
  doc["rx_frames"] = rxFrames;
  doc["tx_errors"] = txErrors;
  doc["rx_errors"] = rxErrors;
  doc["uptime_ms"] = millis();
  printJson(doc);
}

static void sendHeartbeat() {
  StaticJsonDocument<192> doc;
  doc["type"] = "bridge_heartbeat";
  doc["role"] = BRIDGE_ROLE_NAME;
  doc["uptime_ms"] = millis();

  char buffer[BRIDGE_MAX_JSON_BYTES];
  const size_t written = serializeJson(doc, buffer, sizeof(buffer));
  if (written == 0 || written >= sizeof(buffer)) {
    txErrors++;
    return;
  }

  sendJson(buffer);
}

static void processLine(String line) {
  line.trim();
  if (line.isEmpty()) return;

  if (line.equalsIgnoreCase("STATUS")) {
    printStatus();
    return;
  }

  if (line.startsWith("PEER ")) {
    String mac = line.substring(5);
    mac.trim();

    uint8_t parsed[6];
    if (!parseMacAddress(mac, parsed)) {
      printError("PEER_FORMAT", "use PEER AA:BB:CC:DD:EE:FF");
      return;
    }

    memcpy(peerAddress, parsed, sizeof(peerAddress));
    if (!addPeer()) {
      printError("PEER_ADD", "could not add ESP-NOW peer");
      return;
    }

    StaticJsonDocument<160> doc;
    doc["type"] = "bridge_info";
    doc["message"] = "peer updated";
    doc["peer"] = mac;
    printJson(doc);
    return;
  }

  if (!isJsonObject(line)) {
    printError("INPUT", "send JSON, STATUS or PEER command");
    return;
  }

  // ArduinoJson is used only as a syntax check. The bridge does not interpret
  // application fields, which keeps this feature reusable.
  StaticJsonDocument<BRIDGE_MAX_JSON_BYTES> doc;
  if (deserializeJson(doc, line)) {
    printError("JSON", "invalid JSON");
    return;
  }

  sendJson(line.c_str());
}

static void processSerial() {
  while (Serial.available() > 0) {
    const char ch = static_cast<char>(Serial.read());

    if (ch == '\r') continue;

    if (ch == '\n') {
      processLine(serialBuffer);
      serialBuffer = "";
      continue;
    }

    if (serialBuffer.length() < BRIDGE_MAX_JSON_BYTES + 32) {
      serialBuffer += ch;
    } else {
      serialBuffer = "";
      printError("SERIAL_OVERFLOW", "serial line is too long");
    }
  }
}

static void setupEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    printError("ESPNOW_INIT", "esp_now_init failed");
    return;
  }

  if (!addPeer()) printError("PEER_ADD", "could not add default peer");

  esp_now_register_send_cb(onEspNowSend);
  esp_now_register_recv_cb(onEspNowReceive);
}

void setup() {
  Serial.begin(BRIDGE_BAUDRATE);
  delay(300);

  setupEspNow();

  StaticJsonDocument<160> doc;
  doc["type"] = "bridge_boot";
  doc["role"] = BRIDGE_ROLE_NAME;
  doc["baudrate"] = BRIDGE_BAUDRATE;
  printJson(doc);
}

void loop() {
  processSerial();

  const uint32_t now = millis();
  if (now - lastHeartbeatMs >= BRIDGE_HEARTBEAT_MS) {
    lastHeartbeatMs = now;
    sendHeartbeat();
  }

  delay(2);
}
