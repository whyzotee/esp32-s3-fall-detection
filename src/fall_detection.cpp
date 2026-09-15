#include <fall_detection.h>
#include <SPI.h>
#include <driver/rtc_io.h>

namespace {
SPIClass sensorBus(HSPI); // Dedicated SPI3; LoRa uses the global FSPI/SPI2 bus.
bool available = false;
RTC_DATA_ATTR bool pending = false;
uint32_t lastPoll = 0;
uint32_t lastLog = 0;
constexpr uint8_t STATUS = 0x0B;
constexpr uint8_t INACT = 0x20;
constexpr uint8_t REGISTER_ERROR = 0x80;

void readRegisters(uint8_t reg, uint8_t *data, size_t length)
{
    sensorBus.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(Board::accelCs, LOW);
    sensorBus.transfer(0x0B); // ADXL362 register-read command.
    sensorBus.transfer(reg);
    for (size_t i = 0; i < length; ++i) data[i] = sensorBus.transfer(0);
    digitalWrite(Board::accelCs, HIGH);
    sensorBus.endTransaction();
}

bool devicePresent()
{
    uint8_t id[3];
    readRegisters(0x00, id, sizeof(id));
    return id[0] == 0xAD && id[1] == 0x1D && id[2] == 0xF2;
}

bool writeRegister(uint8_t reg, uint8_t value)
{
    sensorBus.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(Board::accelCs, LOW);
    sensorBus.transfer(0x0A);
    sensorBus.transfer(reg);
    sensorBus.transfer(value);
    digitalWrite(Board::accelCs, HIGH);
    sensorBus.endTransaction();
    uint8_t actual = 0;
    readRegisters(reg, &actual, 1);
    return actual == value;
}

bool readInterrupt()
{
    // SPI has no ACK: check identity to catch common disconnected-bus faults.
    if (!devicePresent()) return false;
    uint8_t source = 0;
    readRegisters(STATUS, &source, 1); // Clears ACT/INACT hardware latches.
    if (source & INACT) {
        pending = true;
        Serial.println("[ADXL362] FREE_FALL: suspected fall, status=2 queued");
    }
    return !(source & REGISTER_ERROR);
}
}

bool setup_fall_detection()
{
    available = false;
    rtc_gpio_deinit(FALL_INT_PIN);
    pinMode(FALL_INT_PIN, INPUT_PULLDOWN);
    pinMode(Board::accelInt2, INPUT);
    pinMode(Board::accelCs, OUTPUT);
    digitalWrite(Board::accelCs, HIGH);
    sensorBus.begin(Board::accelSck, Board::accelMiso, Board::accelMosi, Board::accelCs);
    delay(5); // Power-on register access settling.
    if (!devicePresent()) {
        Serial.println("[ADXL362] Not found (expected AD 1D F2); check SPI wiring. Fall wake disabled");
        return false;
    }
    // Preserve a latched wake cause before standby/reconfiguration; never reset first.
    if (!readInterrupt() ||
        !writeRegister(0x2A, 0x00) || // INT1 off during configuration.
        !writeRegister(0x2B, 0x00) || // INT2 unused.
        !writeRegister(0x2D, 0x00) || // Standby.
        !writeRegister(0x27, 0x00) ||
        !writeRegister(0x28, 0x00) || // FIFO disabled.
        !writeRegister(0x2C, 0x03) || // +/-2g, 100 Hz, normal bandwidth.
        !writeRegister(0x23, FALL_THRESHOLD_MG & 0xFF) ||
        !writeRegister(0x24, FALL_THRESHOLD_MG >> 8) ||
        !writeRegister(0x25, FALL_DURATION_SAMPLES & 0xFF) ||
        !writeRegister(0x26, FALL_DURATION_SAMPLES >> 8) ||
        !writeRegister(0x27, 0x04) || // Absolute inactivity, default (not linked) mode.
        !readInterrupt() ||
        !writeRegister(0x2A, INACT) || // Active-high latched INT1.
        !writeRegister(0x2D, 0x02)) { // Continuous measurement; no autosleep.
        Serial.println("[ADXL362] Configuration/readback failed; fall wake disabled");
        return false;
    }
    available = true;
    Serial.printf("[ADXL362] Ready: CS=%d SCK=%d MISO=%d MOSI=%d INT1=%d, %umg/%ums\n",
                  Board::accelCs, Board::accelSck, Board::accelMiso, Board::accelMosi,
                  int(FALL_INT_PIN), FALL_THRESHOLD_MG, FALL_DURATION_SAMPLES * 10);
    return true;
}

void update_fall_detection(bool debug)
{
    if (!available || uint32_t(millis() - lastPoll) < 20) return;
    lastPoll = millis();
    if (!readInterrupt()) {
        if (uint32_t(millis() - lastLog) >= 1000) {
            lastLog = millis();
            Serial.println("[ADXL362] SPI identity/register error");
        }
        return;
    }
    if (debug && uint32_t(millis() - lastLog) >= 1000) {
        lastLog = millis();
        uint8_t data[6];
        readRegisters(0x0E, data, sizeof(data));
        int16_t x = uint16_t(data[0]) | uint16_t(data[1]) << 8;
        int16_t y = uint16_t(data[2]) | uint16_t(data[3]) << 8;
        int16_t z = uint16_t(data[4]) | uint16_t(data[5]) << 8;
        Serial.printf("[ADXL362] x=%.3fg y=%.3fg z=%.3fg pending=%d\n",
                      x * 0.001, y * 0.001, z * 0.001, pending);
    }
}

bool fall_detection_pending() { return pending; }
void acknowledge_fall_detection() { pending = false; }

bool prepare_fall_detection_sleep()
{
    if (!available) return true;
    if (!readInterrupt() || pending) return false;
    // R10 pulls CS high while asleep; sensor remains on always-powered 3V3.
    // Do not read STATUS again after arming: a new event must wake the CPU.
    rtc_gpio_pulldown_en(FALL_INT_PIN);
    rtc_gpio_pullup_dis(FALL_INT_PIN);
    return esp_sleep_enable_ext1_wakeup(1ULL << FALL_INT_PIN, ESP_EXT1_WAKEUP_ANY_HIGH) == ESP_OK;
}
