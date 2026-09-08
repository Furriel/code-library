# ESP32 SD Rotating Logger

Logger simples para ESP32 que grava uma linha por registro no microSD, cria diretorios por data e troca automaticamente o arquivo por hora ou por dia.

A feature e independente e nao depende de outro diretorio deste repositorio.

## O que ela faz

- monta o microSD ou usa um SD ja montado;
- cria automaticamente a arvore `/<raiz>/AAAA/MM/DD/`;
- gera nome de arquivo por hora ou por dia;
- grava texto, CSV, JSON Lines ou qualquer formato de uma linha;
- usa `FILE_APPEND`;
- pode fazer `flush()` a cada registro;
- aceita horario fornecido pelo usuario;
- aceita um `TimeProvider` para NTP, RTC ou outra fonte;
- aceita mutex compartilhado para uso concorrente do cartao.

## Ambiente

- MCU: ESP32 classico / ESP32 DevKit
- Framework: Arduino
- IDE recomendada: VS Code + PlatformIO
- Cartao: microSD via SPI

## Dependencias

Somente bibliotecas incluidas no Arduino-ESP32: `SD`, `SPI` e FreeRTOS.

## Getting Started - 2 minutos

```bash
cd features/esp32-sd-rotating-logger
pio run
```

Para gravar em uma placa:

```bash
pio run -t upload
pio device monitor -b 115200
```

O exemplo usa uma data fixa para nao depender de Wi-Fi ou RTC. Troque por `appendLine()` com um `TimeProvider` real quando integrar na sua aplicacao.

## Exemplo minimo

```cpp
SdRotatingLoggerConfig config;
config.root = "/data";
config.prefix = "log";
config.extension = ".csv";
config.rotation = SdLogRotation::Hourly;

SdRotatingLogger logger(config);
logger.begin(5);

struct tm now = {};
now.tm_year = 2026 - 1900;
now.tm_mon = 9 - 1;
now.tm_mday = 8;
now.tm_hour = 14;

logger.appendLineAt("temperature,25.4", now);
```

Resultado:

```text
/data/2026/09/08/log-2026-09-08-14.csv
```

## Rotacao diaria

```cpp
config.rotation = SdLogRotation::Daily;
```

Resultado:

```text
/data/2026/09/08/log-2026-09-08.csv
```

## Usando NTP ou RTC

Forneca uma funcao que preencha `struct tm`:

```cpp
bool myClock(struct tm *out) {
  return getLocalTime(out, 100);
}

logger.setTimeProvider(myClock);
logger.appendLine("temperature,25.4");
```

O logger nao decide de onde vem o horario. Isso permite usar NTP, DS3231 ou outra fonte sem alterar a biblioteca.

## Onde mexer no codigo

Abra `src/sd_rotating_logger.cpp`:

| Quero alterar... | Procure por |
|---|---|
| criacao do mutex e montagem | `[1] INICIALIZACAO` |
| criacao de pastas | `[2] DIRETORIOS` |
| formato do nome de arquivo | `[3] CAMINHO DO ARQUIVO` |
| escrita e flush | `[4] GRAVACAO` |
| fonte de horario | `[5] HORARIO AUTOMATICO` |

No header, procure por `[1] CONFIGURACAO` para ver os parametros publicos.

## Compartilhando o SD

```cpp
SemaphoreHandle_t sdMutex = xSemaphoreCreateMutex();
logger.begin(5, sdMutex);
```

Passe o mesmo mutex para qualquer outro modulo que use o cartao.

## Status de validacao

- software/estrutura: revisado para uso independente;
- compilacao: validada pelo CI quando o PR correspondente passa;
- hardware: pendente de validacao fisica desta versao generalizada;
- persistencia apos falta de energia e endurance do cartao: nao validadas por esta feature.

## Observacoes

`flush_each_line=true` favorece persistencia dos dados, mas aumenta o numero de operacoes no cartao. Em registros de alta frequencia, pode ser melhor agrupar linhas e fazer flush em intervalos controlados.
