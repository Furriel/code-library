#include "i2c_bus_recovery.h"

/*
 * OBJETIVO DESTE ARQUIVO
 * ----------------------
 * Executar uma sequencia curta de recuperacao eletrica do barramento I2C.
 *
 * FLUXO:
 *   libera linhas -> verifica SCL -> verifica SDA -> pulsa SCL -> STOP -> testa
 *
 * LEIA NESTA ORDEM:
 *   [1] ESTADO INICIAL
 *   [2] PULSOS DE CLOCK
 *   [3] CONDICAO STOP
 *   [4] RESULTADO FINAL
 */

namespace {

void releaseLine(int pin, bool pullup) {
  pinMode(pin, pullup ? INPUT_PULLUP : INPUT);
}

void driveLow(int pin) {
  pinMode(pin, OUTPUT_OPEN_DRAIN);
  digitalWrite(pin, LOW);
}

}  // namespace

I2cRecoveryResult recoverI2cBus(int sda_pin,
                                int scl_pin,
                                const I2cRecoveryConfig &config) {
  if (sda_pin < 0 || scl_pin < 0 || sda_pin == scl_pin || config.clock_pulses == 0) {
    return I2cRecoveryResult::InvalidPins;
  }

  // [1] ESTADO INICIAL
  // ------------------
  // Primeiro deixamos as duas linhas em alta impedancia. Em I2C o nivel HIGH
  // deve vir do resistor de pull-up, nao de um push-pull forcando a linha.
  releaseLine(sda_pin, config.use_internal_pullups);
  releaseLine(scl_pin, config.use_internal_pullups);
  delayMicroseconds(config.settle_us);

  if (digitalRead(scl_pin) == LOW) {
    // Se o slave segura o proprio clock em LOW, nao conseguimos gerar pulsos
    // adicionais sem antes resolver a causa eletrica/funcional.
    return I2cRecoveryResult::ClockStuckLow;
  }

  if (digitalRead(sda_pin) == HIGH) {
    return I2cRecoveryResult::BusOk;
  }

  // [2] PULSOS DE CLOCK
  // -------------------
  // Nove pulsos cobrem um byte de oito bits mais o ciclo de ACK. Um slave que
  // ficou esperando clocks pode assim terminar o byte e liberar SDA.
  for (uint8_t pulse = 0; pulse < config.clock_pulses; ++pulse) {
    driveLow(scl_pin);
    delayMicroseconds(config.pulse_low_us);

    releaseLine(scl_pin, config.use_internal_pullups);
    delayMicroseconds(config.pulse_high_us);

    if (digitalRead(scl_pin) == LOW) {
      return I2cRecoveryResult::ClockStuckLow;
    }

    if (digitalRead(sda_pin) == HIGH) break;
  }

  // [3] CONDICAO STOP
  // -----------------
  // STOP em I2C: SDA sobe enquanto SCL esta HIGH. Fazemos isso explicitamente
  // para deixar o slave em um estado de barramento conhecido.
  if (config.generate_stop) {
    driveLow(sda_pin);
    delayMicroseconds(config.settle_us);

    releaseLine(scl_pin, config.use_internal_pullups);
    delayMicroseconds(config.settle_us);

    releaseLine(sda_pin, config.use_internal_pullups);
    delayMicroseconds(config.settle_us);
  }

  // [4] RESULTADO FINAL
  // -------------------
  const bool scl_high = digitalRead(scl_pin) == HIGH;
  const bool sda_high = digitalRead(sda_pin) == HIGH;

  if (!scl_high) return I2cRecoveryResult::ClockStuckLow;
  return sda_high ? I2cRecoveryResult::Recovered : I2cRecoveryResult::StillStuck;
}

const char *i2cRecoveryResultName(I2cRecoveryResult result) {
  switch (result) {
    case I2cRecoveryResult::BusOk: return "bus_ok";
    case I2cRecoveryResult::Recovered: return "recovered";
    case I2cRecoveryResult::ClockStuckLow: return "clock_stuck_low";
    case I2cRecoveryResult::StillStuck: return "still_stuck";
    case I2cRecoveryResult::InvalidPins: return "invalid_pins";
  }
  return "unknown";
}
