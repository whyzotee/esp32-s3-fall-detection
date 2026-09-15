#pragma once

namespace TrackerApp {
// One wake cycle: acquire telemetry, exchange LoRaWAN data, then sleep.
[[noreturn]] void runCycle();
}
