# WebSocket Command Channel

Canal WebSocket pequeno para sistemas que precisam enviar comandos e receber ACK, erro, estado e telemetria.

## Getting Started - 2 minutos

Pre-requisitos: Node.js e npm instalados.

Na raiz do repositorio:

```bash
cd features/websocket-command-channel
npm install
npm test
```

Resultado esperado:

```text
PASS websocket-command-channel E2E
```

O teste ja faz o ciclo completo automaticamente:

```text
cliente -> comando -> servidor -> ACK/erro -> cliente
```

Tambem valida broadcast de `state` e `telemetry`.

Para usar em um programa, crie `example.js` nesta pasta:

```javascript
const { createCommandServer } = require('./src/server');

createCommandServer({
  port: 8787,
  commandHandler(payload) {
    if (payload.command === 'ping') {
      return { accepted: true, payload: { value: 'pong' } };
    }

    return { accepted: false, code: 1200, message: 'unknown command' };
  }
});

console.log('WebSocket ativo em ws://127.0.0.1:8787');
```

Execute:

```bash
node example.js
```

Se o servidor iniciar sem erro, a feature esta pronta para receber clientes WebSocket.

## O que faz

- recebe mensagens `cmd`;
- entrega o comando para uma funcao da aplicacao;
- responde com `ack` ou `error`;
- permite publicar `state`, `telemetry` e outros tipos de mensagem para todos os clientes;
- usa `sequence` e `ref_sequence` para correlacionar comando e resposta.

## Ambiente

- Microcontrolador: nao e necessario.
- Runtime: Node.js.
- IDE: VS Code ou qualquer editor.
- Dependencia externa: pacote `ws`.

## Instalacao

Dentro desta pasta:

```bash
npm install
```

## Uso

```javascript
const { createCommandServer } = require('./src/server');

const server = createCommandServer({
  port: 8787,
  commandHandler(payload) {
    if (payload.command === 'ping') {
      return {
        accepted: true,
        payload: { value: 'pong' }
      };
    }

    return {
      accepted: false,
      code: 1200,
      message: 'unknown command'
    };
  }
});

server.broadcast('telemetry', { temperature: 25.4 });
```

## Formato de comando

```json
{
  "type": "cmd",
  "sequence": 10,
  "payload": {
    "command": "ping"
  }
}
```

Resposta:

```json
{
  "type": "ack",
  "payload": {
    "ref_sequence": 10,
    "status": "accepted",
    "value": "pong"
  }
}
```

## Teste

```bash
npm install
npm test
```

O teste E2E cria um servidor local, conecta um cliente WebSocket e verifica:

- comando aceito;
- ACK correlacionado por sequencia;
- comando desconhecido com erro;
- broadcast de estado;
- broadcast de telemetria.

Resultado esperado:

```text
PASS websocket-command-channel E2E
```

## Status de validacao

Fluxo WebSocket local validado de ponta a ponta em software.
