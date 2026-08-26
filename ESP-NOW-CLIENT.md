# ESP-NOW client requirements (for the controller/"server")

This describes what an ESP-NOW **client** (subscriber, e.g. a Waveshare touchscreen)
must do to pair with and receive data from the spa **controller** (the "server").
The reference working implementation is `display/ESPNowUtils.cpp` in this repo — mirror
its behavior. Shared wire structs are in `hot_tub_types.h` (must match byte-for-byte).

## The one rule that trips everyone up: channel

The controller runs `WIFI_AP_STA` and is **joined to the home WiFi router**, so its radio
is **hard-locked to the router's channel** (e.g. channel 6). A single-radio ESP32 cannot
run ESP-NOW on any other channel. Therefore:

- The client **must be pure `WIFI_STA` and NOT connected to any AP**
  (`WiFi.mode(WIFI_STA); WiFi.disconnect();`). If the client joins WiFi, its channel is
  pinned and can never match the server → every send is `Delivery Fail`.
- The client **finds the server's channel by scanning**, then **parks on it and stops**.
  While it is still hopping it is almost never on the server's channel, so the server's
  replies/broadcasts get no MAC ACK → `Delivery Fail`. "Stuck sending pairing requests"
  is the classic symptom of a client that never parks.
- **Disable modem sleep** (`WiFi.setSleep(false);` or `esp_wifi_set_ps(WIFI_PS_NONE);`),
  or the client sleeps between beacons and drops packets.

## Message types (`hot_tub_types.h`)

First byte of every packet is `msgType`. Enum order is fixed:

```
enum MessageType { PAIRING=0, COMMAND=1, CONTROL_STATUS=2, SERVER_STATUS=3, METRICS_STATUS=4 };
```

- `struct_pairing`   — handshake, both directions
- `struct_command`   — client → server (user actions / overrides)
- `struct_status_control` (CONTROL_STATUS) — server → client, one per control
- `struct_status_server`  (SERVER_STATUS)  — server → client, water temp / time / flags

When receiving, **bound the `memcpy` to the received `len`** (`min(len, sizeof(target))`)
into a zero-initialized struct, so an older/newer controller struct layout can't over-read
and missing trailing fields default to 0. See `display/ESPNowUtils.cpp` `OnDataRecv`.

## Pairing handshake

**Client scan loop** (see `autoPairing()` in `display/ESPNowUtils.cpp`):

1. Pick a channel; `esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE)`.
2. (Re)init ESP-NOW, register send/recv callbacks.
3. Add a peer for the **broadcast address** on that channel and send a `struct_pairing`:
   - `msgType = PAIRING`
   - `board_id = <your non-zero client id>`  (the server ignores `board_id == 0` = itself)
   - `channel = channel` (the channel you're currently probing)
4. Wait ~250 ms for a response. If none, step to the next channel (1..13) and repeat.

**Server response** (`struct_pairing` with `board_id == 0`):

- `macAddr` = the server's **soft-AP MAC** — the address the client must send commands to.
- `channel` = the server's **actual** channel (not the one you guessed).

**Client on receiving the response — this is the step that must not be skipped:**

1. `esp_wifi_set_channel(resp.channel, WIFI_SECOND_CHAN_NONE)` — lock onto the server's channel.
2. `esp_now_del_peer` + `esp_now_add_peer` for `resp.macAddr` with `peer.channel = resp.channel`.
3. Mark state PAIRED and **stop channel-hopping**. Store `serverAddress = resp.macAddr`.
4. (Optional) persist the channel in NVS/EEPROM to pair faster next boot.

## Interface / MAC direction (why replies fail even on the right channel)

- The server replies and **broadcasts to the client's STA MAC** (it uses `info->src_addr`
  from the client's request). So the client must **send its pairing request from, and
  listen on, its STA interface** — the default in `WIFI_STA`. Don't send from a soft-AP
  interface.
- The client sends **commands to the server's soft-AP MAC** (`resp.macAddr` above).
- The client does **not** need the server registered as a peer to *receive* broadcasts;
  it only needs to be on the server's channel with its STA MAC listening.

## After pairing

- The server broadcasts `SERVER_STATUS` + one `CONTROL_STATUS` per control roughly every
  second. Just handle them in the recv callback.
- **Re-pair if the link goes quiet**: if no message has arrived within a timeout
  (the reference uses `MESSAGE_RECEIVED_MAX_AGE`), drop back to the scan loop — this
  recovers automatically when the server reboots and its channel possibly changes.
- Commands: fill a `struct_command` and `esp_now_send(serverAddress, ...)`.

## Reading `water_temp`

`struct_status_server.water_temp` is **Fahrenheit** (native unit). `temp_unit`
(0 = F, 1 = C) tells you which unit to *display*; convert at presentation time only.
A value of **exactly 0 is the sensor-fault sentinel** (the controller sends 0 when the
water-temp reading is stale/invalid and the heater is locked out) — show a fault
indicator instead of "0°", don't treat it as a real temperature.

## Quick checklist

- [ ] `WIFI_STA` only, `WiFi.disconnect()`, never joined to an AP
- [ ] Modem sleep disabled
- [ ] Channel-hop to find the server, then **lock to `resp.channel` and stop**
- [ ] Peer = server soft-AP MAC (`resp.macAddr`) on the server's channel
- [ ] Send from / listen on the **STA** interface MAC
- [ ] `memcpy` bounded by received `len` into zero-initialized structs
- [ ] Re-enter pairing when the link is silent past a timeout
- [ ] Treat `water_temp == 0` as a sensor fault
