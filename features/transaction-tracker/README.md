# Transaction Tracker

Rastreador simples para correlacionar comandos com ACK ou erro usando um numero de sequencia.

## Getting Started - 2 minutos

Pre-requisito: Node.js instalado.

Na raiz do repositorio:

```bash
cd features/transaction-tracker
node tests/test.js
```

Resultado esperado:

```text
PASS transaction-tracker
```

Para testar manualmente, crie `example.js` nesta pasta:

```javascript
const { TransactionTracker } = require('./src/transaction_tracker');

const tracker = new TransactionTracker();
tracker.start(1, 'read_status');

setTimeout(() => {
  tracker.handleEnvelope({
    type: 'ack',
    payload: { ref_sequence: 1 }
  });

  console.log(tracker.get(1));
}, 100);
```

Execute:

```bash
node example.js
```

O resultado deve mostrar a transacao `1` finalizada como ACK e uma latencia proxima de 100 ms.

## Onde mexer no codigo

Arquivo principal: `src/transaction_tracker.js`.

| Quero entender/alterar... | Procure por |
|---|---|
| onde as transacoes ficam armazenadas | `[1] MEMORIA INTERNA` |
| registro de um novo comando | `[2] INICIO DE UMA TRANSACAO` |
| calculo da latencia e encerramento | `[3] FINALIZACAO` |
| tratamento de ACK e erro recebidos | `[4] ENTRADA DE RESPOSTAS` |
| consulta e limpeza do historico | `[5] CONSULTAS` |

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
