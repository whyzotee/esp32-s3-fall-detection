#include <fall_detection.h>
#include <SPI.h>
#include <audio_feedback.h>
#include <driver/rtc_io.h>
#include <math.h>

namespace {
SPIClass sensorBus(HSPI); // Dedicated SPI3; LoRa uses the global FSPI/SPI2 bus.
bool available = false;
bool diagnosticMode = false;
RTC_DATA_ATTR bool pending = false;
uint32_t lastPoll = 0;
uint32_t lastLog = 0;
bool fallBuzzerPlayed = false;
constexpr uint8_t STATUS = 0x0B;
constexpr uint8_t INACT = 0x20;
constexpr uint8_t REGISTER_ERROR = 0x80;
constexpr uint32_t DEBUG_SAMPLE_INTERVAL_MS = 250;

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
    if (diagnosticMode && !available) {
        Serial.printf("[FALL DEBUG] Device ID: %02X %02X %02X (expected AD 1D F2)\n",
                      id[0], id[1], id[2]);
    }
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
    if (diagnosticMode && actual != value) {
        Serial.printf("[FALL DEBUG] Register 0x%02X write failed: expected=0x%02X actual=0x%02X\n",
                      reg, value, actual);
    }
    return actual == value;
}

bool readInterrupt(bool allowUnconfigured = false, bool verifyDevice = true)
{
    // SPI has no ACK: check identity to catch common disconnected-bus faults.
    if (verifyDevice && !devicePresent()) return false;
    uint8_t source = 0;
    readRegisters(STATUS, &source, 1); // Clears ACT/INACT hardware latches.
    if (source & INACT) {
        pending = true;
        Serial.println("[ADXL362] FREE_FALL: suspected fall, status=2 queued");
    }
    if (source & REGISTER_ERROR) {
        if (diagnosticMode) {
            Serial.printf("[FALL DEBUG] STATUS=0x%02X ERR_USER_REGS=1 (%s)\n",
                          source, allowUnconfigured ? "expected before configuration" : "unexpected");
        }
        return allowUnconfigured;
    }
    return true;
}
}

bool setup_fall_detection(bool debug)
{
    diagnosticMode = debug;
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
    // ERR_USER_REGS is expected at startup until the first protected-register write.
    if (!readInterrupt(true, false) ||
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
        !readInterrupt(false, false) ||
        !writeRegister(0x2A, INACT) || // Active-high latched INT1.
        !writeRegister(0x2D, 0x02)) { // Continuous measurement; no autosleep.
        Serial.println("[ADXL362] Configuration/readback failed; fall wake disabled");
        return false;
    }
    available = true;
    Serial.printf("[ADXL362] Ready: CS=%d SCK=%d MISO=%d MOSI=%d INT1=%d, %umg/%ums\n",
                  Board::accelCs, Board::accelSck, Board::accelMiso, Board::accelMosi,
                  int(FALL_INT_PIN), FALL_THRESHOLD_MG, FALL_DURATION_SAMPLES * 10);
    if (diagnosticMode) {
        Serial.println("[FALL DEBUG] Real deep-sleep wake, status=2 uplink and buzzer enabled");
        Serial.println("[FALL DEBUG] Use a padded fixture; free fall must stay below the threshold for 150 ms");
    }
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
    if (debug && uint32_t(millis() - lastLog) >= DEBUG_SAMPLE_INTERVAL_MS) {
        lastLog = millis();
        uint8_t data[6];
        readRegisters(0x0E, data, sizeof(data));
        int16_t x = uint16_t(data[0]) | uint16_t(data[1]) << 8;
        int16_t y = uint16_t(data[2]) | uint16_t(data[3]) << 8;
        int16_t z = uint16_t(data[4]) | uint16_t(data[5]) << 8;
        const float magnitude = sqrtf(float(x) * x + float(y) * y + float(z) * z) * 0.001f;
        Serial.printf("[FALL DEBUG] x=%+.3fg y=%+.3fg z=%+.3fg |a|=%.3fg INT1=%d pending=%s\n",
                      x * 0.001, y * 0.001, z * 0.001, magnitude,
                      digitalRead(FALL_INT_PIN), pending ? "YES" : "NO");
    }

    if (pending && !fallBuzzerPlayed) {
        fallBuzzerPlayed = true;
        AudioFeedback::play(AudioFeedback::Event::Fall);
        Serial.println("[FALL] EVENT DETECTED; alert tone active");
    }
}

bool fall_detection_pending() { return pending; }
void acknowledge_fall_detection()
{
    pending = false;
    fallBuzzerPlayed = false;
}

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
