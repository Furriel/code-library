#pragma once

// CONFIGURACAO DO BRIDGE SERIAL <-> ESP-NOW
// -----------------------------------------------------------------------------
// Este arquivo concentra os valores que normalmente voce vai querer alterar.
// A ideia e evitar procurar constantes espalhadas pelo main.cpp.
//
// PROCURE AQUI PRIMEIRO quando quiser mudar:
//   [1] velocidade da Serial
//   [2] intervalo do heartbeat
//   [3] tamanho maximo do JSON
//   [4] identificacao do pacote ESP-NOW
//   [5] nome/papel deste bridge

// [1] SERIAL
// Velocidade usada entre o ESP32 e o computador/dispositivo conectado por USB
// ou UART. O monitor serial precisa usar exatamente o mesmo valor.
#ifndef BRIDGE_BAUDRATE
#define BRIDGE_BAUDRATE 115200
#endif

// [2] HEARTBEAT
// Intervalo em milissegundos entre mensagens automaticas de vida do bridge.
// 1000 ms = uma mensagem por segundo.
#ifndef BRIDGE_HEARTBEAT_MS
#define BRIDGE_HEARTBEAT_MS 1000
#endif

// [3] TAMANHO MAXIMO DO JSON
// ESP-NOW possui um frame pequeno. Usamos 200 bytes para manter o pacote inteiro
// abaixo do limite classico de 250 bytes, deixando espaco para o cabecalho.
//
// Se precisar de mensagens maiores, prefira implementar fragmentacao em vez de
// simplesmente aumentar este valor sem verificar o limite do hardware/framework.
#ifndef BRIDGE_MAX_JSON_BYTES
#define BRIDGE_MAX_JSON_BYTES 200
#endif

// [4] IDENTIFICACAO DO PACOTE
// magic: assinatura simples usada para reconhecer frames desta feature.
// version: permite rejeitar no futuro uma estrutura incompatível.
#define BRIDGE_PACKET_MAGIC 0x4842
#define BRIDGE_PACKET_VERSION 1

// [5] PAPEL / NOME DO BRIDGE
// PlatformIO sobrescreve este valor nos ambientes bridge_a e bridge_b.
// Se compilar sem essa definicao, o nome generico abaixo sera utilizado.
#ifndef BRIDGE_ROLE_NAME
#define BRIDGE_ROLE_NAME "BRIDGE"
#endif
