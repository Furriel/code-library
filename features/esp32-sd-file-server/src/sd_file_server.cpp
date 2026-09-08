#include "sd_file_server.h"

#include <SD.h>

/*
 * OBJETIVO DESTE ARQUIVO
 * ----------------------
 * Implementar a navegacao e o download de arquivos do microSD por HTTP.
 *
 * FLUXO PRINCIPAL:
 *   navegador -> WebServer -> valida caminho -> trava SD -> le arquivo -> resposta
 *
 * LEIA NESTA ORDEM:
 *   [1] CONSTRUCAO E MUTEX
 *   [2] VALIDACAO DE CAMINHO
 *   [3] TIPO DE CONTEUDO
 *   [4] ESCAPE HTML E URL
 *   [5] ROTAS SIMPLES
 *   [6] LISTAGEM DE ARQUIVOS
 *   [7] DOWNLOAD
 *   [8] INICIALIZACAO
 */

// [1] CONSTRUCAO E MUTEX
// ---------------------
SdFileServer::SdFileServer(const SdFileServerConfig &config)
    : config_(config), server_(config.port) {
  config_.root = normalizeRoot(config_.root);
}

bool SdFileServer::takeSd() {
  if (!sd_mutex_) return false;
  return xSemaphoreTake(sd_mutex_, pdMS_TO_TICKS(config_.mutex_timeout_ms)) == pdTRUE;
}

void SdFileServer::giveSd() {
  if (sd_mutex_) xSemaphoreGive(sd_mutex_);
}

String SdFileServer::normalizeRoot(const String &root) const {
  if (root.length() == 0 || root == "/") return "/";

  String normalized = root;
  if (!normalized.startsWith("/")) normalized = "/" + normalized;
  while (normalized.length() > 1 && normalized.endsWith("/")) {
    normalized.remove(normalized.length() - 1);
  }
  return normalized;
}

// [2] VALIDACAO DE CAMINHO
// -----------------------
// Esta funcao e a barreira principal contra directory traversal.
// O servidor so aceita caminhos absolutos, sem "..", dentro da raiz escolhida
// e, opcionalmente, com a extensao configurada.
bool SdFileServer::safePath(const String &path) const {
  if (!path.startsWith("/")) return false;
  if (path.indexOf("..") >= 0) return false;

  if (config_.root != "/") {
    const String child_prefix = config_.root + "/";
    if (path != config_.root && !path.startsWith(child_prefix)) return false;
  }

  if (config_.allowed_extension.length() > 0 &&
      !path.endsWith(config_.allowed_extension)) {
    return false;
  }

  return true;
}

// [3] TIPO DE CONTEUDO
// --------------------
// O Content-Type correto facilita abrir textos no navegador. Arquivos
// desconhecidos sao enviados como binario generico.
String SdFileServer::contentTypeFor(const String &path) const {
  if (path.endsWith(".csv")) return "text/csv";
  if (path.endsWith(".json")) return "application/json";
  if (path.endsWith(".jsonl") || path.endsWith(".ndjson")) return "application/x-ndjson";
  if (path.endsWith(".txt") || path.endsWith(".log")) return "text/plain";
  if (path.endsWith(".html")) return "text/html";
  return "application/octet-stream";
}

String SdFileServer::baseName(const String &path) const {
  const int slash = path.lastIndexOf('/');
  return slash >= 0 ? path.substring(slash + 1) : path;
}

// [4] ESCAPE HTML E URL
// --------------------
// Sao duas codificacoes diferentes:
//   - htmlEscape protege o texto exibido no HTML;
//   - urlEncode transforma o caminho em parametro seguro para /download.
String SdFileServer::htmlEscape(const String &value) const {
  String out;
  out.reserve(value.length() + 16);
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c == '&') out += F("&amp;");
    else if (c == '<') out += F("&lt;");
    else if (c == '>') out += F("&gt;");
    else if (c == '"') out += F("&quot;");
    else if (c == '\'') out += F("&#39;");
    else out += c;
  }
  return out;
}

