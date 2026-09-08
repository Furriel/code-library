#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/*
 * ESP32 SD FILE SERVER
 * ====================
 *
 * Objetivo:
 *   Expor arquivos existentes no microSD por uma interface HTTP local e
 *   somente leitura.
 *
 * LEIA NESTA ORDEM:
 *   [1] CONFIGURACAO       -> parametros que o usuario normalmente altera
 *   [2] CLASSE PRINCIPAL   -> API publica da feature
 *
 * O arquivo .cpp possui o fluxo detalhado das rotas e do download.
 */

// [1] CONFIGURACAO
// ----------------
// Strings sao copiadas para dentro da classe, entao o chamador pode montar a
// configuracao com valores temporarios sem se preocupar com ponteiros soltos.
struct SdFileServerConfig {
  uint16_t port = 80;
  String root = "/";
  String allowed_extension = "";  // vazio = aceita qualquer extensao
  String page_title = "ESP32 SD files";
  uint8_t max_depth = 4;
  uint32_t mutex_timeout_ms = 2000;
};

// [2] CLASSE PRINCIPAL
// -------------------
class SdFileServer {
 public:
  explicit SdFileServer(const SdFileServerConfig &config = SdFileServerConfig());

  // Registra as rotas e inicia o servidor.
  //
  // Pre-condicoes:
  //   - Wi-Fi ja conectado pela aplicacao;
  //   - SD ja montado pela aplicacao com SD.begin(...).
  //
  // shared_sd_mutex:
  //   Se outra tarefa usa o SD, passe o MESMO mutex aqui. Se nullptr, a classe
  //   cria um mutex proprio, suficiente apenas quando ela e a unica usuaria.
  bool begin(SemaphoreHandle_t shared_sd_mutex = nullptr);

  // Deve ser chamada repetidamente no loop ou em uma task dedicada.
  void handleClient();

  // Permite consultar o objeto WebServer para extensoes opcionais da aplicacao.
  WebServer &server() { return server_; }

 private:
  SdFileServerConfig config_;
  WebServer server_;
  SemaphoreHandle_t sd_mutex_ = nullptr;
  bool owns_mutex_ = false;
  bool started_ = false;

  bool takeSd();
  void giveSd();
  bool safePath(const String &path) const;
  String normalizeRoot(const String &root) const;
  String htmlEscape(const String &value) const;
  String urlEncode(const String &value) const;
  String contentTypeFor(const String &path) const;
  String baseName(const String &path) const;

  void handleRoot();
  void handleHealth();
  void handleFiles();
  void handleDownload();
  void streamDirectory(const String &path, uint8_t depth);
};
