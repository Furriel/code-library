const { WebSocketServer, WebSocket } = require('ws');

// Keep the wire format deliberately small. Applications may add fields to
// payload without changing the command channel itself.
function createEnvelope(type, payload = {}, sequence = 0) {
  return {
    type,
    sequence,
    timestamp: Date.now(),
    payload
  };
}

function createCommandServer({ port = 8787, commandHandler }) {
  if (typeof commandHandler !== 'function') {
    throw new Error('commandHandler must be a function');
  }

  const wss = new WebSocketServer({ port });

  function send(ws, message) {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify(message));
    }
  }

  function broadcast(type, payload = {}) {
    const raw = JSON.stringify(createEnvelope(type, payload));
    for (const client of wss.clients) {
      if (client.readyState === WebSocket.OPEN) client.send(raw);
    }
  }

  wss.on('connection', (ws) => {
    ws.on('message', async (raw) => {
      let message;

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

      if (message.type !== 'cmd') {
        send(ws, createEnvelope('error', {
          ref_sequence: message.sequence ?? 0,
          code: 1001,
          message: 'expected type=cmd'
        }));
        return;
      }

      try {
        const result = await commandHandler(message.payload || {}, message);

        if (result?.accepted === false) {
          send(ws, createEnvelope('error', {
            ref_sequence: message.sequence ?? 0,
            code: result.code ?? 1200,
            message: result.message ?? 'command rejected'
          }));
          return;
        }

        send(ws, createEnvelope('ack', {
          ref_sequence: message.sequence ?? 0,
          status: 'accepted',
          ...(result?.payload || {})
        }));
      } catch (error) {
        send(ws, createEnvelope('error', {
          ref_sequence: message.sequence ?? 0,
          code: 1500,
          message: error.message || 'handler error'
        }));
      }
    });
  });

  function close() {
    return new Promise((resolve) => wss.close(resolve));
  }

  return { wss, broadcast, close };
}

module.exports = {
  createEnvelope,
  createCommandServer
};
