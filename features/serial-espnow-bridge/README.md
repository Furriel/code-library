# Serial <-> ESP-NOW Bridge

Bridge simples para transportar linhas JSON entre uma porta Serial e ESP-NOW.

A implementacao foi mantida deliberadamente pequena: o bridge nao interpreta os campos da aplicacao. Ele apenas valida se a entrada e um objeto JSON e transporta o conteudo.

## Hardware e ambiente

- Microcontrolador: ESP32 classico / ESP32 DevKit.
- Placa de referencia no PlatformIO: `esp32doit-devkit-v1`.
- IDE recomendada: VS Code.
- Extensao / build system: PlatformIO.
- Framework: Arduino para ESP32.
- Comunicacao com PC: Serial 115200 bit/s.
- Comunicacao sem fio: ESP-NOW.
- Biblioteca externa: ArduinoJson 6.x.

## Arquivos

```text
src/main.cpp
include/bridge_config.h
platformio.ini
README.md
```

A feature nao depende de nenhuma outra pasta do repositorio.

## Funcionamento

```text
PC / dispositivo
      |
   Serial JSON
      |
    ESP32 A
      |
    ESP-NOW
      |
    ESP32 B
      |
   Serial JSON
      |
PC / dispositivo
```

Cada mensagem da aplicacao deve ser um objeto JSON em uma unica linha:

```json
{"type":"telemetry","value":123}
```

O JSON recebido pelo ESP-NOW e escrito na Serial sem modificar os campos da aplicacao.

## Comandos locais

### STATUS

Mostra os contadores do bridge:

```text
STATUS
```

Resposta semelhante a:

```json
{
  "type":"bridge_status",
  "role":"BRIDGE_A",
  "tx_frames":10,
  "rx_frames":9,
  "tx_errors":0,
  "rx_errors":0,
  "uptime_ms":12345
}
```

### PEER

Seleciona o MAC do ESP32 remoto sem recompilar:

```text
PEER AA:BB:CC:DD:EE:FF
```

Por padrao o firmware usa broadcast, o que facilita o primeiro teste.

## Compilacao

Na pasta da feature:

```bash
pio run -e bridge_a
pio run -e bridge_b
```

Gravacao:

```bash
pio run -e bridge_a -t upload
pio run -e bridge_b -t upload
```

## Por que existe um cabecalho antes do JSON

O ESP-NOW transporta um `BridgePacket` com:

- magic number;
- versao;
- sequencia;
- tamanho;
- JSON.

O cabecalho permite rejeitar frames de outro protocolo e detectar tamanhos invalidos antes de tratar o payload.

## Limite de payload

`BRIDGE_MAX_JSON_BYTES` esta configurado em 200 bytes para manter o pacote completo abaixo do limite classico de payload do ESP-NOW.

Se a aplicacao precisar transportar mensagens maiores, o correto e implementar fragmentacao explicitamente em vez de apenas aumentar este valor.

## Status de validacao

A arquitetura Serial <-> ESP-NOW e os dois papeis de firmware foram previamente validados em compilacao. Esta versao generica preserva a mesma estrutura de transporte, removendo identificadores e payloads especificos.

A validacao fisica ESP32 <-> ESP32 deve ser executada antes de classificar esta feature como `hardware validated`.
