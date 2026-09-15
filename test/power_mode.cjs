// Host test: node test/power_mode.cjs (requires c++).
const fs = require('fs');
const cp = require('child_process');
const os = require('os');
const path = require('path');
const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'tracker-power-'));
const root = path.resolve(__dirname, '..');
try {
    fs.writeFileSync(path.join(dir, 'esp_attr.h'), '#define RTC_DATA_ATTR\n');
    fs.writeFileSync(path.join(dir, 'test.cpp'), `
#include <cassert>
#include "power_mode.cpp"
int main() {
    const uint8_t enable[] = {1};
    const uint8_t unknown[] = {0};
    const uint8_t extra[] = {1, 0};
    assert(!PowerMode::enabled());
    assert(PowerMode::reportInterval() == 900000);
    assert(!PowerMode::handleDownlink(2, enable, 1));
    assert(!PowerMode::handleDownlink(10, unknown, 1));
    assert(!PowerMode::handleDownlink(10, extra, 2));
    assert(!PowerMode::handleDownlink(10, enable, 0));
    assert(!PowerMode::handleDownlink(10, nullptr, 1));
    assert(!PowerMode::enabled());
    assert(PowerMode::handleDownlink(10, enable, 1));
    assert(PowerMode::enabled());
    assert(PowerMode::reportInterval() == 3600000);
    assert(PowerMode::handleDownlink(10, enable, 1));
    assert(!PowerMode::handleDownlink(10, unknown, 1));
    assert(PowerMode::reportInterval() == 3600000);
    // Simulate a cold boot reinitializing RTC data.
    lowPowerMarker = 0;
    assert(PowerMode::reportInterval() == 900000);
}
`);
    const binary = path.join(dir, 'test');
    cp.execFileSync('c++', ['-std=c++17', `-I${dir}`,
        `-I${path.join(root, 'include')}`, `-I${path.join(root, 'src')}`,
        path.join(dir, 'test.cpp'), '-o', binary]);
    cp.execFileSync(binary);
    console.log('Low Power command validation and interval tests passed');
} finally {
    fs.rmSync(dir, { recursive: true, force: true });
}
