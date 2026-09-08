# Message Envelope

Formato pequeno e generico para padronizar mensagens entre dispositivos, servidores, simuladores e interfaces.

## Getting Started - 2 minutos

Pre-requisito: Node.js instalado.

Na raiz do repositorio:

```bash
cd features/message-envelope
node tests/test.js
```

Resultado esperado:

```text
PASS message-envelope
```

Para testar no seu proprio codigo, crie `example.js` dentro desta pasta:

```javascript
const { createMessage, createAck } = require('./src/message_envelope');

const command = createMessage({
  type: 'cmd',
  sequence: 1,
  source: 'example',
  payload: { command: 'start' }
});

console.log(command);
console.log(createAck(command.sequence, { status: 'ok' }));
```

Execute:

```bash
node example.js
```

Se aparecerem um comando e seu ACK com `ref_sequence: 1`, a feature esta pronta para uso.

## Tipos suportados

- `cmd`
- `ack`
- `error`
- `state`
- `telemetry`
- `event`

## Ambiente

- Microcontrolador: nao e necessario.
- Runtime: Node.js.
- IDE: qualquer editor; VS Code e suficiente.
- Dependencias externas: nenhuma.

## Estrutura

```json
{
  "type": "cmd",
  "sequence": 7,
  "source": "controller",
  "timestamp": 1000,
  "payload": {
    "command": "start"
  }
}
```

ACK e erro usam `ref_sequence` para indicar a mensagem de origem.

## Uso

```javascript
const {
  createMessage,
  createAck,
  createError,
  validateMessage
} = require('./src/message_envelope');

const command = createMessage({
  type: 'cmd',
  sequence: 10,
  source: 'controller',
  payload: { command: 'read_status' }
});

const ack = createAck(10, { value: 123 });
const error = createError(10, 1200, 'rejected');

console.log(validateMessage(command));
console.log(ack);
console.log(error);
```

## Teste

```bash
node tests/test.js
```

Resultado esperado:

```text
PASS message-envelope
```

## Status de validacao

Teste funcional de criacao e validacao de comando, ACK e erro, incluindo correlacao por `ref_sequence`.

## Observacao

Esta feature nao e obrigatoria para utilizar nenhuma das outras pastas do repositorio. Ela existe como uma opcao de formato comum para quem quiser padronizar mensagens.
