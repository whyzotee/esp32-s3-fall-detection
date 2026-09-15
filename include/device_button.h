#pragma once
#include <stdint.h>

namespace DeviceButton {
enum class Action { None, Sos, PowerOff, PowerOn };
constexpr Action classify(bool off, uint32_t heldMs) {
    if (off) return heldMs >= 3000 ? Action::PowerOn : Action::None;
    return heldMs >= 5000 ? Action::PowerOff :
           heldMs >= 3000 ? Action::Sos : Action::None;
}
// Called before GPS/radio initialization. Rejects short presses while off.
void begin();
// Wait for release and apply pending power-off, outside radio transactions.
void service();
bool sosPending();
void acknowledgeSos();
}
