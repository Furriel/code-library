# I2C Bus Recovery

Rotina pequena para tentar liberar um barramento I2C quando um dispositivo deixa `SDA` preso em nivel baixo apos uma interrupcao ou reset no meio da comunicacao.

A feature e independente e nao depende de nenhuma outra pasta deste repositorio.

## O que ela faz

1. libera SDA e SCL como entradas com pull-up;
2. verifica se o clock esta preso;
3. se SDA estiver baixo, gera ate 9 pulsos em SCL;
4. gera uma condicao STOP;
5. verifica novamente as duas linhas;
6. retorna um resultado explicito.

## Ambiente

- MCU: ESP32
- Framework: Arduino
- IDE recomendada: VS Code + PlatformIO
- Hardware: qualquer barramento I2C com pull-ups adequados

## Getting Started - 2 minutos

```bash
cd features/i2c-bus-recovery
pio run
```

Para testar em uma placa:

```bash
pio run -t upload
pio device monitor -b 115200
```

O exemplo usa SDA 21 e SCL 22.

## Exemplo minimo

```cpp
#include "i2c_bus_recovery.h"

I2cRecoveryResult result = recoverI2cBus(21, 22);

if (result == I2cRecoveryResult::Recovered) {
  Serial.println("I2C recovered");
}
```

Depois da recuperacao, inicialize normalmente o `Wire`:

```cpp
Wire.begin(21, 22);
```

## Resultados

| Resultado | Significado |
|---|---|
| `BusOk` | SDA e SCL ja estavam livres |
| `Recovered` | SDA estava presa e foi liberada |
| `ClockStuckLow` | SCL continua baixa; nao e possivel gerar pulsos |
| `StillStuck` | os pulsos/STOP nao liberaram o barramento |
| `InvalidPins` | pinos invalidos |

## Onde mexer no codigo

Abra `src/i2c_bus_recovery.cpp`:

| Quero alterar... | Procure por |
|---|---|
| leitura inicial do barramento | `[1] ESTADO INICIAL` |
| quantidade de pulsos | `[2] PULSOS DE CLOCK` |
| geracao de STOP | `[3] CONDICAO STOP` |
| verificacao final | `[4] RESULTADO FINAL` |

No header, procure por `[1] CONFIGURACAO` para alterar `clock_pulses`, tempos e pull-up.

## Quando usar

Use preferencialmente antes de `Wire.begin()` durante o boot, ou depois de encerrar/reinicializar o driver I2C da aplicacao.

Nao execute esta rotina enquanto outra task estiver usando o mesmo barramento.

## O que esta rotina nao resolve

- curto eletrico em SDA ou SCL;
- dispositivo sem alimentacao correta;
- pull-up ausente ou inadequado;
- slave mantendo SCL baixo indefinidamente;
- defeito fisico no barramento.

## Status de validacao

- logica: implementada de forma independente;
- compilacao: validada pelo CI quando o PR correspondente passa;
- hardware: pendente de teste desta versao generalizada com barramento propositalmente travado.
