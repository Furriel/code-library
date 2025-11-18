#pragma once

// ------------------------------
// Configurações gerais do sistema
// ------------------------------

// WiFi Access Point
#define AP_SSID        "MQTT_MiEnergy_Esp"
#define AP_PASS        "1234567890"      // mínimo 8 caracteres

// SD Card
#define SD_CS_PIN      5
#define CSV_FILE_PATH  "/energy_log.csv"

// MQTT Broker
#define MQTT_BROKER_PORT 1883

// JSON / CSV
#define MAX_KEYS          200           // Máximo de colunas extraídas
#define JSON_BUFFER_SIZE  12288        // Ajuste conforme tamanho típico do payload

// ----------------------------------------------------
// Modo de descoberta da estrutura do JSON
// 1 = imprime chaves/valores no Serial e NÃO grava no SD
// 0 = grava normalmente no CSV
// ----------------------------------------------------
#define DISCOVERY_MODE 0

// ----------------------------------------------------
// Filtro de tópico (opcional)
// - String vazia ""        -> aceita todos os tópicos
// - Ex: "MiEnergy/"        -> só processa tópicos que
//                             comecem com "MiEnergy/"
// ----------------------------------------------------
#define TOPIC_FILTER ""
