#include "sd_rotating_logger.h"

#include <SD.h>
#include <cstring>

/*
 * OBJETIVO DESTE ARQUIVO
 * ----------------------
 * Concentrar a montagem do SD, criacao da arvore de diretorios, escolha do
 * arquivo corrente e escrita de uma linha.
 *
 * LEIA NESTA ORDEM:
 *   [1] INICIALIZACAO
 *   [2] DIRETORIOS
 *   [3] CAMINHO DO ARQUIVO
 *   [4] GRAVACAO
 *   [5] HORARIO AUTOMATICO
 */

SdRotatingLogger::SdRotatingLogger(const SdRotatingLoggerConfig &config)
    : config_(config) {
  config_.root = normalizeRoot(config.root);
  config_.extension = normalizeExtension(config.extension);
}

String SdRotatingLogger::normalizeRoot(const String &root) const {
  if (root.length() == 0 || root == "/") return "/";

  String out = root;
  if (!out.startsWith("/")) out = "/" + out;
  while (out.length() > 1 && out.endsWith("/")) out.remove(out.length() - 1);
  return out;
}

String SdRotatingLogger::normalizeExtension(const String &extension) const {
  if (extension.length() == 0) return "";
  return extension.startsWith(".") ? extension : "." + extension;
}

// [1] INICIALIZACAO
// -----------------
bool SdRotatingLogger::prepareMutex(SemaphoreHandle_t shared_sd_mutex) {
  if (sd_mutex_) return true;

  if (shared_sd_mutex) {
    sd_mutex_ = shared_sd_mutex;
    owns_mutex_ = false;
  } else {
    sd_mutex_ = xSemaphoreCreateMutex();
    owns_mutex_ = true;
  }
  return sd_mutex_ != nullptr;
}

bool SdRotatingLogger::takeSd() {
  if (!sd_mutex_) return false;
  return xSemaphoreTake(sd_mutex_, pdMS_TO_TICKS(config_.mutex_timeout_ms)) == pdTRUE;
}

void SdRotatingLogger::giveSd() {
  if (sd_mutex_) xSemaphoreGive(sd_mutex_);
}

bool SdRotatingLogger::begin(uint8_t cs_pin,
                             SemaphoreHandle_t shared_sd_mutex,
                             SPIClass &spi) {
  if (!prepareMutex(shared_sd_mutex)) return false;
  if (!takeSd()) return false;

  const bool mounted = SD.begin(cs_pin, spi);
  giveSd();

  ready_ = mounted && SD.cardType() != CARD_NONE;
  return ready_;
}

bool SdRotatingLogger::beginMounted(SemaphoreHandle_t shared_sd_mutex) {
  if (!prepareMutex(shared_sd_mutex)) return false;
  ready_ = SD.cardType() != CARD_NONE;
  return ready_;
}

// [2] DIRETORIOS
// --------------
// Cria cada nivel individualmente. SD.mkdir() nao cria toda a arvore de uma
// vez em todas as versoes do framework.
bool SdRotatingLogger::ensureDirectoryTree(const String &directory) {
  if (!directory.startsWith("/")) return false;
  if (directory == "/") return true;

  String current;
  int start = 1;
  while (start <= directory.length()) {
    int slash = directory.indexOf('/', start);
    if (slash < 0) slash = directory.length();

    const String part = directory.substring(start, slash);
    if (part.length() > 0) {
      current += "/";
      current += part;
      if (!SD.exists(current) && !SD.mkdir(current)) return false;
    }

    start = slash + 1;
  }
  return true;
}

String SdRotatingLogger::directoryFor(const struct tm &when) const {
  char date_path[24];
  strftime(date_path, sizeof(date_path), "%Y/%m/%d", &when);

  if (config_.root == "/") return "/" + String(date_path);
  return config_.root + "/" + String(date_path);
}

// [3] CAMINHO DO ARQUIVO
// ----------------------
String SdRotatingLogger::buildPathAt(const struct tm &when) const {
  const String directory = directoryFor(when);

  char timestamp[24];
  if (config_.rotation == SdLogRotation::Hourly) {
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d-%H", &when);
  } else {
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d", &when);
  }

  String path = directory + "/" + config_.prefix + "-" + timestamp;
  path += config_.extension;
  return path;
}

// [4] GRAVACAO
// ------------
// O mutex cobre criacao de pastas + open + write + flush + close. Assim, um
// servidor de download que compartilhe o mesmo mutex nao acessa o arquivo no
// meio da atualizacao.
bool SdRotatingLogger::appendLineAt(const char *line, const struct tm &when) {
  if (!ready_ || !line) return false;

  const String path = buildPathAt(when);
  const int slash = path.lastIndexOf('/');
  if (slash < 0) return false;
  const String directory = path.substring(0, slash);

  if (!takeSd()) return false;

  bool ok = ensureDirectoryTree(directory);
  if (ok) {
    File file = SD.open(path, FILE_APPEND);
    if (!file) {
      ok = false;
    } else {
      const size_t expected = strlen(line) + 1;  // println adiciona newline.
      const size_t written = file.println(line);
      if (config_.flush_each_line) file.flush();
      file.close();
      ok = written >= expected;
    }
  }

  giveSd();
  return ok;
}

// [5] HORARIO AUTOMATICO
// ----------------------
bool SdRotatingLogger::appendLine(const char *line) {
  if (!time_provider_) return false;

  struct tm now = {};
  if (!time_provider_(&now)) return false;
  return appendLineAt(line, now);
}
