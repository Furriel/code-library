// Generic message envelope
//
// The envelope gives different transports the same small message shape.
// It does not know anything about the device or application using it.

const MESSAGE_TYPES = new Set([
  'cmd',
  'ack',
  'error',
  'state',
  'telemetry',
  'event'
]);

function createMessage({
  type,
  sequence = 0,
  source = 'device',
  timestamp = Date.now(),
  payload = {}
}) {
  if (!MESSAGE_TYPES.has(type)) {
    throw new Error(`unsupported message type: ${type}`);
  }

  if (!Number.isInteger(sequence) || sequence < 0) {
    throw new Error('sequence must be a non-negative integer');
  }

  return {
    type,
    sequence,
    source,
    timestamp,
    payload
  };
}

function createAck(refSequence, payload = {}) {
  return createMessage({
    type: 'ack',
    payload: {
      ref_sequence: refSequence,
      status: 'accepted',
      ...payload
    }
  });
}

function createError(refSequence, code, message, payload = {}) {
  return createMessage({
    type: 'error',
    payload: {
      ref_sequence: refSequence,
      code,
      message,
      ...payload
    }
  });
}

function validateMessage(message) {
  if (!message || typeof message !== 'object') return false;
  if (!MESSAGE_TYPES.has(message.type)) return false;
  if (!Number.isInteger(message.sequence) || message.sequence < 0) return false;
  if (typeof message.source !== 'string' || message.source.length === 0) return false;
  if (!Number.isFinite(message.timestamp) || message.timestamp < 0) return false;
  if (!message.payload || typeof message.payload !== 'object' || Array.isArray(message.payload)) return false;

  if (message.type === 'ack' || message.type === 'error') {
    if (!Number.isInteger(message.payload.ref_sequence)) return false;
  }

  if (message.type === 'error') {
    if (!Number.isInteger(message.payload.code)) return false;
    if (typeof message.payload.message !== 'string' || message.payload.message.length === 0) return false;
  }

  return true;
}

module.exports = {
  MESSAGE_TYPES,
  createMessage,
  createAck,
  createError,
  validateMessage
};
