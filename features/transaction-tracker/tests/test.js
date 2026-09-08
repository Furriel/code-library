const assert = require('assert');
const { TransactionTracker } = require('../src/transaction_tracker');

const tracker = new TransactionTracker();

tracker.start(10, 'start', 1000);
const ack = tracker.handleEnvelope({
  type: 'ack',
  payload: { ref_sequence: 10 }
}, 1042);

assert.strictEqual(ack.status, 'ack');
assert.strictEqual(ack.latency_ms, 42);

tracker.start(11, 'stop', 2000);
const error = tracker.handleEnvelope({
  type: 'error',
  payload: {
    ref_sequence: 11,
    code: 1200,
    message: 'rejected'
  }
}, 2075);

assert.strictEqual(error.status, 'error');
assert.strictEqual(error.latency_ms, 75);
assert.strictEqual(error.error.code, 1200);
assert.strictEqual(tracker.list().length, 2);
assert.strictEqual(tracker.get(999), null);

console.log('PASS transaction-tracker');
