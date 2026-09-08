const assert = require('assert');
const {
  createMessage,
  createAck,
  createError,
  validateMessage
} = require('../src/message_envelope');

const command = createMessage({
  type: 'cmd',
  sequence: 7,
  source: 'controller',
  timestamp: 1000,
  payload: { command: 'start' }
});

assert.strictEqual(validateMessage(command), true);

const ack = createAck(7, { result: 'ok' });
assert.strictEqual(validateMessage(ack), true);
assert.strictEqual(ack.payload.ref_sequence, 7);

const error = createError(7, 1200, 'rejected');
assert.strictEqual(validateMessage(error), true);
assert.strictEqual(error.payload.code, 1200);

assert.strictEqual(validateMessage({ type: 'unknown' }), false);
assert.throws(() => createMessage({ type: 'unknown' }));

console.log('PASS message-envelope');
