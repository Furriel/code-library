/*
================================================================================
DATALOGGER ANALISADOR DE ENERGIA MQTT - MAIN
================================================================================

Responsabilidade:
-----------------
Função principal do sistema. Realiza a sequência de inicialização dos módulos:

1. setupAccessPoint()  → Cria o Access Point da ESP32 (rede SIGE_MQTT_LOGGER).
2. loggerInit()        → Inicializa o cartão SD e o arquivo CSV.
3. brokerInit()        → Inicia o broker MQTT embarcado (EmbeddedMqttBroker) 
                         e o cliente interno de logging (PubSubClient).
4. loop()              → Mantém o cliente interno conectado e processando 
                         mensagens recebidas para registro no CSV.

Fluxo de execução:
------------------
- Após a inicialização, o sistema opera de forma autônoma:
    - Recebe publicações MQTT de dispositivos conectados.
    - Processa os payloads JSON.
    - Registra os dados em /energy_log.csv com colunas separadas.

================================================================================
*/

#include <Arduino.h>
#include "wifi_ap.h"
#include "logger.h"
#include "broker_handler.h"

void setup() {
    Serial.begin(115200);
    delay(1000);

    setupAccessPoint();  // Cria o AP e mostra IP do broker
    loggerInit();        // Inicializa SD / CSV
    brokerInit();        // Sobe o broker MQTT interno + cliente logger
}

void loop() {
    brokerLoop();        // Mantém o cliente interno escutando e logando
}
