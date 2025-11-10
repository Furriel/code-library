/*
================================================================================
DATALOGGER ANALISADOR DE ENERGIA MQTT - WIFI AP (HEADER)
================================================================================

Responsabilidade:
-----------------
Declara a função de inicialização do Access Point WiFi da ESP32, que será
utilizado como rede dedicada para o broker MQTT embarcado.

Função exposta:
---------------
- setupAccessPoint(): configura o modo AP, inicia a rede SIGE_MQTT_LOGGER e
  imprime no Serial o IP da ESP32, que será usado como endereço do broker.

================================================================================
*/
#pragma once

#pragma once

void setupAccessPoint();
