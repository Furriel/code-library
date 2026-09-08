# Reusable Code Library

Biblioteca de exemplos e componentes independentes para sistemas embarcados, comunicacao e ferramentas de validacao.

O objetivo deste repositorio e manter codigo simples, legivel e de livre reutilizacao. Cada feature deve funcionar de forma independente: ela nao pode depender de outra pasta deste repositorio para compilar, executar ou ser testada.

## Comece em 2 minutos

Clone o repositorio:

```bash
git clone https://github.com/Furriel/code-library.git
cd code-library
```

Depois escolha uma feature. Cada pasta possui um `Getting Started - 2 minutos` com os comandos minimos para executar ou validar o codigo.

| Feature | Teste rapido |
|---|---|
| [`message-envelope`](features/message-envelope/) | `cd features/message-envelope && node tests/test.js` |
| [`transaction-tracker`](features/transaction-tracker/) | `cd features/transaction-tracker && node tests/test.js` |
| [`nmea-gga-parser`](features/nmea-gga-parser/) | `cd features/nmea-gga-parser && node tests/test.js` |
| [`websocket-command-channel`](features/websocket-command-channel/) | `cd features/websocket-command-channel && npm install && npm test` |
| [`serial-espnow-bridge`](features/serial-espnow-bridge/) | `cd features/serial-espnow-bridge && pio run -e bridge_a` |

> Os comandos acima sao apenas atalhos. O README de cada feature explica pre-requisitos, resultado esperado, exemplo minimo e nivel de validacao.

## Onde procurar no codigo

Os arquivos principais possuem no topo uma secao `LEIA NESTA ORDEM` e blocos numerados como `[1]`, `[2]`, `[3]`. Isso serve como um indice dentro do proprio codigo.

| Quero encontrar... | Abra | Procure por |
|---|---|---|
| tipos e formato das mensagens | `features/message-envelope/src/message_envelope.js` | `[1] TIPOS`, `[2] CRIACAO`, `[5] VALIDACAO` |
| correlacao entre comando e ACK/erro | `features/transaction-tracker/src/transaction_tracker.js` | `[2] INICIO`, `[3] FINALIZACAO`, `[4] ENTRADA` |
| conversao de coordenadas NMEA | `features/nmea-gga-parser/src/nmea_gga.js` | `[1] CONVERSAO` |
| campos retornados pelo parser GGA | `features/nmea-gga-parser/src/nmea_gga.js` | `[3] PARSER PRINCIPAL` |
| tratamento de comandos WebSocket | `features/websocket-command-channel/src/server.js` | `[5] ENTRADA DE CONEXOES E COMANDOS` |
| broadcast de telemetria/estado | `features/websocket-command-channel/src/server.js` | `[4] BROADCAST` |
| baudrate, heartbeat e tamanho de JSON do ESP32 | `features/serial-espnow-bridge/include/bridge_config.h` | `[1]`, `[2]`, `[3]` |
| envio Serial -> ESP-NOW | `features/serial-espnow-bridge/src/main.cpp` | `[4] ENVIO SERIAL -> ESP-NOW` |
| recepcao ESP-NOW -> Serial | `features/serial-espnow-bridge/src/main.cpp` | `[5] CALLBACKS DO ESP-NOW` |
| comandos `STATUS` e `PEER` | `features/serial-espnow-bridge/src/main.cpp` | `[6] INTERPRETACAO DE UMA LINHA` |
| fluxo principal do firmware ESP32 | `features/serial-espnow-bridge/src/main.cpp` | `[9] CICLO PRINCIPAL` |

### Padrao de comentarios

O objetivo dos comentarios nao e traduzir cada linha de codigo. Eles devem permitir que uma pessoa encontre e compreenda o fluxo sem conhecer previamente a implementacao.

Cada arquivo principal deve ter:

1. objetivo do arquivo;
2. fluxo simplificado quando houver entrada/processamento/saida;
3. `LEIA NESTA ORDEM` com as principais funcoes ou blocos;
4. secoes numeradas pesquisaveis (`[1]`, `[2]`, ...);
5. comentario antes de funcoes importantes explicando entrada, saida e motivo de existencia;
6. comentario nos pontos onde uma decisao tecnica nao e obvia;
7. indicacao clara de onde alterar configuracoes comuns.

## Regras para novas features

1. Cada feature fica em sua propria pasta.
2. A feature deve conter tudo que precisa para funcionar.
3. Dependencias externas devem estar declaradas dentro da propria feature.
4. O codigo deve priorizar clareza em vez de abstracoes desnecessarias.
5. Funcoes importantes devem possuir comentarios explicando o que fazem e por que existem.
6. Cada arquivo principal deve possuir um mapa de leitura com secoes numeradas pesquisaveis.
7. Cada feature deve possuir um README com ambiente, hardware, IDE, dependencias, exemplo de uso, teste e um `Getting Started - 2 minutos`.
8. O status de validacao deve distinguir teste de software, compilacao e teste em hardware.

## Features

| Feature | Funcao | Microcontrolador | Ambiente / IDE | Validacao |
|---|---|---|---|---|
| [`message-envelope`](features/message-envelope/) | Formato generico para `cmd`, `ack`, `error`, `state`, `telemetry` e `event` | Independente de MCU | Node.js / qualquer editor | Teste funcional |
| [`transaction-tracker`](features/transaction-tracker/) | Correlaciona comandos com ACK/erro por numero de sequencia e mede latencia | Independente de MCU | Node.js / qualquer editor | Teste funcional |
| [`nmea-gga-parser`](features/nmea-gga-parser/) | Converte sentencas NMEA GGA em latitude, longitude e informacoes do fix | Independente de MCU | Node.js / qualquer editor | Teste funcional |
| [`websocket-command-channel`](features/websocket-command-channel/) | Canal WebSocket simples com comando, ACK, erro, estado e telemetria | Independente de MCU | Node.js / qualquer editor | Teste E2E local |
| [`serial-espnow-bridge`](features/serial-espnow-bridge/) | Bridge Serial JSON <-> ESP-NOW com diagnostico e heartbeat | ESP32 | PlatformIO + VS Code, framework Arduino | Compilacao validada; bancada fisica pendente |

## Estrutura de uma feature

```text
features/
  nome-da-feature/
    README.md
    src/
    tests/
    examples/       # quando necessario
    package.json    # quando necessario
    platformio.ini  # quando necessario
```

Nao e necessario clonar ou usar outra feature para executar uma pasta individual. Quando duas features usam ideias compativeis, a compatibilidade e mantida por formatos simples, nao por dependencia entre diretorios.

## Licenca

Este repositorio usa a licenca MIT. Ela permite uso pessoal, academico e comercial, modificacao e redistribuicao, desde que o aviso de copyright e a licenca sejam preservados.

A escolha da MIT e intencional: o objetivo desta biblioteca e facilitar reutilizacao de codigo com o minimo de restricoes.

Consulte [`LICENSE`](LICENSE).
