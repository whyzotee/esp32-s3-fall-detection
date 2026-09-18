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
static_assert(classify(false, 2999) == Action::None, "short press");
static_assert(classify(false, 3000) == Action::Sos, "SOS lower boundary");
static_assert(classify(false, 4999) == Action::Sos, "SOS upper boundary");
static_assert(classify(false, 5000) == Action::PowerOff, "power-off lower boundary");
static_assert(classify(false, 7000) == Action::PowerOff, "power-off hold");
static_assert(classify(false, 10000) == Action::PowerOff, "long power-off hold");
static_assert(classify(true, 0) == Action::None, "off idle");
static_assert(classify(true, 2999) == Action::None, "off short press");
static_assert(classify(true, 3000) == Action::PowerOn, "power-on lower boundary");
static_assert(classify(true, 7000) == Action::PowerOn, "power-on hold");
int main() {}
` });
    cp.execFileSync(binary);
    console.log('Button SOS/off/on hold boundary tests passed');
} finally {
    fs.rmSync(dir, { recursive: true, force: true });
}
