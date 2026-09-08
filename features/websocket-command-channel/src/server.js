// WEBSOCKET COMMAND CHANNEL
// -----------------------------------------------------------------------------
// Objetivo: receber comandos por WebSocket e responder com ACK ou erro.
// Tambem permite publicar estado/telemetria para todos os clientes conectados.
//
// Fluxo principal:
//   cliente -> cmd -> commandHandler() -> ack/error -> cliente
//
// LEIA NESTA ORDEM:
//   [1] createEnvelope      -> formato das mensagens
//   [2] createCommandServer -> cria o servidor
//   [3] send                -> envia para um cliente
//   [4] broadcast           -> envia para todos os clientes
//   [5] connection/message  -> recebe e trata comandos
//   [6] close               -> encerra o servidor
//
// Se quiser adaptar a logica da sua aplicacao, normalmente voce mexera apenas
// no commandHandler fornecido para createCommandServer().

const { WebSocketServer, WebSocket } = require('ws');

// [1] FORMATO DA MENSAGEM
// Mantemos o envelope pequeno de proposito. O canal so precisa saber o tipo,
// sequencia, instante e payload. Campos especificos ficam dentro de payload.
function createEnvelope(type, payload = {}, sequence = 0) {
  return {
    type,
    sequence,
    timestamp: Date.now(),
    payload
  };
}

// [2] CRIACAO DO SERVIDOR
//
// port: porta TCP usada pelo WebSocket.
// commandHandler(payload, message): funcao da aplicacao que decide o que fazer
// com cada comando recebido.
//
// commandHandler deve retornar algo como:
//   { accepted: true, payload: { ... } }
// ou
//   { accepted: false, code: 1200, message: 'motivo' }
function createCommandServer({ port = 8787, commandHandler }) {
  if (typeof commandHandler !== 'function') {
    throw new Error('commandHandler must be a function');
  }

  const wss = new WebSocketServer({ port });

  // [3] ENVIO PARA UM CLIENTE
  // Verifica se a conexao ainda esta aberta antes de enviar. Isso evita tentar
  // escrever em um socket que ja foi encerrado.
  function send(ws, message) {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify(message));
    }
  }

  // [4] BROADCAST
  // Use esta funcao para telemetria, estado ou eventos que todos os clientes
  // conectados precisam receber.
  function broadcast(type, payload = {}) {
    const raw = JSON.stringify(createEnvelope(type, payload));

    for (const client of wss.clients) {
      if (client.readyState === WebSocket.OPEN) {
        client.send(raw);
      }
    }
  }

  // [5] ENTRADA DE CONEXOES E COMANDOS
  // Cada novo cliente recebe seu proprio listener de mensagens.
  wss.on('connection', (ws) => {
    ws.on('message', async (raw) => {
      let message;

      // 5.1 - Primeiro transformamos o texto recebido em objeto JavaScript.
      try {
        message = JSON.parse(String(raw));
      } catch {
        send(ws, createEnvelope('error', {
          ref_sequence: 0,
          code: 1000,
          message: 'invalid json'
        }));
        return;
      }

      // 5.2 - Este canal foi feito para receber comandos. Outros tipos de
      // mensagem sao rejeitados para manter o comportamento previsivel.
      if (message.type !== 'cmd') {
        send(ws, createEnvelope('error', {
          ref_sequence: message.sequence ?? 0,
          code: 1001,
          message: 'expected type=cmd'
        }));
        return;
      }

      try {
        // 5.3 - A regra da aplicacao fica fora desta feature. Aqui apenas
        // entregamos payload + mensagem completa para o callback do usuario.
        const result = await commandHandler(message.payload || {}, message);

        // 5.4 - accepted=false transforma o resultado em uma resposta error.
        if (result?.accepted === false) {
          send(ws, createEnvelope('error', {
            ref_sequence: message.sequence ?? 0,
            code: result.code ?? 1200,
            message: result.message ?? 'command rejected'
          }));
          return;
        }

        // 5.5 - Qualquer resultado aceito gera ACK apontando para a sequencia
        // do comando original.
        send(ws, createEnvelope('ack', {
          ref_sequence: message.sequence ?? 0,
          status: 'accepted',
          ...(result?.payload || {})
        }));
      } catch (error) {
        // Excecao dentro do commandHandler vira erro controlado para o cliente.
        send(ws, createEnvelope('error', {
          ref_sequence: message.sequence ?? 0,
          code: 1500,
          message: error.message || 'handler error'
        }));
      }
    });
  });

  // [6] ENCERRAMENTO
  // Retorna Promise para permitir: await server.close().
  function close() {
    return new Promise((resolve) => wss.close(resolve));
  }

  // API publica: servidor bruto, broadcast e fechamento limpo.
  return { wss, broadcast, close };
}

module.exports = {
  createEnvelope,
  createCommandServer
};
