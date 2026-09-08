# Transaction Tracker

Rastreador simples para correlacionar comandos com ACK ou erro usando um numero de sequencia.

## Por que existe

Em comunicacoes assincronas, a resposta pode chegar depois de outros eventos. O `sequence` identifica cada comando e o `ref_sequence` identifica a resposta correspondente.

A feature tambem calcula a latencia entre envio e resposta.

## Ambiente

- Microcontrolador: nao e necessario.
- Runtime: Node.js.
- IDE: qualquer editor; VS Code e suficiente.
- Dependencias externas: nenhuma.
- Transporte: independente. Pode ser usado com Serial, WebSocket, MQTT, TCP, CAN ou outro meio.

## Arquivos

```text
src/transaction_tracker.js
tests/test.js
```

## Uso

```javascript
const { TransactionTracker } = require('./src/transaction_tracker');

const tracker = new TransactionTracker();

tracker.start(25, 'read_status');

tracker.handleEnvelope({
  type: 'ack',
  payload: { ref_sequence: 25 }
});

console.log(tracker.get(25));
```

## Formato minimo de resposta

ACK:

```json
{
  "type": "ack",
  "payload": {
    "ref_sequence": 25
  }
}
```

Erro:

```json
{
  "type": "error",
  "payload": {
    "ref_sequence": 25,
    "code": 1200,
    "message": "rejected"
  }
}
```

## Teste

```bash
node tests/test.js
```

Resultado esperado:

```text
PASS transaction-tracker
```

## Status de validacao

Teste funcional de correlacao por sequencia, ACK, erro, armazenamento e calculo de latencia.
