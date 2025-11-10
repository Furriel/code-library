/*
================================================================================
DATALOGGER ANALISADOR DE ENERGIA MQTT - BROKER HANDLER (HEADER)
================================================================================

Responsabilidade:
-----------------
Abstrair a inicialização e o loop de operação do broker MQTT embarcado e do
cliente interno de logging.

Funções:
--------
- brokerInit():
    Inicia o broker MQTT (EmbeddedMqttBroker) na porta configurada e configura
    o cliente MQTT interno (PubSubClient) apontando para o broker local.

- brokerLoop():
    Deve ser chamado no loop() principal para manter o cliente interno ativo,
    garantindo o recebimento das publicações para o logger.

================================================================================
*/
#pragma once

void brokerInit();
void brokerLoop();
