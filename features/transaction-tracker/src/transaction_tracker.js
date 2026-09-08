// TRANSACTION TRACKER
// -----------------------------------------------------------------------------
// Objetivo: descobrir qual resposta pertence a qual comando e quanto tempo a
// resposta demorou para chegar.
//
// Conceito principal:
//   comando sequence=25  --->  resposta payload.ref_sequence=25
//
// LEIA NESTA ORDEM:
//   [1] constructor     -> memoria interna das transacoes
//   [2] start           -> registra um comando enviado
//   [3] resolve         -> encerra a transacao e calcula latencia
//   [4] handleEnvelope  -> interpreta ACK ou erro recebido
//   [5] get/list/clear  -> consulta e manutencao
//
// Este codigo NAO envia mensagens. Ele apenas acompanha o estado delas.

class TransactionTracker {
  // [1] MEMORIA INTERNA
  // Map usa o numero de sequencia como chave. Isso deixa a consulta direta:
  // sequence=25 -> transacao correspondente.
  constructor() {
    this._items = new Map();
  }

  // [2] INICIO DE UMA TRANSACAO
  // Chame esta funcao imediatamente antes ou depois de enviar um comando.
  //
  // sequence: identificador unico do comando.
  // command: nome legivel da operacao, usado principalmente para debug/log.
  // sentAtMs: instante do envio; normalmente Date.now().
  //
  // Retorna uma copia do registro criado.
  start(sequence, command, sentAtMs = Date.now()) {
    if (!Number.isInteger(sequence) || sequence < 0) {
      throw new Error('sequence must be a non-negative integer');
    }
    if (!command) {
      throw new Error('command is required');
    }

    // Reutilizar uma sequencia ainda registrada criaria ambiguidade: duas
    // operacoes diferentes poderiam receber a mesma resposta.
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

  // [3] FINALIZACAO
  // Atualiza uma transacao existente quando sua resposta chega.
  //
  // status normalmente sera 'ack' ou 'error'.
  // receivedAtMs permite testar a latencia com tempos controlados.
  resolve(sequence, status, receivedAtMs = Date.now(), error = null) {
    const transaction = this._items.get(sequence);

    // Retorna null quando recebemos uma resposta para uma sequencia que nao
    // esta sendo acompanhada.
    if (!transaction) return null;

    transaction.status = status;
    transaction.received_at_ms = receivedAtMs;
    transaction.latency_ms = Math.max(0, receivedAtMs - transaction.sent_at_ms);
    transaction.error = error;

    return { ...transaction };
  }

  // [4] ENTRADA DE RESPOSTAS
  // Passe aqui o envelope recebido da rede/Serial/etc.
  // A funcao procura payload.ref_sequence e decide se e ACK ou erro.
  //
  // Apenas estes campos sao realmente necessarios:
  //   { type: 'ack', payload: { ref_sequence: 25 } }
  // ou
  //   { type: 'error', payload: { ref_sequence: 25, code: 1, message: '...' } }
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

    // Telemetria, estado e outros tipos nao encerram uma transacao.
    return null;
  }

  // [5] CONSULTAS
  // get(sequence): retorna uma transacao especifica.
  get(sequence) {
    const item = this._items.get(sequence);
    return item ? { ...item } : null;
  }

  // list(): retorna todas as transacoes registradas.
  list() {
    return Array.from(this._items.values(), (item) => ({ ...item }));
  }

  // clear(): apaga o historico mantido em memoria.
  clear() {
    this._items.clear();
  }
}

module.exports = { TransactionTracker };
