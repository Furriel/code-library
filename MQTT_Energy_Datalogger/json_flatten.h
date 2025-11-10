/*
================================================================================
DATALOGGER ANALISADOR DE ENERGIA MQTT - JSON FLATTEN (HEADER)
================================================================================

Responsabilidade:
-----------------
Fornece funções utilitárias para converter (achatar) estruturas JSON arbitrárias
em pares chave/valor planos, adequados para registro em CSV.

Regras principais:
------------------
- Objetos do tipo {"value": X} são tratados como campos escalares:
    {"tensao_a":{"value":223.5}} -> chave "tensao_a", valor "223.5"
- Objetos aninhados e arrays são percorridos recursivamente, concatenando nomes
  com "_" para formar chaves únicas.
- Produz vetores paralelos:
    - keys[i] = nome da coluna
    - values[i] = valor convertido para string

Uso:
----
Chamado pelo módulo logger para extrair colunas a partir do payload JSON.

================================================================================
*/
#pragma once

#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// Achata o JSON em vetores (keys/values).
// Regra especial: {"value": X} -> chave = prefixo, valor = X.
void flattenToArrays(JsonVariantConst v,
                     const String &prefix,
                     String *keys,
                     String *values,
                     size_t &count,
                     size_t maxCount);

String findValueForKey(const String &key,
                       String *keys,
                       String *values,
                       size_t count);
