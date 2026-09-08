# Reusable Code Library

Biblioteca de exemplos e componentes independentes para sistemas embarcados, comunicacao e ferramentas de validacao.

O objetivo deste repositorio e manter codigo simples, legivel e de livre reutilizacao. Cada feature deve funcionar de forma independente: ela nao pode depender de outra pasta deste repositorio para compilar, executar ou ser testada.

## Regras para novas features

1. Cada feature fica em sua propria pasta.
2. A feature deve conter tudo que precisa para funcionar.
3. Dependencias externas devem estar declaradas dentro da propria feature.
4. O codigo deve priorizar clareza em vez de abstrações desnecessarias.
5. Funcoes importantes devem possuir comentarios explicando o que fazem e por que existem.
6. Cada feature deve possuir um README com ambiente, hardware, IDE, dependencias, exemplo de uso e teste.
7. O status de validacao deve distinguir teste de software, compilacao e teste em hardware.

## Features

| Feature | Funcao | Microcontrolador | Ambiente / IDE | Validacao |
|---|---|---|---|---|
| [`nmea-gga-parser`](features/nmea-gga-parser/) | Converte sentencas NMEA GGA em latitude, longitude e informacoes do fix | Independente de MCU | Node.js / qualquer editor | Teste funcional |
| [`transaction-tracker`](features/transaction-tracker/) | Correlaciona comandos com ACK/erro por numero de sequencia e mede latencia | Independente de MCU | Node.js / qualquer editor | Teste funcional |
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

## Licenca

Este repositorio usa a licenca MIT. Ela permite uso pessoal, academico e comercial, modificacao e redistribuicao, desde que o aviso de copyright e a licenca sejam preservados.

A escolha da MIT e intencional: o objetivo desta biblioteca e facilitar reutilizacao de codigo com o minimo de restricoes.

Consulte [`LICENSE`](LICENSE).
