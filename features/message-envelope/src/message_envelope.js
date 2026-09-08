// MESSAGE ENVELOPE
// -----------------------------------------------------------------------------
// Objetivo: criar um formato pequeno e previsivel para mensagens entre partes
// de um sistema. Este arquivo nao conhece Serial, MQTT, WebSocket, CAN etc.
// Ele cuida apenas do FORMATO da mensagem.
//
// LEIA NESTA ORDEM:
//   [1] MESSAGE_TYPES   -> tipos permitidos
//   [2] createMessage   -> cria qualquer mensagem
//   [3] createAck       -> cria uma confirmacao ligada a outro comando
//   [4] createError     -> cria uma resposta de erro ligada a outro comando
//   [5] validateMessage -> verifica se uma mensagem tem o formato minimo
//
// Dica de busca: procure por "[1]", "[2]" etc. para navegar pelo arquivo.

// [1] TIPOS DE MENSAGEM
// Estes sao os tipos aceitos pela biblioteca. Se quiser adicionar um novo tipo,
// este e o primeiro ponto que deve ser alterado.
const MESSAGE_TYPES = new Set([
  'cmd',
  'ack',
  'error',
  'state',
  'telemetry',
  'event'
]);

// [2] CRIACAO DE MENSAGEM GENERICA
//
// Parametros principais:
// - type: tipo da mensagem, por exemplo 'cmd' ou 'telemetry'.
// - sequence: numero usado para identificar a mensagem.
// - source: quem gerou a mensagem.
// - timestamp: instante de criacao em ms.
// - payload: dados especificos da aplicacao.
//
// Retorna um objeto JavaScript pronto para serializar em JSON.
function createMessage({
  type,
  sequence = 0,
  source = 'device',
  timestamp = Date.now(),
  payload = {}
}) {
  // Bloqueia tipos desconhecidos logo na entrada. Isso evita que mensagens com
  // nomes diferentes circulem silenciosamente pelo sistema.
  if (!MESSAGE_TYPES.has(type)) {
    throw new Error(`unsupported message type: ${type}`);
  }

  // A sequencia precisa ser inteira e positiva ou zero porque ela sera usada
  // posteriormente para correlacionar comando e resposta.
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

// [3] CRIACAO DE ACK
// ACK = confirmacao de que uma mensagem anterior foi aceita.
//
// refSequence aponta para o sequence da mensagem original.
// Exemplo: comando sequence=10 -> ACK payload.ref_sequence=10.
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

// [4] CRIACAO DE ERRO
// Funciona como o ACK, mas informa que a operacao falhou.
//
// code: numero simples para a aplicacao tratar programaticamente.
// message: texto legivel para log, debug ou interface.
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

// [5] VALIDACAO
// Retorna true quando a estrutura minima esta correta e false quando ha algum
// problema. Esta funcao nao valida o significado dos dados dentro do payload;
// ela valida apenas o envelope comum.
function validateMessage(message) {
  // Validacoes comuns a todos os tipos de mensagem.
  if (!message || typeof message !== 'object') return false;
  if (!MESSAGE_TYPES.has(message.type)) return false;
  if (!Number.isInteger(message.sequence) || message.sequence < 0) return false;
  if (typeof message.source !== 'string' || message.source.length === 0) return false;
  if (!Number.isFinite(message.timestamp) || message.timestamp < 0) return false;
  if (!message.payload || typeof message.payload !== 'object' || Array.isArray(message.payload)) return false;

  // ACK e error precisam apontar para a mensagem que estao respondendo.
  if (message.type === 'ack' || message.type === 'error') {
    if (!Number.isInteger(message.payload.ref_sequence)) return false;
  }

  // Uma mensagem de erro precisa ter codigo e descricao minima.
  if (message.type === 'error') {
    if (!Number.isInteger(message.payload.code)) return false;
    if (typeof message.payload.message !== 'string' || message.payload.message.length === 0) return false;
  }

  return true;
}

// Exporta somente a API publica da feature.
module.exports = {
  MESSAGE_TYPES,
  createMessage,
  createAck,
  createError,
  validateMessage
};
