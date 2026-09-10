#include <Arduino.h>
#include <TinyGPS++.h>
#include <driver/rtc_io.h>
#include <RadioLib.h>
#include <Preferences.h>
#include <SPI.h>
#include <math.h>

#include <gnss.h>
#include <lora_wan.h>

/* --- Setup Key --- */
uint32_t devAddr = (uint32_t)0x00000000;
uint8_t nwkSKey[] = {};
uint8_t appSKey[] = {};
uint8_t devEui[] = {};
uint8_t appEui[] = {};
uint8_t appKey[] = {};

// /* --- Settings --- */
static constexpr uint32_t appTxDutyCycle = 900000;
static constexpr uint8_t appPort = 2;
static SX1262 radio = new Module(LoRa_NSS, DIO1, LoRa_RST, LoRa_BUSY);
// Preserve the original channel-0-only configuration (923.2 MHz).
static LoRaWANBand_t loraBand = [] {
    LoRaWANBand_t band = AS923;
    band.txFreqs[1].freq = 0;
    return band;
}();
static LoRaWANNode node(&radio, &loraBand);
static Preferences storage;
static bool ready = false;
static bool attempted = false;
static uint32_t lastAttempt = 0;
static uint32_t reservedCounter = 0;
static uint8_t appData[17];
static size_t appDataSize = 0;

