#include <Arduino.h>
#include <Wire.h>

#include "i2c_bus_recovery.h"

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;

void setup() {
  Serial.begin(115200);

  const I2cRecoveryResult result = recoverI2cBus(SDA_PIN, SCL_PIN);
  Serial.print("I2C recovery result: ");
  Serial.println(i2cRecoveryResultName(result));

  // Inicialize o driver somente depois de a rotina liberar os GPIOs.
  Wire.begin(SDA_PIN, SCL_PIN);
}

void loop() {
  delay(1000);
}
