/*
================================================================================
DATALOGGER ANALISADOR DE ENERGIA MQTT - WIFI AP (IMPLEMENTAÇÃO)
================================================================================

Responsabilidade:
-----------------
Configura a ESP32 em modo Access Point com os parâmetros definidos em config.h.

Comportamento:
--------------
- Inicia o AP SIGE_MQTT_LOGGER com a senha especificada.
- Em caso de falha, mantém o sistema em loop de erro.
- Em caso de sucesso, exibe via Serial:
    - SSID ativo.
    - IP da ESP32 (endereço do broker MQTT).

================================================================================
*/

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

void setupAccessPoint() {
    Serial.println("Iniciando Access Point...");

    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(AP_SSID, AP_PASS)) {
        Serial.println("Falha ao iniciar AP.");
        while (true) {
            delay(1000);
        }
    }

    Serial.print("AP ativo. SSID: ");
    Serial.println(AP_SSID);

    Serial.print("IP do broker (ESP32): ");
    Serial.println(WiFi.softAPIP());
}
