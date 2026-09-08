#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>

#include "bridge_config.h"

// SERIAL <-> ESP-NOW BRIDGE
// -----------------------------------------------------------------------------
// Objetivo: transportar uma linha JSON recebida pela Serial para outro ESP32
// usando ESP-NOW e fazer o caminho inverso sem interpretar os dados da aplicacao.
//
// FLUXO:
//   Serial -> processSerial() -> processLine() -> sendJson() -> ESP-NOW
//   ESP-NOW -> onEspNowReceive() -> Serial.println()
//
// LEIA NESTA ORDEM:
//   [1] BridgePacket        -> estrutura enviada pelo radio
//   [2] variaveis globais   -> contadores e estado
//   [3] funcoes auxiliares  -> JSON, erros e peer
//   [4] sendJson            -> envio ESP-NOW
//   [5] callbacks ESP-NOW   -> confirmacao e recepcao
//   [6] processLine         -> interpreta STATUS, PEER ou JSON
//   [7] processSerial       -> monta uma linha recebida pela UART
//   [8] setupEspNow         -> inicializa o radio
//   [9] setup/loop          -> ciclo principal do firmware
//
// Dica: para mudar baudrate, heartbeat ou tamanho maximo do JSON, abra primeiro
// include/bridge_config.h. Nao e necessario procurar essas constantes aqui.

// [1] PACOTE TRANSPORTADO PELO ESP-NOW
// O JSON recebe um pequeno cabecalho antes de ir ao radio.
//
// magic    -> identifica que o frame pertence a esta feature.
// version  -> permite detectar uma versao de pacote incompatível.
// sequence -> contador crescente de transmissao, util para diagnostico.
// length   -> informa quantos bytes do array payload realmente sao usados.
// payload  -> texto JSON terminado em '\0'.
struct BridgePacket {
  uint16_t magic;
  uint8_t version;
  uint8_t reserved;
  uint32_t sequence;
  uint16_t length;
  char payload[BRIDGE_MAX_JSON_BYTES];
};

// [2] ESTADO E CONTADORES
// Todas estas variaveis pertencem ao bridge inteiro, por isso ficam fora das
// funcoes. Os contadores aparecem no comando STATUS.
static uint32_t txSequence = 0;
static uint32_t txFrames = 0;
static uint32_t rxFrames = 0;
static uint32_t txErrors = 0;
static uint32_t rxErrors = 0;
static uint32_t lastHeartbeatMs = 0;
static String serialBuffer;

// Endereco do ESP32 remoto. Broadcast e usado inicialmente para facilitar o
// primeiro teste. O comando "PEER AA:BB:CC:DD:EE:FF" troca esse endereco.
static uint8_t peerAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// [3] FUNCOES AUXILIARES

// Imprime um JsonDocument em uma unica linha. Uma mensagem por linha facilita
// leitura humana, logs e integracao com programas que leem a porta Serial.
static void printJson(const JsonDocument &doc) {
  serializeJson(doc, Serial);
  Serial.println();
}

// Padroniza mensagens de erro geradas pelo proprio bridge.
static void printError(const char *code, const char *message) {
  StaticJsonDocument<192> doc;
  doc["type"] = "bridge_error";
  doc["role"] = BRIDGE_ROLE_NAME;
  doc["code"] = code;
  doc["message"] = message;
  printJson(doc);
}

// Verificacao rapida antes do parse completo com ArduinoJson.
// Ela apenas confirma que o texto parece ser um objeto JSON: {...}.
static bool isJsonObject(const String &text) {
  String value = text;
  value.trim();
  return value.startsWith("{") && value.endsWith("}");
}

// Registra peerAddress no ESP-NOW. Se o peer ja existe, nao faz nada.
static bool addPeer() {
  if (esp_now_is_peer_exist(peerAddress)) return true;

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, peerAddress, sizeof(peerAddress));
  peer.channel = 0;
  peer.encrypt = false;

  return esp_now_add_peer(&peer) == ESP_OK;
}

