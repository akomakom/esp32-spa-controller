//
// Created for BLE BTHome v2 broadcast — see BTHomeUtils.h.
//

#include "BTHomeUtils.h"

// BLE/BTHome is opt-in: enable with -DENABLE_BLE_BTHOME (the `controller-ble` env in
// platformio.ini). When off, this whole translation unit — and the NimBLE dependency —
// compiles to nothing, and the call sites in controller.ino are #ifdef'd out too.
#ifdef ENABLE_BLE_BTHOME

#include <NimBLEDevice.h>
#include <Arduino.h>
#include <math.h>

#include "TemperatureUtils.h"

bool     BTHomeUtils::initialised   = false;
bool     BTHomeUtils::advertising   = false;
uint8_t  BTHomeUtils::packetId      = 0;
uint32_t BTHomeUtils::lastRefreshMs = 0;
float    BTHomeUtils::waterTempC    = 0.0f;
bool     BTHomeUtils::waterTempValid = false;

void BTHomeUtils::setWaterTemp(float waterTempF, bool valid) {
    waterTempValid = valid;
    if (valid) {
        waterTempC = TemperatureUtils::fToC(waterTempF);
    }
}

// Build everything that follows the 0xFCD2 service UUID: the BTHome device-info
// byte, then one measurement record per value. Object ids must appear in ascending
// order per the BTHome v2 spec; keep additions sorted.
std::vector<uint8_t> BTHomeUtils::buildPayload() {
    std::vector<uint8_t> p;
    p.push_back(BTHOME_DEVICE_INFO);

    // Packet id (0x00): lets the consumer dedup repeated adv frames.
    p.push_back(OBJ_PACKET_ID);
    p.push_back(packetId);

    // Temperature (0x02): sint16 little-endian, factor 0.01 degC. Omitted entirely
    // when the reading is stale/faulted so the consumer sees "no data" rather than 0.
    if (waterTempValid) {
        long raw = lroundf(waterTempC * 100.0f);
        if (raw >  32767) raw =  32767;
        if (raw < -32768) raw = -32768;
        int16_t t = (int16_t) raw;
        p.push_back(OBJ_TEMPERATURE);
        p.push_back((uint8_t)(t & 0xFF));
        p.push_back((uint8_t)((t >> 8) & 0xFF));
    }

    return p;
}

void BTHomeUtils::publish() {
    std::vector<uint8_t> payload = buildPayload();

    // Assemble the raw advertisement: Flags AD + Service-Data (16-bit UUID) AD.
    std::vector<uint8_t> adv;
    adv.push_back(0x02);            // Flags AD length
    adv.push_back(0x01);            // Flags AD type
    adv.push_back(0x06);            // LE General Discoverable, BR/EDR not supported

    adv.push_back((uint8_t)(3 + payload.size())); // len: type(1) + uuid(2) + payload
    adv.push_back(0x16);           // Service Data - 16-bit UUID
    adv.push_back((uint8_t)(BTHOME_UUID & 0xFF));        // UUID low byte
    adv.push_back((uint8_t)((BTHOME_UUID >> 8) & 0xFF)); // UUID high byte
    adv.insert(adv.end(), payload.begin(), payload.end());

    NimBLEAdvertisementData advData;
    advData.addData(adv.data(), adv.size());

    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    if (advertising) {
        pAdv->stop();
    }
    bool dataOk  = pAdv->setAdvertisementData(advData);
    bool startOk = pAdv->start();
    advertising = true;

    // Diagnostics: dump the exact advertised bytes + whether the stack accepted them.
    Serial.printf("BTHome adv (%u B) setData=%d start=%d active=%d: ",
                  (unsigned)adv.size(), dataOk, startOk, ble_gap_adv_active());
    for (uint8_t b : adv) Serial.printf("%02X ", b);
    Serial.println();
}

void BTHomeUtils::setup(const char* deviceName) {
    if (initialised) {
        return;
    }
    NimBLEDevice::init(deviceName ? deviceName : "Hot Tub");
    bool pwrOk = NimBLEDevice::setPower(9); // max TX power (dBm) — help range through coexistence
    Serial.printf("BTHome: NimBLE up, BLE MAC %s, setPower(9)=%d readback=%d dBm\n",
                  NimBLEDevice::getAddress().toString().c_str(), pwrOk, NimBLEDevice::getPower());

    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    // Non-connectable, non-scannable beacon: we only broadcast, never accept links.
    pAdv->setConnectableMode(BLE_GAP_CONN_MODE_NON);
    pAdv->enableScanResponse(false);
    // Advertising interval in 0.625ms units. Long interval => less radio contention
    // with the ESP-NOW link.
    uint16_t slots = (uint16_t)((uint32_t)ADVERTISE_INTERVAL_MS * 1000UL / 625UL);
    pAdv->setMinInterval(slots);
    pAdv->setMaxInterval(slots);

    initialised = true;
    // Publish an initial frame immediately; loop() refreshes it thereafter.
    publish();
    lastRefreshMs = millis();
}

void BTHomeUtils::loop() {
    if (!initialised) {
        return;
    }
    uint32_t nowMs = millis();
    if (nowMs - lastRefreshMs < REFRESH_INTERVAL_MS) {
        return;
    }
    lastRefreshMs = nowMs;
    packetId++;
    publish();
}

#endif // ENABLE_BLE_BTHOME
