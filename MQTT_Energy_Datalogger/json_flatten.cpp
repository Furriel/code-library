/*
================================================================================
DATALOGGER ANALISADOR DE ENERGIA MQTT - JSON FLATTEN (IMPLEMENTAÇÃO)
================================================================================

Implementa:
-----------
- flattenToArrays():
    Percorre recursivamente o JSON (objetos, arrays e escalares),
    preenchendo vetores de chaves e valores para posterior gravação no CSV.

- findValueForKey():
    Busca o valor correspondente a uma chave dentro dos vetores gerados
    para montar cada linha conforme o cabeçalho fixo.

Regras principais:
------------------
- Objetos do tipo {"value": X} são tratados como campos escalares:
    {"tensao_a":{"value":223.5}} -> chave "tensao_a", valor "223.5"
- Objetos aninhados e arrays são percorridos recursivamente, concatenando
  nomes com "_" para formar chaves únicas.

================================================================================
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>
#include "json_flatten.h"

// -----------------------------------------------------------------------------
// Função auxiliar: converte um JsonVariantConst escalar em String
// -----------------------------------------------------------------------------
static String scalarToString(JsonVariantConst v) {
    if (v.isNull()) {
        return "";
    }

    if (v.is<bool>()) {
        return v.as<bool>() ? "true" : "false";
    }

    // Números inteiros
    if (v.is<long>() || v.is<int>()) {
        long x = v.as<long>();
        return String(x);
    }

    // Números com ponto flutuante
    if (v.is<float>() || v.is<double>()) {
        double x = v.as<double>();
        // 6 casas é um bom compromisso (ajuste se quiser menos/more)
        return String(x, 6);
    }

    // Strings
    const char* s = v.as<const char*>();
    if (s) {
        return String(s);
    }

    // Fallback
    return "";
}

// -----------------------------------------------------------------------------
// flattenToArrays
// -----------------------------------------------------------------------------
void flattenToArrays(JsonVariantConst v,
                     const String &prefix,
                     String *keys,
                     String *values,
                     size_t &count,
                     size_t maxCount) {
    // Proteções básicas
    if (!v || count >= maxCount) {
        return;
    }

    // -------------------------------------------------------------------------
    // Caso 1: Objeto JSON
    // -------------------------------------------------------------------------
    if (v.is<JsonObjectConst>()) {
        JsonObjectConst obj = v.as<JsonObjectConst>();

        // Regra especial: {"value": X} -> chave = prefixo, valor = X
        if (obj.size() == 1) {
            JsonPairConst kv = *obj.begin();
            const char* k = kv.key().c_str();
            JsonVariantConst inner = kv.value();

            if (strcmp(k, "value") == 0) {
                // Só faz sentido se houver prefixo (nome do campo)
                if (prefix.length() > 0 && count < maxCount) {
                    String valStr = scalarToString(inner);
                    keys[count]   = prefix;
                    values[count] = valStr;
                    count++;
                }
                return;
            }
        }

        // Caso geral: objeto com N pares chave/valor
        for (JsonPairConst kv : obj) {
            const char* k = kv.key().c_str();
            JsonVariantConst child = kv.value();

            String newPrefix;
            if (prefix.length() > 0) {
                newPrefix.reserve(prefix.length() + 1 + strlen(k));
                newPrefix  = prefix;
                newPrefix += "_";
                newPrefix += k;
            } else {
                newPrefix = k;   // nível raiz
            }

            flattenToArrays(child, newPrefix, keys, values, count, maxCount);
            if (count >= maxCount) {
                break;
            }
        }
        return;
    }

    // -------------------------------------------------------------------------
    // Caso 2: Array JSON
    // -------------------------------------------------------------------------
    if (v.is<JsonArrayConst>()) {
        JsonArrayConst arr = v.as<JsonArrayConst>();
        size_t idx = 0;
        for (JsonVariantConst child : arr) {
            String newPrefix = prefix;
            if (newPrefix.length() > 0) {
                newPrefix += "_";
            }
            newPrefix += idx;  // sufixo com índice do array

            flattenToArrays(child, newPrefix, keys, values, count, maxCount);
            if (count >= maxCount) {
                break;
            }
            idx++;
        }
        return;
    }

    // -------------------------------------------------------------------------
    // Caso 3: Escalar (string, número, bool, etc.)
    // -------------------------------------------------------------------------
    if (prefix.length() == 0 || count >= maxCount) {
        // Sem prefixo não temos nome de coluna; ignoramos
        return;
    }

    String valStr = scalarToString(v);
    keys[count]   = prefix;
    values[count] = valStr;
    count++;
}

// -----------------------------------------------------------------------------
// findValueForKey
// -----------------------------------------------------------------------------
String findValueForKey(const String &key,
                       String *keys,
                       String *values,
                       size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (keys[i] == key) {
            return values[i];
        }
    }
    return "";
}
