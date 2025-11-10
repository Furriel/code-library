/*
================================================================================
DATALOGGER ANALISADOR DE ENERGIA MQTT - CONFIGURAÇÕES GERAIS
================================================================================

Responsabilidade:
-----------------
Centraliza todas as constantes de configuração do sistema, incluindo:

- Parâmetros do Access Point WiFi (SSID e senha).
- Pino CS do cartão SD e caminho do arquivo CSV.
- Porta do broker MQTT embarcado.
- Limites de buffer e quantidade máxima de chaves para o parser JSON.

Observação:
-----------
Ajuste estes valores conforme o hardware e a aplicação (ex.: pino do SD,
nome da rede, limites de memória para JSON).

==============================================================================
*/

#pragma once

// ------------------------------
// Configurações gerais do sistema
// ------------------------------

// WiFi Access Point
#define AP_SSID        "MQTT_Energy_LOGGER*"
#define AP_PASS        "12345678"      // mínimo 8 caracteres

// SD Card
#define SD_CS_PIN      5
#define CSV_FILE_PATH  "/energy_log.csv"

// MQTT Broker
#define MQTT_BROKER_PORT 1883

// JSON / CSV
#define MAX_KEYS          80           // Máximo de colunas extraídas
#define JSON_BUFFER_SIZE  4096         // Ajuste conforme tamanho típico do payload