// One atomic NVS blob: credentials/nonces and MAC state must match.
struct SavedSession
{
    uint8_t nonces[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
    uint8_t session[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
};

// RadioLib 7.6.0 serialization (version pinned in platformio.ini).
static uint32_t nextCounter(const uint8_t *session)
{
    const uint8_t *p = session + RADIOLIB_LORAWAN_SESSION_FCNT_UP;
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}

static void updateSessionChecksum(uint8_t *session)
{
    uint16_t checksum = 0;
    for (size_t i = 0; i < RADIOLIB_LORAWAN_SESSION_SIGNATURE; i += 2)
    {
        uint16_t word = uint16_t(session[i]) << 8;
        if (i + 1 < RADIOLIB_LORAWAN_SESSION_SIGNATURE)
            word |= session[i + 1];
        checksum ^= word;
    }
    session[RADIOLIB_LORAWAN_SESSION_SIGNATURE] = checksum;
    session[RADIOLIB_LORAWAN_SESSION_SIGNATURE + 1] = checksum >> 8;
}

// Only repair the known pre-fix ABP layout, after RadioLib has validated
// both buffer checksums and the Nonces configuration. Never change counters.
static bool repairLegacySession(SavedSession &saved)
{
    const uint8_t *s = saved.session;
    if (s[RADIOLIB_LORAWAN_SESSION_NONCES_SIGNATURE] != 0 ||
        s[RADIOLIB_LORAWAN_SESSION_NONCES_SIGNATURE + 1] != 0 ||
        s[RADIOLIB_LORAWAN_SESSION_VERSION] != 0 ||
        s[RADIOLIB_LORAWAN_SESSION_STATUS] != RADIOLIB_LORAWAN_SESSION_ACTIVE ||
        memcmp(s + RADIOLIB_LORAWAN_SESSION_APP_SKEY, appSKey, 16) != 0 ||
        memcmp(s + RADIOLIB_LORAWAN_SESSION_NWK_SENC_KEY, nwkSKey, 16) != 0 ||
        memcmp(s + RADIOLIB_LORAWAN_SESSION_FNWK_SINT_KEY, nwkSKey, 16) != 0 ||
        memcmp(s + RADIOLIB_LORAWAN_SESSION_SNWK_SINT_KEY, nwkSKey, 16) != 0)
        return false;
    for (unsigned i = 0; i < 4; ++i)
        if (s[RADIOLIB_LORAWAN_SESSION_DEV_ADDR + i] != uint8_t(devAddr >> (8 * i)))
            return false;
    // The old activation signed an entirely zero-filled Nonces buffer.
    for (size_t i = RADIOLIB_LORAWAN_NONCES_DEV_NONCE;
         i < RADIOLIB_LORAWAN_NONCES_SIGNATURE; ++i)
        if (saved.nonces[i] != 0)
            return false;
    memcpy(saved.session + RADIOLIB_LORAWAN_SESSION_NONCES_SIGNATURE,
           saved.nonces + RADIOLIB_LORAWAN_NONCES_SIGNATURE, 2);
    updateSessionChecksum(saved.session);
    return true;
}

static bool saveSession(bool reserveUplink)
{
    SavedSession saved;
    memcpy(saved.nonces, node.getBufferNonces(), sizeof(saved.nonces));
    memcpy(saved.session, node.getBufferSession(), sizeof(saved.session));
    uint32_t counter = nextCounter(saved.session);
    if (counter == UINT32_MAX)
        return false;
    if (reserveUplink)
        ++counter;
    // Never roll back a counter reserved before a possibly transmitted packet.
    if (counter < reservedCounter)
        counter = reservedCounter;
    for (unsigned i = 0; i < 4; ++i)
        saved.session[RADIOLIB_LORAWAN_SESSION_FCNT_UP + i] = counter >> (8 * i);
    updateSessionChecksum(saved.session);
    if (storage.putBytes("session", &saved, sizeof(saved)) != sizeof(saved))
        return false;
    reservedCounter = counter;
    return true;
}

extern TinyGPSPlus GPS;

extern int32_t rtc_lat;
extern int32_t rtc_lon;
extern uint8_t rtc_hour;
extern uint8_t rtc_minute;
extern uint8_t rtc_second;
extern uint8_t rtc_centisecond;

static void prepareTxFrame(uint8_t status, bool debug)
{
    uint8_t hour, second, minute, centisecond;
    float lat, lon;

    if (debug)
    {
        // Synthetic starting point, not a GPS fix. Continue a walking track.
        static double walkingLat = 13.7563;
        static double walkingLon = 100.5018;
        static double heading = 0.0;
        static uint32_t lastStep = 0;
        static bool started = false;
        uint32_t now = millis();
        if (started)
        {
            double seconds = uint32_t(now - lastStep) / 1000.0;
            double speed = 0.9 + (esp_random() % 601) / 1000.0; // 0.9–1.5 m/s
            heading += (int(esp_random() % 6101) - 3050) / 100.0 * DEG_TO_RAD;
            double distance = speed * seconds;
            walkingLat += distance * cos(heading) / 111320.0;
            walkingLon += distance * sin(heading) / (111320.0 * cos(walkingLat * DEG_TO_RAD));
        }
        started = true;
        lastStep = now;
        lat = walkingLat;
        lon = walkingLon;
        // Synthetic elapsed time; never presented as actual GNSS time.
        uint32_t seconds = now / 1000;
        hour = (seconds / 3600) % 24;
        minute = (seconds / 60) % 60;
        second = seconds % 60;
        centisecond = (now % 1000) / 10;
        Serial.print("[SIMULATED WALK]");
    }
    else if (status == 1 || status == 2)
    {
        // if (rtc_lat == 0 && rtc_lon == 0)
        //     get_location();

        lat = rtc_lat / 1e6f;
        lon = rtc_lon / 1e6f;
        hour = rtc_hour;
        minute = rtc_minute;
        second = rtc_second;
        centisecond = rtc_centisecond;
    }
    else
    {
        if (!debug)
            get_location();
        lat = GPS.location.lat();
        lon = GPS.location.lng();
        hour = GPS.time.hour();
        minute = GPS.time.minute();
        second = GPS.time.second();
        centisecond = GPS.time.centisecond();
    }

    Serial.printf(" %02d:%02d:%02d.%02d", hour, minute, second, centisecond);
    Serial.print(", LAT: ");
    Serial.print(lat, 6);
    Serial.print(", LON: ");
    Serial.print(lon, 6);
    Serial.printf(", STATUS: %d", status);
    Serial.println();

    unsigned char *puc;

    appDataSize = 0;
    appData[appDataSize++] = status;

    puc = (unsigned char *)(&lat);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];
    appData[appDataSize++] = puc[2];
    appData[appDataSize++] = puc[3];
    puc = (unsigned char *)(&lon);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = puc[1];
    appData[appDataSize++] = puc[2];
    appData[appDataSize++] = puc[3];

    puc = (unsigned char *)(&hour);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = 0;

    puc = (unsigned char *)(&minute);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = 0;

    puc = (unsigned char *)(&second);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = 0; // Preserve decoder's two-byte time fields.

    puc = (unsigned char *)(&centisecond);
    appData[appDataSize++] = puc[0];
    appData[appDataSize++] = 0;
}
bool setup_lora_wan_app()
{
    ready = false;
    storage.end();
    SPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI, LoRa_NSS);
    // SX1262 with DIO3-controlled 1.8 V TCXO; DIO2 controls RF switch.
    int16_t state = radio.begin(923.2, 125.0, 9, 5, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 14, 8, 1.8);
    Serial.printf("[LoRa] radio init: %d\n", state);
    if (state != RADIOLIB_ERR_NONE)
        return false;
    state = node.beginABP(devAddr, nullptr, nullptr, nwkSKey, appSKey);
    if (state != RADIOLIB_ERR_NONE)
        return false;
    if (!storage.begin("radiolib-abp", false))
    {
        Serial.println("[LoRa] NVS unavailable; transmission disabled");
        return false;
    }
    if (storage.isKey("session"))
    {
        SavedSession saved;
        if (storage.getBytesLength("session") != sizeof(saved) ||
            storage.getBytes("session", &saved, sizeof(saved)) != sizeof(saved))
        {
            Serial.println("[LoRa] Saved session size/read failed; no counter reset");
            return false;
        }
        state = node.setBufferNonces(saved.nonces);
        if (state != RADIOLIB_ERR_NONE)
        {
            Serial.printf("[LoRa] Nonces restore failed: %d; no counter reset\n", state);
            return false;
        }
        state = node.setBufferSession(saved.session);
        if (state == RADIOLIB_ERR_SESSION_DISCARDED && repairLegacySession(saved))
        {
            state = node.setBufferSession(saved.session);
            if (state == RADIOLIB_ERR_NONE)
                Serial.printf("[LoRa] Recovered legacy ABP signature; FCnt preserved: %lu\n",
                              (unsigned long)nextCounter(saved.session));
        }
        if (state != RADIOLIB_ERR_NONE)
        {
            Serial.printf("[LoRa] Session restore failed: %d; no counter reset\n", state);
            return false;
        }
        reservedCounter = nextCounter(saved.session);
    }
    else
        Serial.println("[LoRa] New ABP session: ensure server frame counter matches this migration");
    // Populate version/mode/plan/key checksum BEFORE activateABP signs Nonces
    // and links that signature into the new session (RadioLib 7.6.0).
    node.getBufferNonces();
    state = node.activateABP();
    // Activation has negative SUCCESS status codes; unlike sendReceive(),
    // a negative result alone does not indicate failure here.
    if (state != RADIOLIB_ERR_NONE &&
        state != RADIOLIB_LORAWAN_NEW_SESSION &&
        state != RADIOLIB_LORAWAN_SESSION_RESTORED)
    {
        Serial.printf("[LoRa] ABP activation failed: %d\n", state);
        return false;
    }
    Serial.printf("[LoRa] ABP ready: %s (%d)\n",
                  state == RADIOLIB_LORAWAN_NEW_SESSION ? "new session" :
                  (state == RADIOLIB_LORAWAN_SESSION_RESTORED ? "session restored" : "already active"), state);
    node.setADR(false);
    // DR3 (SF9/BW125) accommodates the 17-byte payload with AS923 dwell time.
    if (node.setDatarate(3) != RADIOLIB_ERR_NONE)
        return false;
    ready = saveSession(false);
    return ready;
}

