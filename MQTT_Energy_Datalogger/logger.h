/*
================================================================================
DATALOGGER ANALISADOR DE ENERGIA MQTT - LOGGER (HEADER)
================================================================================

Responsabilidade:
-----------------
Definir a interface do módulo de registro em CSV no cartão SD.

Funções:
--------
- loggerInit():
    Inicializa o cartão SD, verifica a existência do arquivo CSV e define
    se o cabeçalho já foi escrito.

- processMessage(client_id, topic, payload):
    Recebe mensagens do broker (via cliente interno), interpreta o payload JSON,
    gera o cabeçalho (na primeira mensagem válida) e grava linhas no CSV com:
        timestamp_relativo, client_id, topic, colunas de dados.

================================================================================
*/
#pragma once
#include <Arduino.h>

// Inicializa SD e estado do logger.
// Não recria cabeçalho se o arquivo já existir.
void loggerInit();

// Processa uma mensagem recebida do broker:
// - payload JSON
// - gera/corrige cabeçalho (na 1ª mensagem)
// - grava linha: timestamp, client_id, topic, colunas
void processMessage(const String &client_id,
                    const String &topic,
                    const String &payload);
