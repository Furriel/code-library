#include <Arduino.h>
#include <SD.h>
#include <WiFi.h>

#include "sd_file_server.h"

/*
 * EXEMPLO MINIMO
 * --------------
 * 1. conecta o ESP32 ao Wi-Fi;
 * 2. monta o microSD;
 * 3. inicia o servidor;
 * 4. atende o navegador no loop().
 */

constexpr const char *WIFI_SSID = "YOUR_WIFI_SSID";
constexpr const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
constexpr uint8_t SD_CS_PIN = 5;

SdFileServer *fileServer = nullptr;

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 15000) {
    delay(250);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi connection failed");
    return;
  }

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD mount failed");
    return;
  }

  SdFileServerConfig config;
  config.root = "/";
  config.allowed_extension = "";  // Ex.: ".csv" para listar apenas CSV.
  config.page_title = "ESP32 SD File Server";

  // A instancia e estatica porque precisa continuar existindo depois de setup().
  static SdFileServer configuredServer(config);
  fileServer = &configuredServer;

  if (!fileServer->begin()) {
    Serial.println("File server start failed");
    fileServer = nullptr;
    return;
  }

  Serial.print("Open http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void loop() {
  if (fileServer) fileServer->handleClient();
  delay(2);
}