void enter_lora_wan_app(uint8_t wake_status, bool debug, uint32_t debug_interval_ms)
{
    update_fall_detection(debug);
    // Keep GPS parsing while awake, including debug mode with no GPS fix.
    while (Serial1.available())
        GPS.encode(Serial1.read());
    // Pending events retry at 15 seconds even in normal mode while awake.
    uint32_t interval = debug ? max(debug_interval_ms, uint32_t(1000)) :
                        (fall_detection_pending() ? 15000 : appTxDutyCycle);
    if (attempted && uint32_t(millis() - lastAttempt) < interval)
    {
        delay(10);
        return;
    }
    attempted = true;
    lastAttempt = millis();
    if (!ready && !setup_lora_wan_app())
    {
        Serial.println("[LoRa] Initialization failed; will retry next interval");
        return;
    }
    if (node.timeUntilUplink() > 0)
    {
        Serial.println("[LoRa] Duty-cycle hold; retry next interval");
        return;
    }
    if (node.setDatarate(debug ? 5 : 3) != RADIOLIB_ERR_NONE)
    {
        Serial.println("[LoRa] Could not set data rate; transmission skipped");
        return;
    }
    prepareTxFrame(fall_detection_pending() ? 2 : (debug ? 0 : wake_status), debug);
    // GPS acquisition may block; capture any hardware-latched event before TX.
    update_fall_detection(false);
    const bool sendingFall = fall_detection_pending();
    if (sendingFall)
        appData[0] = 2;
    // Commit the NEXT counter before RF transmission, including sudden power loss.
    if (!saveSession(true))
    {
        Serial.println("[LoRa] Could not reserve frame counter; transmission skipped");
        ready = false;
        return;
    }
    Serial.printf("[LoRa] TX port=%u bytes=%u FCnt=%lu GPS=%s\n", appPort,
                  unsigned(appDataSize), (unsigned long)nextCounter(node.getBufferSession()),
                  debug ? "SIMULATED" : (GPS.location.isValid() ? "valid" : "no fix"));
    int16_t state = node.sendReceive(appData, appDataSize, appPort, false);
    if (state >= RADIOLIB_ERR_NONE && sendingFall)
        acknowledge_fall_detection();
    Serial.printf("[LoRa] sendReceive=%d (0=no downlink, 1/2=RX window, negative=error)\n", state);
    if (!saveSession(false))
    {
        Serial.println("[LoRa] Session save failed; reserved uplink counter remains in NVS");
        ready = false;
    }
    if (debug)
    {
        Serial.printf("[LoRa] Debug: next attempt in %lu ms; ESP32 stays awake\n", (unsigned long)interval);
        return;
    }
    uint32_t sleepMs = max(appTxDutyCycle, uint32_t(node.timeUntilUplink()));
    // Capture events arriving during TX/RX; don't sleep over an unsent event.
    if (!prepare_fall_detection_sleep())
        return;
    radio.sleep();
    Serial1.end();
    pinMode(VGNSS_CTRL, OUTPUT);
    digitalWrite(VGNSS_CTRL, HIGH);
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, HIGH);
    pinMode(GNSS_TX, INPUT);
    pinMode(GNSS_RX, INPUT);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_INT_PIN, 0);
    esp_sleep_enable_timer_wakeup(uint64_t(sleepMs) * 1000);
    Serial.printf("[LoRa] Deep sleep for %lu ms\n", (unsigned long)sleepMs);
    Serial.flush();
    esp_deep_sleep_start();
}
