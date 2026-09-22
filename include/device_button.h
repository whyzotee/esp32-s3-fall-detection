#pragma once
#include <stdint.h>

namespace DeviceButton {
enum class Action { None, PowerOff, PowerOn, StartOta };
constexpr uint32_t powerOnMinMs = 1000;
constexpr uint32_t powerOnMaxMs = 4000;
constexpr uint32_t powerOffMinMs = 5000;
constexpr uint32_t powerOffMaxMs = 7000;
constexpr uint32_t otaMinMs = 8000;
constexpr uint32_t sosClickMaxMs = 1000;
constexpr uint32_t sosTripleClickWindowMs = 1500;
constexpr Action classify(bool off, uint32_t heldMs) {
    return off
        ? ((heldMs >= powerOnMinMs && heldMs <= powerOnMaxMs) ? Action::PowerOn : Action::None)
        : ((heldMs >= otaMinMs) ? Action::StartOta
           : ((heldMs >= powerOffMinMs && heldMs <= powerOffMaxMs) ? Action::PowerOff : Action::None));
}
constexpr bool isSosClick(uint32_t heldMs) { return heldMs < sosClickMaxMs; }
// Called before GPS/radio initialization. Rejects short presses while off.
void begin();
// Wait for release and apply pending power-off, outside radio transactions.
void service();
bool sosPending();
void acknowledgeSos();
bool otaPending();
void acknowledgeOta();
}
