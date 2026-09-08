const assert = require('assert');
const WebSocket = require('ws');
const { createCommandServer } = require('../src/server');

function waitFor(check, timeoutMs = 3000) {
  return new Promise((resolve, reject) => {
    const started = Date.now();
    const timer = setInterval(() => {
      if (check()) {
        clearInterval(timer);
        resolve();
      } else if (Date.now() - started > timeoutMs) {
        clearInterval(timer);
        reject(new Error('timeout waiting for expected message'));
      }
    }, 20);
  });
}

async function run() {
  const port = 8899;

  const server = createCommandServer({
    port,
    commandHandler(payload) {
      if (payload.command === 'ping') {
        return { accepted: true, payload: { value: 'pong' } };
      }

      return {
        accepted: false,
        code: 1200,
        message: 'unknown command'
      };
    }
  });

  const inbox = [];
  const ws = new WebSocket(`ws://127.0.0.1:${port}`);

  await new Promise((resolve, reject) => {
    ws.once('open', resolve);
    ws.once('error', reject);
  });

  ws.on('message', (raw) => inbox.push(JSON.parse(String(raw))));

  ws.send(JSON.stringify({
    type: 'cmd',
    sequence: 10,
    payload: { command: 'ping' }
  }));

  await waitFor(() => inbox.some(
    (m) => m.type === 'ack' && m.payload.ref_sequence === 10
  ));

  const ack = inbox.find((m) => m.type === 'ack');
  assert.strictEqual(ack.payload.value, 'pong');

  ws.send(JSON.stringify({
    type: 'cmd',
    sequence: 11,
    payload: { command: 'unknown' }
  }));

  await waitFor(() => inbox.some(
    (m) => m.type === 'error' && m.payload.ref_sequence === 11
  ));

  server.broadcast('state', { online: true });
  server.broadcast('telemetry', { value: 123 });

  await waitFor(() =>
    inbox.some((m) => m.type === 'state') &&
    inbox.some((m) => m.type === 'telemetry')
  );

  ws.close();
  await new Promise((resolve) => ws.once('close', resolve));
  await server.close();

  console.log('PASS websocket-command-channel E2E');
}

run().catch((error) => {
  console.error(`FAIL websocket-command-channel E2E: ${error.message}`);
  process.exit(1);
});
