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

Observação:
-----------
Mantém-se genérico para suportar diferentes formatos de publicações MQTT,
incluindo medidores de energia, sensores compostos, etc.

================================================================================
*/

#include "json_flatten.h"

void flattenToArrays(JsonVariantConst v,
                     const String &prefix,
                     String *keys,
                     String *values,
                     size_t &count,
                     size_t maxCount) {
    if (count >= maxCount) return;

    if (v.is<JsonObject>()) {
        JsonObjectConst obj = v.as<JsonObjectConst>();

        // Caso {"value": X}
        if (obj.size() == 1 && obj.containsKey("value")) {
            if (prefix.length() > 0 && count < maxCount) {
                keys[count]   = prefix;
                values[count] = String(obj["value"].as<String>());
                count++;
            }
        } else {
            // Percorre campos filhos
            for (JsonPairConst kv : obj) {
                String key = kv.key().c_str();
                String newPrefix = prefix.length() ? (prefix + "_" + key) : key;
                flattenToArrays(kv.value(), newPrefix, keys, values, count, maxCount);
                if (count >= maxCount) return;
            }
        }
    }
    else if (v.is<JsonArray>()) {
        JsonArrayConst arr = v.as<JsonArrayConst>();
        for (size_t i = 0; i < arr.size(); i++) {
            String idx = String(i);
            String newPrefix = prefix.length() ? (prefix + "_" + idx) : idx;
            flattenToArrays(arr[i], newPrefix, keys, values, count, maxCount);
            if (count >= maxCount) return;
        }
    }
    else if (!v.isNull()) {
        // Valor simples
        if (prefix.length() > 0 && count < maxCount) {
            keys[count]   = prefix;
            values[count] = String(v.as<String>());
            count++;
        }
    }
}

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
