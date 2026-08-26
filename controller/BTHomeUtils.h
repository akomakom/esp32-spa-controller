//
// BTHome v2 BLE advertiser for the spa controller.
//
// Broadcasts sensor telemetry as a passive, connectionless BLE advertisement
// (BTHome v2, unencrypted, service UUID 0xFCD2) so another ESP32 — directly or
// via a BLE repeater / ESPHome bluetooth_proxy — can consume it without pairing.
// This mirrors the ESP-NOW broadcast model: fire-and-forget periodic beacons.
//
// Coexistence note: the controller shares one 2.4GHz radio between WiFi + ESP-NOW
// and BLE. Advertising is kept low-duty (slow interval, infrequent refresh) so it
// does not starve the ESP-NOW link. See ADVERTISE_INTERVAL_MS / REFRESH_INTERVAL_MS.
//
// Phase 1 payload: water temperature only (+ a monotonic packet id). The encoder
// is structured so binary sensors (heater/pump/... on-off) drop in later — see
// buildPayload().
//

#ifndef HOT_TUB_CONTROLLER_BTHOMEUTILS_H
#define HOT_TUB_CONTROLLER_BTHOMEUTILS_H

#include <sys/types.h>
#include <vector>

class BTHomeUtils {
public:
    // Initialise NimBLE and begin advertising. Safe to call once from setup().
    static void setup(const char* deviceName);

    // Rebuilds and republishes the advertisement at most every REFRESH_INTERVAL_MS.
    // Call every main-loop iteration; it throttles internally.
    static void loop();

    // Latest sensor values, set by the main loop before loop() publishes them.
    // waterTempF is the native Fahrenheit reading; valid=false means the sensor
    // reading is stale/faulted (the BTHome temperature object is then omitted).
    static void setWaterTemp(float waterTempF, bool valid);

private:
    // BLE advertising interval (how often the radio emits the beacon). Kept long to
    // minimise airtime stolen from ESP-NOW. Units: milliseconds (converted to 0.625ms
    // slots internally).
    // Fast enough that a weak / coexistence-throttled link still lands frames often
    // enough for a receiver to discover and parse one. A 13-byte non-connectable packet
    // is <1ms of airtime, so this adds negligible pressure on the ESP-NOW link.
    static constexpr uint32_t ADVERTISE_INTERVAL_MS = 250;
    // How often we rebuild the payload with fresh values. A hot tub changes slowly,
    // so infrequent updates are plenty and keep the radio quiet.
    static constexpr uint32_t REFRESH_INTERVAL_MS = 10000;

    // BTHome service UUID (16-bit) and the device-information byte:
    // bit0=encryption(0), bit2=trigger(0), bits5-7=version(2) => 0x40.
    static constexpr uint16_t BTHOME_UUID = 0xFCD2;
    static constexpr uint8_t  BTHOME_DEVICE_INFO = 0x40;

    // BTHome v2 object ids we emit.
    static constexpr uint8_t OBJ_PACKET_ID   = 0x00; // uint8 counter, for dedup
    static constexpr uint8_t OBJ_TEMPERATURE = 0x02; // sint16 LE, factor 0.01 degC

    static bool     initialised;
    static bool     advertising;
    static uint8_t  packetId;
    static uint32_t lastRefreshMs;

    static float    waterTempC;
    static bool     waterTempValid;

    // Assemble the BTHome service-data payload (everything after the 0xFCD2 UUID:
    // the device-info byte followed by measurement records).
    static std::vector<uint8_t> buildPayload();
    static void publish();
};

#endif //HOT_TUB_CONTROLLER_BTHOMEUTILS_H
