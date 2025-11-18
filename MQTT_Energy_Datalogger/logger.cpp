/*
================================================================================
DATALOGGER ANALISADOR DE ENERGIA MQTT - LOGGER (IMPLEMENTAÇÃO)
================================================================================

Responsabilidade:
-----------------
Executar toda a lógica de registro dos dados em CSV:

1. Inicialização (loggerInit):
   - Monta o SD.
   - Detecta se o arquivo CSV já existe e possui conteúdo.
   - Em caso afirmativo, assume que o cabeçalho já foi criado.

2. Processamento de mensagens (processMessage):
   - Tenta desserializar o payload como JSON.
   - Usa o módulo json_flatten para extrair pares chave/valor.
   - Na primeira mensagem válida:
       - Gera o cabeçalho automático: "timestamp,client_id,topic,<chaves JSON>".
   - Para cada mensagem:
       - Gera timestamp relativo ao boot (T+hhmmss).
       - Grava uma linha com os valores alinhados ao cabeçalho.

Observações:
------------
- Focado em robustez: mensagens inválidas são ignoradas sem travar o sistema.
- Arquivo: definido em config.h (CSV_FILE_PATH).

================================================================================
*/

#include "logger.h"
#include "config.h"
#include "json_flatten.h"

#include <SD.h>
#include <ArduinoJson.h>

static bool headerWritten = false;
static String headerKeys[MAX_KEYS];
static size_t headerCount = 0;

// Timestamp simples relativo ao boot (T+hhmmss)
static String getTimestamp() {
  unsigned long ms = millis();
  unsigned long s = ms / 1000;
  unsigned long m = s / 60;
  unsigned long h = m / 60;

  char buf[32];
  sprintf(buf, "T+%02luh%02lum%02lus", h, m % 60, s % 60);
  return String(buf);
}

void loggerInit() {
  Serial.println();
  Serial.println("==== loggerInit() ====");
  Serial.print("Inicializando SD...");

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Falha SD. Verifique fiação / CS / formatação.");
    while (true) {
      delay(1000);
    }
  }
  Serial.println("SD OK.");

  Serial.print("Verificando existência do arquivo CSV: ");
  Serial.println(CSV_FILE_PATH);

  // Se arquivo já existe e tem conteúdo, assumimos cabeçalho já escrito
  if (SD.exists(CSV_FILE_PATH)) {
    File f = SD.open(CSV_FILE_PATH, FILE_READ);
    if (f) {
      uint32_t size = f.size();
      Serial.print("Arquivo encontrado. Tamanho: ");
      Serial.print(size);
      Serial.println(" bytes.");

      if (size > 0) {
        headerWritten = true;
        Serial.println("Cabeçalho presumido como já existente.");
      } else {
        Serial.println("Arquivo existe, mas está vazio. Cabeçalho será criado na primeira mensagem válida.");
      }
      f.close();
    } else {
      Serial.println("Arquivo existe, mas não foi possível abrir para leitura.");
    }
  } else {
    Serial.println("Arquivo CSV ainda não existe. Será criado na primeira mensagem válida.");
  }

  Serial.println("==== Fim loggerInit() ====");
}


void processMessage(const String &client_id,
                    const String &topic,
                    const String &payload) {
  Serial.println();
  Serial.println("========== processMessage() ==========");

  Serial.print("client_id: ");
  Serial.println(client_id);
  Serial.print("topic: ");
  Serial.println(topic);
  Serial.print("payload bruto: ");
  Serial.println(payload);

  // Tenta interpretar JSON
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
DeserializationError err = deserializeJson(doc, payload);
if (err) {
    Serial.print("JSON inválido, ignorando para CSV: ");
    Serial.println(err.c_str());
    Serial.println("======================================");
    return;
}

  Serial.println("JSON desserializado com sucesso.");

  // Achata JSON em arrays locais
  String keysLocal[MAX_KEYS];
  String valuesLocal[MAX_KEYS];
  size_t localCount = 0;

  flattenToArrays(doc.as<JsonVariantConst>(), "", keysLocal, valuesLocal, localCount, MAX_KEYS);

  Serial.print("Total de campos extraídos do JSON: ");
  Serial.println(localCount);

  if (localCount == 0) {
    Serial.println("Nenhum campo extraído do JSON. Nada será gravado.");
    Serial.println("======================================");
    return;
  }

  // ============================================================
  // MODO DESCOBERTA: só imprime estrutura e NÃO grava no SD
  // ============================================================
#if DISCOVERY_MODE
  Serial.println();
  Serial.println("=== MODO DE DESCOBERTA ATIVO (DISCOVERY_MODE = 1) ===");
  Serial.println("Estrutura detectada do JSON (chave = exemplo de valor):");

  for (size_t i = 0; i < localCount; i++) {
    Serial.print("  - ");
    Serial.print(keysLocal[i]);
    Serial.print(" = ");
    Serial.println(valuesLocal[i]);
  }

  Serial.println("Nenhum dado foi gravado no SD.");
  Serial.println("Para habilitar o log em CSV, ajuste DISCOVERY_MODE para 0 em config.h.");
  Serial.println("======================================");
  return;
#endif

  String ts = getTimestamp();
  Serial.print("Timestamp gerado: ");
  Serial.println(ts);


  // Cria cabeçalho na 1ª mensagem válida, se ainda não existir
  if (!headerWritten) {
    headerCount = (localCount > MAX_KEYS) ? MAX_KEYS : localCount;

    Serial.print("Criando cabeçalho CSV com ");
    Serial.print(headerCount);
    Serial.println(" campos:");

    for (size_t i = 0; i < headerCount; i++) {
      headerKeys[i] = keysLocal[i];
      Serial.print("  [");
      Serial.print(i);
      Serial.print("] ");
      Serial.println(headerKeys[i]);
    }

    File f = SD.open(CSV_FILE_PATH, FILE_WRITE);
    if (!f) {
      Serial.print("Erro ao abrir CSV para cabeçalho: ");
      Serial.println(CSV_FILE_PATH);
      Serial.println("======================================");
      return;
    }

    // Cabeçalho: timestamp,client_id,topic,campos...
    f.print("timestamp,client_id,topic");
    for (size_t i = 0; i < headerCount; i++) {
      f.print(",");
      f.print(headerKeys[i]);
    }
    f.println();
    f.close();

    headerWritten = true;
    Serial.println("Cabeçalho CSV criado com sucesso.");
  }

  // Abre arquivo para append da linha
  File f = SD.open(CSV_FILE_PATH, FILE_WRITE);
  if (!f) {
    Serial.print("Erro ao abrir CSV para escrita: ");
    Serial.println(CSV_FILE_PATH);
    Serial.println("======================================");
    return;
  }

  Serial.print("Gravando linha no CSV: ");
  Serial.println(CSV_FILE_PATH);

  // timestamp,client_id,topic,...
  f.print(ts);
  f.print(",");
  f.print(client_id);
  f.print(",");
  f.print(topic);

  // Colunas em ordem fixa do cabeçalho
  for (size_t i = 0; i < headerCount; i++) {
    f.print(",");
    String v = findValueForKey(headerKeys[i], keysLocal, valuesLocal, localCount);
    f.print(v);

    Serial.print("  Campo '");
    Serial.print(headerKeys[i]);
    Serial.print("' = '");
    Serial.print(v);
    Serial.println("'");
  }

  f.println();
  f.close();

  Serial.println("Linha registrada no CSV com sucesso.");
  Serial.println("======================================");
}
