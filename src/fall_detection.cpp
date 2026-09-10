#include <fall_detection.h>
#include <Wire.h>
#include <driver/rtc_io.h>

namespace
{
TwoWire sensorBus(1); // OLED uses Wire (controller 0).
uint8_t address = 0x53;
bool available = false;
RTC_DATA_ATTR bool pending = false;
uint32_t lastPoll = 0;
uint32_t lastLog = 0;
constexpr uint8_t FREE_FALL = 0x04;

bool readRegisters(uint8_t reg, uint8_t *data, uint8_t length)
{
    sensorBus.beginTransmission(address);
    sensorBus.write(reg);
    if (sensorBus.endTransmission(false) != 0 ||
        sensorBus.requestFrom(address, length) != length)
        return false;
    for (uint8_t i = 0; i < length; ++i)
        data[i] = sensorBus.read();
    return true;
}

bool writeRegister(uint8_t reg, uint8_t value)
{
    sensorBus.beginTransmission(address);
    sensorBus.write(reg);
    sensorBus.write(value);
    return sensorBus.endTransmission() == 0;
}

bool readInterrupt()
{
    uint8_t source = 0;
    if (!readRegisters(0x30, &source, 1))
        return false;
    // INT_SOURCE clears the hardware latch; keep the event until TX succeeds.
    if (source & FREE_FALL)
    {
        pending = true;
        Serial.println("[ADXL345] FREE_FALL: suspected fall, status=2 queued");
    }
    return true;
}
}

bool setup_fall_detection()
{
    rtc_gpio_deinit(FALL_INT_PIN);
    pinMode(FALL_INT_PIN, INPUT_PULLDOWN);
    if (!sensorBus.begin(ADXL345_SDA_PIN, ADXL345_SCL_PIN, 100000))
        return false;
    sensorBus.setTimeOut(25);
    uint8_t id = 0;
    if (!readRegisters(0x00, &id, 1) || id != 0xE5)
    {
        address = 0x1D;
        if (!readRegisters(0x00, &id, 1) || id != 0xE5)
        {
            Serial.println("[ADXL345] Not found at 0x53/0x1D; check wiring. Fall wake disabled");
            return false;
        }
    }
    // Capture the latched cause BEFORE reconfiguration on a deep-sleep wake.
    if (!readInterrupt() ||
        !writeRegister(0x2E, 0x00) || // Disable interrupts during configuration.
        !writeRegister(0x2D, 0x00) || // Standby for configuration.
        !writeRegister(0x31, 0x0B) || // Full resolution, +/-16g, active-high INT.
        !writeRegister(0x2C, 0x0A) || // 100 Hz, normal power.
        !writeRegister(0x38, 0x00) || // FIFO bypass.
        !writeRegister(0x28, FALL_THRESHOLD) ||
        !writeRegister(0x29, FALL_DURATION) ||
        !writeRegister(0x2F, 0x00) || // FREE_FALL mapped to INT1.
        !readInterrupt() ||
        !writeRegister(0x2E, FREE_FALL) ||
        !writeRegister(0x2D, 0x08)) // Measurement remains ON during ESP32 sleep.
    {
        Serial.println("[ADXL345] Configuration failed; fall wake disabled");
        return false;
    }
    available = true;
    Serial.printf("[ADXL345] Ready at 0x%02X: SDA=%d SCL=%d INT1=%d, threshold=%.3fg duration=%ums\n",
                  address, ADXL345_SDA_PIN, ADXL345_SCL_PIN, int(FALL_INT_PIN),
                  FALL_THRESHOLD * 0.0625, FALL_DURATION * 5);
    return true;
}

void update_fall_detection(bool debug)
{
    if (!available || uint32_t(millis() - lastPoll) < 20)
        return;
    lastPoll = millis();
    if (!readInterrupt())
    {
        if (uint32_t(millis() - lastLog) >= 1000)
        {
            lastLog = millis();
            Serial.println("[ADXL345] I2C read failed");
        }
        return;
    }
    if (debug && uint32_t(millis() - lastLog) >= 1000)
    {
        lastLog = millis();
        uint8_t data[6];
        if (readRegisters(0x32, data, sizeof(data)))
        {
            int16_t x = uint16_t(data[0]) | uint16_t(data[1]) << 8;
            int16_t y = uint16_t(data[2]) | uint16_t(data[3]) << 8;
            int16_t z = uint16_t(data[4]) | uint16_t(data[5]) << 8;
            Serial.printf("[ADXL345] x=%.3fg y=%.3fg z=%.3fg pending=%d\n",
                          x * 0.0039, y * 0.0039, z * 0.0039, pending);
        }
    }
}

bool fall_detection_pending() { return pending; }
void acknowledge_fall_detection() { pending = false; }

bool prepare_fall_detection_sleep()
{
    if (!available)
        return true; // Continue tracker operation without an unconnected wake pin.
    if (!readInterrupt() || pending)
        return false;
    // Do not clear INT_SOURCE again after arming; a new event must wake the CPU.
    rtc_gpio_pulldown_en(FALL_INT_PIN);
    rtc_gpio_pullup_dis(FALL_INT_PIN);
    return esp_sleep_enable_ext1_wakeup(1ULL << FALL_INT_PIN, ESP_EXT1_WAKEUP_ANY_HIGH) == ESP_OK;
}
