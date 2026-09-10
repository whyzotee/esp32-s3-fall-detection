// Host test: node test/session_recovery.cjs (requires c++ and installed RadioLib).
const fs = require('fs');
const cp = require('child_process');
const os = require('os');
const path = require('path');
const header = fs.readFileSync('.pio/libdeps/heltec_wifi_lora_32_V4/RadioLib/src/protocols/LoRaWAN/LoRaWAN.h', 'utf8');
const source = fs.readFileSync('src/lora_wan.cpp', 'utf8');
// Extract only pure helpers, never the project's device credentials.
const helpers = source.slice(source.indexOf('struct SavedSession'), source.indexOf('static bool saveSession'));
const definitions = header.match(/^#define RADIOLIB_.*$/gm).join('\n');
const enums = header.match(/enum LoRaWANScheme(?:Base|Session)_t \{[\s\S]*?\n\};/g).join('\n');
const program = `
#include <cstdint>
#include <cstring>
#include <cassert>
#define RADIOLIB_AES128_KEY_SIZE 16
${definitions}
${enums}
uint32_t devAddr = 0x01020304;
uint8_t appSKey[16] = {1}, nwkSKey[16] = {2};
${helpers}
int main() {
    SavedSession s = {};
    s.session[RADIOLIB_LORAWAN_SESSION_STATUS] = RADIOLIB_LORAWAN_SESSION_ACTIVE;
    memcpy(s.session + RADIOLIB_LORAWAN_SESSION_APP_SKEY, appSKey, 16);
    for (auto offset : {RADIOLIB_LORAWAN_SESSION_NWK_SENC_KEY,
         RADIOLIB_LORAWAN_SESSION_FNWK_SINT_KEY, RADIOLIB_LORAWAN_SESSION_SNWK_SINT_KEY})
        memcpy(s.session + offset, nwkSKey, 16);
    for (unsigned i=0; i<4; ++i)
        s.session[RADIOLIB_LORAWAN_SESSION_DEV_ADDR+i] = devAddr >> (8*i);
    s.session[RADIOLIB_LORAWAN_SESSION_FCNT_UP] = 123;
    s.nonces[RADIOLIB_LORAWAN_NONCES_SIGNATURE] = 42;
    updateSessionChecksum(s.session);
    auto original = s;
    assert(repairLegacySession(s));
    assert(nextCounter(s.session) == 123);
    for (size_t i=0; i<sizeof(s.session); ++i)
        if (i != RADIOLIB_LORAWAN_SESSION_NONCES_SIGNATURE &&
            i != RADIOLIB_LORAWAN_SESSION_NONCES_SIGNATURE+1 &&
            i < RADIOLIB_LORAWAN_SESSION_SIGNATURE)
            assert(s.session[i] == original.session[i]);
    assert(!repairLegacySession(s));
    s = original; s.session[RADIOLIB_LORAWAN_SESSION_APP_SKEY] ^= 1;
    assert(!repairLegacySession(s));
    s = original; s.session[RADIOLIB_LORAWAN_SESSION_DEV_ADDR] ^= 1;
    assert(!repairLegacySession(s));
    s = original; s.nonces[RADIOLIB_LORAWAN_NONCES_DEV_NONCE] = 1;
    assert(!repairLegacySession(s));
}
`;
const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'session-recovery-'));
const binary = path.join(dir, 'test');
cp.execFileSync('c++', ['-std=c++17', '-include', 'initializer_list', '-x', 'c++', '-', '-o', binary], { input: program });
cp.execFileSync(binary);
console.log('Session recovery helper tests passed (synthetic credentials only)');
