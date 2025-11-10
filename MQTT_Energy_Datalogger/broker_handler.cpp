/*
================================================================================
DATALOGGER ANALISADOR DE ENERGIA MQTT - BROKER HANDLER (IMPLEMENTAÇÃO)
================================================================================

Arquitetura:
------------
- Broker MQTT:
    Implementado com EmbeddedMqttBroker, executando diretamente na ESP32
    na porta definida em config.h.

- Cliente interno (esp32_logger):
    Implementado com PubSubClient.
    Conecta ao broker local (IP do AP da própria ESP32).
    Inscrito no tópico "#", ouve todas as mensagens publicadas no sistema.

Fluxo:
------
1. brokerInit():
   - Inicia o broker MQTT embarcado.
   - Configura o cliente interno para usar o IP do AP da ESP32.
   - Define o callback mqttCallback(), que:
        - Converte o payload em String.
        - Encaminha para processMessage("esp32_logger", topic, payload).

2. brokerLoop():
   - Garante que o cliente interno esteja conectado.
   - Chama mqttClient.loop() para processar as mensagens.

Resultado:
----------
Todas as publicações de qualquer cliente conectado ao broker local são
capturadas pelo cliente interno e registradas em CSV pelo módulo logger,
viabilizando o uso da ESP32 como um "Datalogger Analisador de Energia MQTT"
autônomo.

================================================================================
*/

#include <Arduino.h>
#include <WiFi.h>
#include <EmbeddedMqttBroker.h>
#include <PubSubClient.h>

#include "config.h"
#include "logger.h"

using namespace mqttBrokerName;

// Broker MQTT rodando na ESP32
static MqttBroker broker(MQTT_BROKER_PORT);

// Cliente interno que vai se conectar ao broker da própria ESP32
static WiFiClient espClient;
static PubSubClient mqttClient(espClient);

// Callback: toda mensagem publicada em qualquer tópico que o cliente interno
// estiver inscrito (#) cai aqui. É aqui que chamamos o logger/CSV.
static void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String t = String(topic);
    String p;
    p.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        p += (char)payload[i];
    }

    // client_id fixo para o logger interno
    processMessage("esp32_logger", t, p);
}

void brokerInit() {
    Serial.println("Iniciando broker EmbeddedMqttBroker...");

    broker.setMaxNumClients(8);     // ajuste conforme necessidade
    broker.startBroker();

    Serial.print("Broker na porta ");
    Serial.println(MQTT_BROKER_PORT);

    // Cliente interno conectando no próprio broker (IP do AP da ESP32)
    IPAddress brokerIP = WiFi.softAPIP();
    mqttClient.setServer(brokerIP, MQTT_BROKER_PORT);
    mqttClient.setCallback(mqttCallback);
}

static void ensureMqttConnected() {
    if (!mqttClient.connected()) {
        String clientId = "esp32_logger";
        if (mqttClient.connect(clientId.c_str())) {
            mqttClient.subscribe("#");  // escuta TODOS os tópicos
            Serial.println("Logger interno conectado ao broker e inscrito em '#'");
        }
    }
}

void brokerLoop() {
    // EmbeddedMqttBroker roda em tasks internas; aqui só garantimos o cliente logger.
    ensureMqttConnected();
    mqttClient.loop();
}
