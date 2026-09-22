// Host test for release-time decisions; GPIO timing requires hardware testing.
const fs = require('fs');
const cp = require('child_process');
const os = require('os');
const path = require('path');
const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'tracker-button-'));
try {
    const binary = path.join(dir, 'test');
    // ESP32 Arduino 2.x uses a C++11-era toolchain, where constexpr function
    // bodies must contain a single return statement.
    cp.execFileSync('c++', ['-std=c++11', '-Iinclude', '-x', 'c++', '-', '-o', binary], {
        input: `
#include <device_button.h>
using namespace DeviceButton;
static_assert(classify(false, 0) == Action::None, "idle");
static_assert(classify(false, 4999) == Action::None, "power-off lower boundary");
static_assert(classify(false, 5000) == Action::PowerOff, "power-off lower boundary");
static_assert(classify(false, 7000) == Action::PowerOff, "power-off upper boundary");
static_assert(classify(false, 7001) == Action::None, "gap before OTA");
static_assert(classify(false, 7999) == Action::None, "OTA lower boundary");
static_assert(classify(false, 8000) == Action::StartOta, "OTA lower boundary");
static_assert(classify(false, 12000) == Action::StartOta, "overlong OTA hold");
static_assert(classify(true, 0) == Action::None, "off idle");
static_assert(classify(true, 999) == Action::None, "power-on lower boundary");
static_assert(classify(true, 1000) == Action::PowerOn, "power-on lower boundary");
static_assert(classify(true, 4000) == Action::PowerOn, "power-on upper boundary");
static_assert(classify(true, 4001) == Action::None, "overlong power-on ignored");
static_assert(isSosClick(999), "short click");
static_assert(!isSosClick(1000), "one-second press is not a click");
int main() {}
` });
    cp.execFileSync(binary);
    console.log('Button triple-SOS/off/on hold boundary tests passed');
} finally {
    fs.rmSync(dir, { recursive: true, force: true });
}
