#include <Arduino.h>

#include "sd_rotating_logger.h"

constexpr uint8_t SD_CS_PIN = 5;

SdRotatingLoggerConfig config;
SdRotatingLogger logger(config);

void setup() {
  Serial.begin(115200);

  config.root = "/data";
  config.prefix = "example";
  config.extension = ".csv";
  config.rotation = SdLogRotation::Hourly;

  static SdRotatingLogger configuredLogger(config);
  if (!configuredLogger.begin(SD_CS_PIN)) {
    Serial.println("SD logger start failed");
    return;
  }

  // Horario fixo apenas para tornar o exemplo totalmente independente de
  // Wi-Fi, NTP ou RTC. Em uma aplicacao real, forneca o horario real.
  struct tm when = {};
  when.tm_year = 2026 - 1900;
  when.tm_mon = 9 - 1;
  when.tm_mday = 8;
  when.tm_hour = 14;
  when.tm_min = 30;

  const String path = configuredLogger.buildPathAt(when);
  Serial.print("Writing to: ");
  Serial.println(path);

  if (configuredLogger.appendLineAt("temperature_c,25.4", when)) {
    Serial.println("PASS write");
  } else {
    Serial.println("FAIL write");
  }
}

void loop() {
  delay(1000);
}
