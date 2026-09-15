// Host regression of the actual ADXL362 driver against a register-level SPI mock.
const fs = require('fs');
const cp = require('child_process');
const os = require('os');
const path = require('path');
const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'adxl362-test-'));
const root = path.resolve('.');
fs.mkdirSync(path.join(dir, 'driver'));
fs.writeFileSync(path.join(dir, 'Arduino.h'), `
#pragma once
#include <cstdint>
#include <cstddef>
#include <cassert>
#define RTC_DATA_ATTR
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLDOWN 2
#define LOW 0
#define HIGH 1
using gpio_num_t = int;
constexpr int GPIO_NUM_0=0, GPIO_NUM_7=7;
constexpr int ESP_OK=0, ESP_EXT1_WAKEUP_ANY_HIGH=1;
inline uint32_t now=0;
inline uint64_t wakeMask=0;
inline int wakeResult=0;
inline void pinMode(int,int) {}
inline void digitalWrite(int,int) {}
inline uint32_t millis() { return now; }
inline void delay(int n) { now+=n; }
inline int esp_sleep_enable_ext1_wakeup(uint64_t mask,int mode) {
    assert(mode==ESP_EXT1_WAKEUP_ANY_HIGH); wakeMask=mask; return wakeResult;
}
inline struct { void println(const char*) {} template<class... T> void printf(const char*,T...) {} } Serial;
`);
fs.writeFileSync(path.join(dir, 'driver/rtc_io.h'), `
inline void rtc_gpio_deinit(int) {}
inline void rtc_gpio_pulldown_en(int) {}
inline void rtc_gpio_pullup_dis(int) {}
`);
fs.writeFileSync(path.join(dir, 'SPI.h'), `
#pragma once
#include <Arduino.h>
#define HSPI 1
#define MSBFIRST 1
#define SPI_MODE0 0
struct SPISettings { SPISettings(int speed,int order,int mode) {
    assert(speed==1000000 && order==MSBFIRST && mode==SPI_MODE0);
} };
inline uint8_t regs[256]={};
inline bool disconnected=false;
inline int failWrite=-1;
inline int statusReads=0;
class SPIClass {
    int phase=0; uint8_t command=0, address=0;
public:
    SPIClass(int bus) { assert(bus==HSPI); }
    void begin(int sck,int miso,int mosi,int cs) {
        assert(sck==37 && miso==34 && mosi==35 && cs==33);
    }
    void beginTransaction(SPISettings) { phase=0; }
    void endTransaction() {}
    uint8_t transfer(uint8_t value) {
        if(phase++==0) { command=value; return 0; }
        if(phase==2) { address=value; return 0; }
        if(disconnected) return 0xFF;
        if(command==0x0A) { if(address!=failWrite) regs[address]=value; ++address; return 0; }
        assert(command==0x0B);
        uint8_t result=regs[address];
        if(address==0x0B) { ++statusReads; regs[address]&=~0x30; }
        ++address; return result;
    }
};
`);
fs.writeFileSync(path.join(dir, 'test.cpp'), `
#include "${root}/src/fall_detection.cpp"
void resetSensor() {
    for(auto &r:regs) r=0;
    regs[0]=0xAD; regs[1]=0x1D; regs[2]=0xF2;
    disconnected=false; failWrite=-1; pending=false; wakeMask=0; wakeResult=0;
}
int main() {
    resetSensor();
    regs[0x0B]=0x20; // Event latched before wake/reconfiguration must survive.
    assert(setup_fall_detection());
    assert(fall_detection_pending());
    assert(regs[0x23]==0x77 && regs[0x24]==0x01); // 375 mg at +/-2g.
    assert(regs[0x25]==15 && regs[0x26]==0);
    assert(regs[0x27]==4 && regs[0x2A]==0x20 && regs[0x2B]==0);
    assert(regs[0x2C]==3 && regs[0x2D]==2);
    assert(!prepare_fall_detection_sleep());
    acknowledge_fall_detection();
    assert(prepare_fall_detection_sleep());
    assert(wakeMask==(1ULL<<7));
    regs[0x0B]=0x20; // Event during blocking TX/GPS.
    assert(!prepare_fall_detection_sleep());
    assert(fall_detection_pending());
    acknowledge_fall_detection();
    disconnected=true;
    assert(!prepare_fall_detection_sleep());
    disconnected=false;
    regs[0x0B]=0x80; // Register protection error.
    assert(!prepare_fall_detection_sleep());
    regs[0x0B]=0;
    wakeResult=-1;
    assert(!prepare_fall_detection_sleep());
    resetSensor(); failWrite=0x2A;
    assert(!setup_fall_detection());
    resetSensor(); disconnected=true;
    assert(!setup_fall_detection());
    assert(prepare_fall_detection_sleep()); // Missing sensor: no wake armed.
    assert(wakeMask==0);
    resetSensor(); assert(setup_fall_detection());
    regs[0x0B]=0x20; now+=1000;
    update_fall_detection(true);
    assert(fall_detection_pending());
}
`);
try {
    cp.execFileSync('c++', ['-std=c++17', '-I'+dir, '-I'+path.join(root,'include'),
        path.join(dir,'test.cpp'), '-o', path.join(dir,'test')], {stdio:'inherit'});
    cp.execFileSync(path.join(dir,'test'), [], {stdio:'inherit'});
    console.log('ADXL362 SPI/configuration/latch/sleep tests passed');
} finally {
    fs.rmSync(dir, {recursive:true, force:true});
}
