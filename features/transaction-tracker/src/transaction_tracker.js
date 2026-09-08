// Command transaction tracker
//
// The tracker exists to answer a simple question: which ACK or error belongs
// to which command? Sequence numbers provide that correlation without tying the
// code to any transport such as Serial, WebSocket, MQTT or CAN.

class TransactionTracker {
  constructor() {
    this._items = new Map();
  }

  start(sequence, command, sentAtMs = Date.now()) {
    if (!Number.isInteger(sequence) || sequence < 0) {
      throw new Error('sequence must be a non-negative integer');
    }
    if (!command) {
      throw new Error('command is required');
    }
    if (this._items.has(sequence)) {
      throw new Error(`sequence ${sequence} is already in use`);
    }

    const transaction = {
      sequence,
      command: String(command),
      status: 'pending',
      sent_at_ms: sentAtMs,
      received_at_ms: null,
      latency_ms: null,
      error: null
    };

    this._items.set(sequence, transaction);
    return { ...transaction };
  }

  resolve(sequence, status, receivedAtMs = Date.now(), error = null) {
    const transaction = this._items.get(sequence);
    if (!transaction) return null;

    transaction.status = status;
    transaction.received_at_ms = receivedAtMs;
    transaction.latency_ms = Math.max(0, receivedAtMs - transaction.sent_at_ms);
    transaction.error = error;

    return { ...transaction };
  }

  // Accepts a generic ACK/error envelope. Only ref_sequence is required,
  // which keeps the tracker independent from the communication transport.
  handleEnvelope(envelope, receivedAtMs = Date.now()) {
    const type = envelope?.type;
    const refSequence = envelope?.payload?.ref_sequence;

    if (!Number.isInteger(refSequence)) return null;

    if (type === 'ack') {
      return this.resolve(refSequence, 'ack', receivedAtMs);
    }

    if (type === 'error') {
      return this.resolve(
        refSequence,
        'error',
        receivedAtMs,
        {
          code: envelope.payload?.code ?? null,
          message: envelope.payload?.message ?? ''
        }
      );
    }

    return null;
  }

  get(sequence) {
    const item = this._items.get(sequence);
    return item ? { ...item } : null;
  }

  list() {
    return Array.from(this._items.values(), (item) => ({ ...item }));
  }

  clear() {
    this._items.clear();
  }
}

module.exports = { TransactionTracker };
