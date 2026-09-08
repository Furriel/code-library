# Serial <-> ESP-NOW Bridge

Bridge simples para transportar linhas JSON entre uma porta Serial e ESP-NOW.

A implementacao foi mantida deliberadamente pequena: o bridge nao interpreta os campos da aplicacao. Ele apenas valida se a entrada e um objeto JSON e transporta o conteudo.

## Getting Started - 2 minutos

Pre-requisitos:

- ESP32 DevKit ou compativel;
- VS Code + PlatformIO, ou PlatformIO CLI;
- cabo USB.

Para apenas confirmar que o codigo compila, na raiz do repositorio:

```bash
cd features/serial-espnow-bridge
pio run -e bridge_a
pio run -e bridge_b
```

Os dois comandos devem terminar com:

```text
SUCCESS
```

Para gravar uma placa e testar a interface Serial:

```bash
pio run -e bridge_a -t upload
pio device monitor -b 115200
```

No monitor serial envie:

```text
STATUS
```

A placa deve responder com um JSON semelhante a:

```json
{"type":"bridge_status","role":"BRIDGE_A","tx_frames":0,"rx_frames":0,"tx_errors":0,"rx_errors":0}
```

Isso confirma firmware, Serial e comandos locais. A comunicacao ESP-NOW entre duas placas exige duas ESP32 e e tratada separadamente como teste de bancada.

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

Os ambientes `bridge_a` e `bridge_b` compilam no CI com PlatformIO.

O teste rapido com `STATUS` valida firmware, Serial e comandos locais quando executado em uma placa. A comunicacao fisica ESP32 <-> ESP32 continua devendo ser executada antes de classificar esta feature como `hardware validated`.