String SdFileServer::urlEncode(const String &value) const {
  // Arduino define HEX como macro numerica para Serial.print(..., HEX).
  // Por isso o nome abaixo evita uma colisao de pre-processador pouco obvia.
  static const char HEX_DIGITS[] = "0123456789ABCDEF";
  String out;
  out.reserve(value.length() * 3);

  for (size_t i = 0; i < value.length(); ++i) {
    const uint8_t c = static_cast<uint8_t>(value[i]);
    const bool unreserved =
        (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~';
    if (unreserved) {
      out += static_cast<char>(c);
    } else {
      out += '%';
      out += HEX_DIGITS[c >> 4];
      out += HEX_DIGITS[c & 0x0F];
    }
  }
  return out;
}

// [5] ROTAS SIMPLES
// -----------------
void SdFileServer::handleRoot() {
  String html;
  html.reserve(512);
  html += F("<!doctype html><html><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>");
  html += htmlEscape(config_.page_title);
  html += F("</title><body><h1>");
  html += htmlEscape(config_.page_title);
  html += F("</h1><p><a href='/files'>Listar arquivos do SD</a></p>");
  html += F("<p><a href='/health'>Health</a></p></body></html>");
  server_.send(200, "text/html; charset=utf-8", html);
}

void SdFileServer::handleHealth() {
  const bool sd_ready = SD.cardType() != CARD_NONE;
  String body = F("{\"status\":\"");
  body += sd_ready ? F("ok") : F("sd_unavailable");
  body += F("\",\"root\":\"");
  body += config_.root;
  body += F("\"}");
  server_.send(sd_ready ? 200 : 503, "application/json", body);
}

// [6] LISTAGEM DE ARQUIVOS
// -----------------------
// A resposta e enviada em blocos. Assim, a listagem nao precisa ser montada
// inteira na RAM antes de chegar ao navegador.
void SdFileServer::streamDirectory(const String &path, uint8_t depth) {
  if (depth > config_.max_depth) return;

  File directory = SD.open(path);
  if (!directory || !directory.isDirectory()) {
    if (directory) directory.close();
    return;
  }

  for (File entry = directory.openNextFile(); entry; entry = directory.openNextFile()) {
    String entry_path = entry.path();
    if (entry_path.length() == 0) {
      // Compatibilidade com versoes em que name() retorna apenas o nome local.
      entry_path = path;
      if (!entry_path.endsWith("/")) entry_path += "/";
      entry_path += entry.name();
    }

    if (entry.isDirectory()) {
      server_.sendContent("<li><strong>" + htmlEscape(entry_path) + "/</strong><ul>");
      entry.close();
      streamDirectory(entry_path, depth + 1);
      server_.sendContent(F("</ul></li>"));
      continue;
    }

    const bool allowed = safePath(entry_path);
    if (allowed) {
      String row = F("<li><a href='/download?file=");
      row += urlEncode(entry_path);
      row += F("'>");
      row += htmlEscape(entry_path);
      row += F("</a> - ");
      row += String(entry.size());
      row += F(" bytes</li>");
      server_.sendContent(row);
    }
    entry.close();
  }

  directory.close();
}

void SdFileServer::handleFiles() {
  if (!takeSd()) {
    server_.send(503, "text/plain", "SD busy");
    return;
  }

  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "text/html; charset=utf-8", "");
  server_.sendContent(F("<!doctype html><html><meta charset='utf-8'><body><h1>Arquivos</h1><ul>"));
  streamDirectory(config_.root, 0);
  server_.sendContent(F("</ul><p><a href='/'>Voltar</a></p></body></html>"));
  server_.sendContent("");

  giveSd();
}

// [7] DOWNLOAD
// ------------
// O uso do WebServer sincronico e intencional: streamFile() termina antes de
// retornarmos desta funcao. Portanto, conseguimos manter o mutex do SD travado
// durante todo o envio, algo que nao e garantido por uma resposta assincrona.
void SdFileServer::handleDownload() {
  if (!server_.hasArg("file")) {
    server_.send(400, "text/plain", "Missing file parameter");
    return;
  }

  const String path = server_.arg("file");
  if (!safePath(path)) {
    server_.send(400, "text/plain", "Invalid path");
    return;
  }

  if (!takeSd()) {
    server_.send(503, "text/plain", "SD busy");
    return;
  }

  File file = SD.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    if (file) file.close();
    giveSd();
    server_.send(404, "text/plain", "File not found");
    return;
  }

  const String disposition = "attachment; filename=\"" + baseName(path) + "\"";
  server_.sendHeader("Content-Disposition", disposition);
  server_.streamFile(file, contentTypeFor(path));
  file.close();

  giveSd();
}

// [8] INICIALIZACAO
// -----------------
bool SdFileServer::begin(SemaphoreHandle_t shared_sd_mutex) {
  if (started_) return true;
  if (SD.cardType() == CARD_NONE) return false;

  if (shared_sd_mutex) {
    sd_mutex_ = shared_sd_mutex;
    owns_mutex_ = false;
  } else {
    sd_mutex_ = xSemaphoreCreateMutex();
    owns_mutex_ = true;
  }
  if (!sd_mutex_) return false;

  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/health", HTTP_GET, [this]() { handleHealth(); });
  server_.on("/files", HTTP_GET, [this]() { handleFiles(); });
  server_.on("/download", HTTP_GET, [this]() { handleDownload(); });
  server_.onNotFound([this]() { server_.send(404, "text/plain", "Not found"); });

  server_.begin();
  started_ = true;
  return true;
}

void SdFileServer::handleClient() {
  if (started_) server_.handleClient();
}
