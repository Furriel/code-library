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
    unsigned long s  = ms / 1000;
    unsigned long m  = s / 60;
    unsigned long h  = m / 60;

    char buf[32];
    sprintf(buf, "T+%02luh%02lum%02lus", h, m % 60, s % 60);
    return String(buf);
}

void loggerInit() {
    Serial.print("Inicializando SD...");
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("Falha SD.");
        while (true) {
            delay(1000);
        }
    }
    Serial.println("✅ SD OK.");

    // Se arquivo já existe e tem conteúdo, assumimos cabeçalho já escrito
    if (SD.exists(CSV_FILE_PATH)) {
        File f = SD.open(CSV_FILE_PATH, FILE_READ);
        if (f && f.size() > 0) {
            headerWritten = true;
            Serial.println("ℹ️ CSV existente detectado. Mantendo cabeçalho atual.");
        }
        if (f) f.close();
    }
}

void processMessage(const String &client_id,
                    const String &topic,
                    const String &payload) {
    // Tenta interpretar JSON
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.print("SON inválido, ignorando para CSV: ");
        Serial.println(err.c_str());
        return;
    }

    // Achata JSON em arrays locais
    String keysLocal[MAX_KEYS];
    String valuesLocal[MAX_KEYS];
    size_t localCount = 0;

    flattenToArrays(doc.as<JsonVariantConst>(), "", keysLocal, valuesLocal, localCount, MAX_KEYS);

    if (localCount == 0) {
        Serial.println("Nenhum campo extraído do JSON.");
        return;
    }

    String ts = getTimestamp();

    // Cria cabeçalho na 1ª mensagem válida, se ainda não existir
    if (!headerWritten) {
        headerCount = (localCount > MAX_KEYS) ? MAX_KEYS : localCount;
        for (size_t i = 0; i < headerCount; i++) {
            headerKeys[i] = keysLocal[i];
        }

        File f = SD.open(CSV_FILE_PATH, FILE_WRITE);
        if (!f) {
            Serial.println("Erro ao abrir CSV para cabeçalho.");
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
        Serial.println("Cabeçalho CSV criado.");
    }

    // Abre para append da linha
    File f = SD.open(CSV_FILE_PATH, FILE_APPEND);
    if (!f) {
        Serial.println("Erro ao abrir CSV para escrita.");
        return;
    }

    // timestamp,client_id,topic
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
    }

    f.println();
    f.close();

    Serial.println("Linha registrada no CSV.");
}
