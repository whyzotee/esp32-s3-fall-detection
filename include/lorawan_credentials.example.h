#pragma once
#include <stdint.h>

// Copy to lorawan_credentials.h (gitignored). Never commit real root keys.
// EUIs: hexadecimal numbers in the SAME displayed order as ChirpStack (not reversed).
namespace LoRaCredentials {
constexpr uint64_t joinEui = 0x0000000000000000ULL;
constexpr uint64_t devEui = 0x0000000000000000ULL; // Set your assigned 16-digit DevEUI.
// LoRaWAN 1.0.4 AppKey: 16 bytes in displayed order. NOT the old ABP AppSKey.
constexpr uint8_t appKey[16] = {0};
}