// [4] ENVIO SERIAL -> ESP-NOW
// Recebe uma string JSON ja validada, coloca o cabecalho BridgePacket e entrega
// o pacote ao driver ESP-NOW.
//
// Retorna true quando esp_now_send aceitou o envio e false em erro imediato.
// A confirmacao final do radio chega depois em onEspNowSend().
static bool sendJson(const char *json) {
  const size_t length = strnlen(json, BRIDGE_MAX_JSON_BYTES + 1);

  // O payload precisa caber no array e ainda deixar espaco para '\0'.
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

// [5] CALLBACKS DO ESP-NOW

// Chamado pelo framework depois da tentativa de transmissao pelo radio.
// Um erro aqui significa que o envio havia sido aceito inicialmente, mas nao
// terminou com sucesso na camada ESP-NOW.
static void onEspNowSend(const uint8_t *macAddress, esp_now_send_status_t status) {
  (void)macAddress;
  if (status != ESP_NOW_SEND_SUCCESS) txErrors++;
}

// Chamado automaticamente quando um frame ESP-NOW chega.
// Esta funcao valida o cabecalho e imprime somente o JSON na Serial.
static void onEspNowReceive(const uint8_t *macAddress, const uint8_t *data, int dataLength) {
  (void)macAddress;

  // Esta implementacao usa tamanho fixo de BridgePacket. Qualquer outro frame
  // e ignorado para evitar interpretar bytes de outro protocolo como JSON.
  if (dataLength != static_cast<int>(sizeof(BridgePacket))) {
    rxErrors++;
    return;
  }

  BridgePacket packet{};
  memcpy(&packet, data, sizeof(packet));

  // Confere assinatura, versao e comprimento antes de tocar no payload.
  if (packet.magic != BRIDGE_PACKET_MAGIC ||
      packet.version != BRIDGE_PACKET_VERSION ||
      packet.length == 0 ||
      packet.length >= BRIDGE_MAX_JSON_BYTES) {
    rxErrors++;
    return;
  }

  // Garante terminacao da string antes de usa-la como texto C.
  packet.payload[packet.length] = '\0';

  if (!isJsonObject(String(packet.payload))) {
    rxErrors++;
    return;
  }

  // O bridge e transparente: o JSON recebido sai pela Serial sem alterar os
  // campos da aplicacao.
  Serial.println(packet.payload);
  rxFrames++;
}

// Converte texto "AA:BB:CC:DD:EE:FF" para os seis bytes usados pelo ESP-NOW.
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

// Gera o JSON retornado pelo comando STATUS.
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

// Envia periodicamente uma mensagem simples para indicar que o bridge continua
// executando. O intervalo e definido em BRIDGE_HEARTBEAT_MS.
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

// [6] INTERPRETACAO DE UMA LINHA DA SERIAL
// Tudo que chega terminado por '\n' passa por esta funcao.
// Existem somente tres possibilidades:
//   STATUS                    -> mostra diagnostico local
//   PEER AA:BB:CC:DD:EE:FF   -> altera o destino ESP-NOW
//   {...}                     -> envia o JSON pelo ESP-NOW
static void processLine(String line) {
  line.trim();
  if (line.isEmpty()) return;

  // Comando local 1: diagnostico.
  if (line.equalsIgnoreCase("STATUS")) {
    printStatus();
    return;
  }

  // Comando local 2: configuracao do peer.
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

  // Se nao for comando local, esperamos um objeto JSON.
  if (!isJsonObject(line)) {
    printError("INPUT", "send JSON, STATUS or PEER command");
    return;
  }

  // ArduinoJson e usado somente para confirmar a sintaxe. O conteudo nao e
  // interpretado para manter a feature independente da aplicacao.
  StaticJsonDocument<BRIDGE_MAX_JSON_BYTES> doc;
  if (deserializeJson(doc, line)) {
    printError("JSON", "invalid JSON");
    return;
  }

  sendJson(line.c_str());
}

// [7] LEITURA DA SERIAL
// A UART entrega caracteres individualmente. Esta funcao acumula os caracteres
// em serialBuffer ate encontrar '\n'; somente entao chama processLine().
static void processSerial() {
  while (Serial.available() > 0) {
    const char ch = static_cast<char>(Serial.read());

    // Ignora '\r' para aceitar tanto linhas Windows (\r\n) quanto Unix (\n).
    if (ch == '\r') continue;

    // Fim da linha: processa e limpa o buffer para a proxima mensagem.
    if (ch == '\n') {
      processLine(serialBuffer);
      serialBuffer = "";
      continue;
    }

    // O limite extra evita crescimento indefinido se o emissor nunca mandar
    // uma quebra de linha.
    if (serialBuffer.length() < BRIDGE_MAX_JSON_BYTES + 32) {
      serialBuffer += ch;
    } else {
      serialBuffer = "";
      printError("SERIAL_OVERFLOW", "serial line is too long");
    }
  }
}

// [8] INICIALIZACAO DO ESP-NOW
static void setupEspNow() {
  // ESP-NOW usa o radio Wi-Fi. O modo station e suficiente e nao exige conexao
  // com roteador.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    printError("ESPNOW_INIT", "esp_now_init failed");
    return;
  }

  // Registra o peer inicial, que por padrao e broadcast.
  if (!addPeer()) {
    printError("PEER_ADD", "could not add default peer");
  }

  // Entrega ao framework as funcoes que tratam envio concluido e recepcao.
  esp_now_register_send_cb(onEspNowSend);
  esp_now_register_recv_cb(onEspNowReceive);
}

// [9] CICLO PRINCIPAL DO ARDUINO
void setup() {
  Serial.begin(BRIDGE_BAUDRATE);
  delay(300);

  setupEspNow();

  // Mensagem de boot: ajuda a confirmar baudrate, papel do firmware e que o
  // setup terminou.
  StaticJsonDocument<160> doc;
  doc["type"] = "bridge_boot";
  doc["role"] = BRIDGE_ROLE_NAME;
  doc["baudrate"] = BRIDGE_BAUDRATE;
  printJson(doc);
}

void loop() {
  // 1. Processa qualquer dado pendente na Serial.
  processSerial();

  // 2. Verifica se chegou o momento de enviar o heartbeat.
  const uint32_t now = millis();
  if (now - lastHeartbeatMs >= BRIDGE_HEARTBEAT_MS) {
    lastHeartbeatMs = now;
    sendHeartbeat();
  }

  // Pequena pausa para nao ocupar 100% da CPU em polling continuo.
  delay(2);
}
