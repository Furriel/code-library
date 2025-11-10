# Datalogger Analisador de Energia MQTT (ESP32)

Firmware modular para **ESP32** que atua como **broker MQTT embarcado** e **registrador de dados (datalogger)**, salvando automaticamente publicações JSON em um arquivo CSV com colunas separadas.

---

##  Funcionalidades principais

- Cria um **Access Point dedicado** (`MQTT_Energy_LOGGER / 12345678`)
- Executa o **broker MQTT local** (`EmbeddedMqttBroker`)
- Mantém um **cliente interno** (`PubSubClient`) inscrito no tópico `#`
- Converte payloads JSON em **colunas CSV**
- Armazena as mensagens em **/energy_log.csv** no cartão SD
- Cabeçalho gerado automaticamente na primeira mensagem válida

---

##  Estrutura modular

| Módulo | Função |
|--------|--------|
| `wifi_ap.*` | Cria o Access Point e exibe IP local |
| `broker_handler.*` | Inicia o broker MQTT e cliente interno |
| `logger.*` | Gerencia o SD e grava os dados CSV |
| `json_flatten.*` | “Achata” o JSON em pares chave/valor |
| `config.h` | Define parâmetros gerais |
| `main.cpp` | Ponto principal do firmware |

---

##  Dependências

Instale via Gerenciador de Bibliotecas (Arduino IDE):

- [EmbeddedMqttBroker](https://github.com/alexCajas/EmbeddedMqttBroker)
- PubSubClient
- ArduinoJson (v6.x)
- SD (nativa ESP32)
- WiFi (nativa ESP32)

---

##  Como usar

1. Compile e envie o código para a ESP32.
2. Verifique o IP no **Monitor Serial** (ex: `192.168.4.1`).
3. Configure seu dispositivo MQTT (ex.: MiEnergy) para:
   - Rede WiFi: `MQTT_Energy_LOGGER`
   - Senha: `12345678`
   - Broker: `192.168.1.1`
   - Porta: `1883`
   - Payload: JSON
4. Cada publicação é gravada em `/energy_log.csv` no SD com colunas automáticas.

---

© 2025 - Furriel, Geovanne 
