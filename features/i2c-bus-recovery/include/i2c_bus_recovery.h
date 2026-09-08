#pragma once

#include <Arduino.h>

/*
 * I2C BUS RECOVERY
 * ================
 *
 * Objetivo:
 *   Recuperar o caso comum em que um slave fica com SDA em LOW porque a
 *   comunicacao foi interrompida no meio de um byte.
 *
 * LEIA NESTA ORDEM:
 *   [1] CONFIGURACAO
 *   [2] RESULTADOS
 *   [3] FUNCAO PRINCIPAL
 */

// [1] CONFIGURACAO
// ----------------
struct I2cRecoveryConfig {
  uint8_t clock_pulses = 9;
  uint16_t pulse_low_us = 5;
  uint16_t pulse_high_us = 5;
  uint16_t settle_us = 10;
  bool use_internal_pullups = true;
  bool generate_stop = true;
};

// [2] RESULTADOS
// --------------
enum class I2cRecoveryResult {
  BusOk,
  Recovered,
  ClockStuckLow,
  StillStuck,
  InvalidPins,
};

// [3] FUNCAO PRINCIPAL
// --------------------
// Chame antes de Wire.begin() ou quando nenhum outro codigo estiver dirigindo
// o mesmo barramento.
I2cRecoveryResult recoverI2cBus(
    int sda_pin,
    int scl_pin,
    const I2cRecoveryConfig &config = I2cRecoveryConfig());

const char *i2cRecoveryResultName(I2cRecoveryResult result);
