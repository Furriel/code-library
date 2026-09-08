#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <time.h>

/*
 * ESP32 SD ROTATING LOGGER
 * ========================
 *
 * Objetivo:
 *   Gravar uma linha por registro no microSD e organizar automaticamente os
 *   arquivos por data, sem impor CSV, JSONL ou outro formato ao usuario.
 *
 * LEIA NESTA ORDEM:
 *   [1] CONFIGURACAO
 *   [2] FONTE DE HORARIO
 *   [3] API PUBLICA
 */

enum class SdLogRotation {
  Hourly,
  Daily,
};

// [1] CONFIGURACAO
// ----------------
struct SdRotatingLoggerConfig {
  String root = "/logs";
  String prefix = "log";
  String extension = ".jsonl";
  SdLogRotation rotation = SdLogRotation::Hourly;
  bool flush_each_line = true;
  uint32_t mutex_timeout_ms = 2000;
};

// [2] FONTE DE HORARIO
// --------------------
// A funcao deve preencher *out e retornar true quando o horario for valido.
// Pode usar NTP, RTC, GPS ou qualquer outra referencia.
using SdLoggerTimeProvider = bool (*)(struct tm *out);

// [3] API PUBLICA
// ---------------
class SdRotatingLogger {
 public:
  explicit SdRotatingLogger(const SdRotatingLoggerConfig &config = SdRotatingLoggerConfig());

  // Monta o SD e prepara o mutex.
  bool begin(uint8_t cs_pin,
             SemaphoreHandle_t shared_sd_mutex = nullptr,
             SPIClass &spi = SPI);

  // Usa um SD que ja foi montado pela aplicacao.
  bool beginMounted(SemaphoreHandle_t shared_sd_mutex = nullptr);

  // Define a fonte usada por appendLine().
  void setTimeProvider(SdLoggerTimeProvider provider) { time_provider_ = provider; }

  // Obtem horario pelo provider e grava.
  bool appendLine(const char *line);

  // Grava usando um horario ja conhecido pelo chamador.
  bool appendLineAt(const char *line, const struct tm &when);

  // Gera o caminho sem gravar. Util para diagnostico e testes de integracao.
  String buildPathAt(const struct tm &when) const;

  SemaphoreHandle_t mutex() const { return sd_mutex_; }

 private:
  SdRotatingLoggerConfig config_;
  SdLoggerTimeProvider time_provider_ = nullptr;
  SemaphoreHandle_t sd_mutex_ = nullptr;
  bool owns_mutex_ = false;
  bool ready_ = false;

  String normalizeRoot(const String &root) const;
  String normalizeExtension(const String &extension) const;
  bool prepareMutex(SemaphoreHandle_t shared_sd_mutex);
  bool takeSd();
  void giveSd();
  bool ensureDirectoryTree(const String &directory);
  String directoryFor(const struct tm &when) const;
};
