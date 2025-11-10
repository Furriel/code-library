/*
================================================================================
DATALOGGER ANALISADOR DE ENERGIA MQTT - ESP32
================================================================================

    DESCRIÇÃO GERAL:
Este firmware transforma a ESP32 em um datalogger autônomo de energia baseado em
MQTT, capaz de atuar simultaneamente como **broker MQTT embarcado** e **logger CSV**
em cartão SD.

As mensagens publicadas por dispositivos conectados (ex.: medidores MiEnergy) são
interpretadas, “achatadas” (flatten) e registradas em um arquivo CSV, com cada
campo do JSON convertido em uma coluna distinta. O cabeçalho é criado
automaticamente a partir da primeira mensagem JSON válida recebida.

--------------------------------------------------------------------------------
     DEPENDÊNCIAS (instalar na Arduino IDE)
--------------------------------------------------------------------------------
  1 - **EmbeddedMqttBroker**
    - Biblioteca responsável por rodar o broker MQTT diretamente na ESP32.
    - Disponível em: https://github.com/alexCajas/EmbeddedMqttBroker

  2 -  **PubSubClient**
    - Utilizado como cliente MQTT interno que se conecta ao broker local e
      assina o tópico "#" (todos os tópicos), repassando as mensagens ao logger.

  3 -  **ArduinoJson (v6.x)**
    - Necessário para o parser e “achatamento” (flatten) do JSON recebido.

  4 - **SD** (biblioteca nativa da ESP32)
    - Responsável pela gravação dos dados em `/energy_log.csv`.

  5 - **WiFi** (nativa da ESP32)
    - Usada para criar a rede Access Point dedicada do sistema.

--------------------------------------------------------------------------------
   FUNCIONAMENTO
--------------------------------------------------------------------------------
O sistema realiza as seguintes etapas:

1. Cria um Access Point (AP) dedicado:
   - SSID: **MQTT_Energy_LOGGER**
   - Senha: **12345678**
   - IP exibido no Serial (ex.: 192.168.4.1)

2. Inicia o broker MQTT local na porta **1883** usando `EmbeddedMqttBroker`.

3. Inicializa um cliente MQTT interno (`PubSubClient`) conectado ao broker local:
   - Assina o tópico "#" para capturar todas as publicações.
   - Encaminha cada mensagem JSON recebida ao módulo `logger`.

4. O logger:
   - Interpreta o payload JSON.
   - “Achata” estruturas do tipo `{"campo":{"value":123}}` → `campo = 123`.
   - Na primeira mensagem válida, cria o cabeçalho CSV automaticamente.
   - Em cada nova mensagem, adiciona uma linha com:
       `timestamp_relativo, client_id, topic, colunas extraídas`.

5. O arquivo final é salvo como:
   - *./energy_log.csv** no cartão SD.

--------------------------------------------------------------------------------
   COMO USAR NA PRÁTICA
--------------------------------------------------------------------------------
  1 - Compile e envie o código para a ESP32.

  2 - Abra o Monitor Serial — será exibido o IP do Access Point criado
    (exemplo: 192.168.4.1).

  3 - Configure o dispositivo publicador (ex.: MiEnergy) para:
   - Conectar ao WiFi **MQTT_Energy_LOGGER***
   - Senha: **sige1234**
   - Broker MQTT: **IP mostrado no Serial** (ex.: 192.168.4.1)
   - Porta: **1883**
   - Publicar os dados em formato JSON no tópico desejado (ex.: "MiEnergy_01")

  4 -  Cada publicação será registrada automaticamente no arquivo CSV,
    já com colunas separadas.

--------------------------------------------------------------------------------
   OBSERVAÇÕES
--------------------------------------------------------------------------------
- O timestamp atual é relativo ao tempo de execução (T+hhmmss).
  Pode ser substituído por NTP/RTC conforme necessidade.
- Para operação em rede existente (modo STA), substitua o módulo `wifi_ap`
  por uma configuração de WiFi Station.
- O cabeçalho é dinâmico e gerado com base na primeira mensagem JSON.
  Caso o formato de dados seja fixo, é possível travar um layout padrão no logger.
- O projeto segue estrutura modular para facilitar manutenção e extensão:
  `wifi_ap.*`, `broker_handler.*`, `logger.*`, `json_flatten.*`, `config.h`.

================================================================================
*/
