#pragma once

// Keep all values in one file so the feature can be copied as a complete unit
// and adapted without searching through the firmware source.

#ifndef BRIDGE_BAUDRATE
#define BRIDGE_BAUDRATE 115200
#endif

#ifndef BRIDGE_HEARTBEAT_MS
#define BRIDGE_HEARTBEAT_MS 1000
#endif

// ESP-NOW has a small frame size. 200 bytes keeps the complete packet below
// the classic 250-byte payload limit and leaves room for the packet header.
#ifndef BRIDGE_MAX_JSON_BYTES
#define BRIDGE_MAX_JSON_BYTES 200
#endif

#define BRIDGE_PACKET_MAGIC 0x4842
#define BRIDGE_PACKET_VERSION 1

#ifndef BRIDGE_ROLE_NAME
#define BRIDGE_ROLE_NAME "BRIDGE"
#endif
