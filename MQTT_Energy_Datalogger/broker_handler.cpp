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

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.println();
    Serial.println("========== MQTT CALLBACK ==========");

    Serial.print("Tópico recebido: ");
    Serial.println(topic);
    Serial.print("Tamanho do payload (bytes): ");
    Serial.println(length);

    String t = String(topic);

    // Filtro de tópico opcional
    if (String(TOPIC_FILTER).length() > 0) {
        if (!t.startsWith(String(TOPIC_FILTER))) {
            Serial.print("Tópico ignorado pelo filtro TOPIC_FILTER = \"");
            Serial.print(TOPIC_FILTER);
            Serial.println("\"");
            Serial.println("========== FIM MQTT CALLBACK (IGNORADO) ==========");
            return;
        }
    }

    String p;
    p.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        p += (char)payload[i];
    }

    Serial.print("Payload (string): ");
    Serial.println(p);

    Serial.println("Encaminhando para processMessage(\"esp32_logger\", topic, payload)...");
    processMessage("esp32_logger", t, p);

    Serial.println("========== FIM MQTT CALLBACK ==========");
}


void brokerInit() {
    Serial.println();
    Serial.println("==== brokerInit() ====");
    Serial.println("Iniciando broker EmbeddedMqttBroker...");

    broker.setMaxNumClients(8);
    broker.startBroker();

    Serial.print("Broker iniciado na porta ");
    Serial.println(MQTT_BROKER_PORT);

    // Só para debug: IP do AP para os dispositivos externos
    IPAddress apIP = WiFi.softAPIP();
    Serial.print("IP do broker para clientes remotos (AP da ESP32): ");
    Serial.println(apIP);

    // *** Cliente interno usa loopback ***
    IPAddress loopback(127, 0, 0, 1);
    mqttClient.setServer(loopback, MQTT_BROKER_PORT);
    mqttClient.setCallback(mqttCallback);

    Serial.print("Cliente interno logger apontando para ");
    Serial.println(loopback); // deve imprimir 127.0.0.1

    Serial.println("Configuração do cliente interno MQTT concluída.");
    Serial.println("=======================================");
}

static void ensureMqttConnected() {
    static bool testePublicado = false;  // <--- flag para não ficar publicando sempre

    if (!mqttClient.connected()) {
        Serial.println();
        Serial.println("---- Cliente logger MQTT desconectado. Tentando reconectar...");

        String clientId = "esp32_logger";
        Serial.print("Tentando conectar ao broker local como ");
        Serial.println(clientId);

        if (mqttClient.connect(clientId.c_str())) {
            Serial.print("Conectado ao broker como clientId = ");
            Serial.println(clientId);

            if (mqttClient.subscribe("#")) {
                Serial.println("Inscrição em '#' realizada com sucesso.");
            } else {
                Serial.println("Falha ao se inscrever em '#'.");
            }

        } else {
            Serial.print("Falha ao conectar cliente logger MQTT. state = ");
            Serial.println(mqttClient.state());
        }
        Serial.println("--------------------------------------");
    }
}


void brokerLoop() {
    ensureMqttConnected();
    mqttClient.loop();
}
