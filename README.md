# endgame-op1w

A Linux configuration tool for the **Endgame Gear OP1w 4k v2** wireless mouse —
an independent implementation of the protocol used by the vendor's Windows-only
tool, reverse-engineered for interoperability.

Protocol documentation: [`re/PROTOCOL.md`](re/PROTOCOL.md).

This tool might work for the OP1w v1, and the wireless XM2 tools as well. I
lack the hardware to test it though.

__Note__: The code is completely vibe coded with Opus 5. Thus, if you submit
any changes/updates, or tested with the XM2, I am likely to accept the change.
However, I am pretty sure the reverse engineered protocol is correct, and
captured every settings change myself with [pcap](https://www.winpcap.org/).

## What it does

Everything the vendor tool exposes, except firmware update:

- **CPI** — 4 stages, independent X/Y, stage count, active stage
- **Sensor** — lift-off distance (0.7–2.0 mm), angle snapping, ripple control,
  angle tuning, motion sync, glass mode, force max sensor FPS
- **Polling** — 4000 / 2000 / 1000 Hz, and 1000 Hz with wireless power saving
- **Power** — power-saving and deep-sleep inactivity timeouts (1–120 min)
- **Clicks** — slamclick filter, multiclick filter, per-button filter value,
  and the SPDT GX Safe / GX Speed modes on the left and right buttons
- **Buttons** — remap to mouse buttons, wheel, media keys, CPI cycle, a fixed
  CPI value, or nothing; left-handed mode
- **Device** — battery and signal level, mouse and dongle firmware, live
  sleep/wake state, re-pair, factory reset

## Building

```bash
sudo apt install build-essential cmake pkg-config libhidapi-dev qt6-base-dev
cmake -B build -S .
cmake --build build -j
```

The GUI is optional — if Qt 6 isn't found, only `egg-cli` is built.

## Permissions

`hidraw` nodes are root-only by default. Install the udev rule and replug the
dongle:

```bash
sudo cp udev/70-endgamegear.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

No kernel driver needs detaching — `hidraw` coexists with `usbhid`, so the mouse
keeps working while the tool talks to it.

## Usage

```bash
./build/egg-gui                        # graphical
./build/egg-cli info                   # firmware, battery
./build/egg-cli show                   # current configuration
./build/egg-cli set cpi 1 1600
./build/egg-cli set polling 4000
./build/egg-cli set lod 1.5
./build/egg-cli set click-filter left gx-speed
./build/egg-cli map back consumer 0xEA # back button -> volume down
./build/egg-cli map back cpi 600 400   # back button -> fixed 600/400 CPI
./build/egg-cli listen                 # watch battery / sleep events live
./build/egg-cli --help
```

## Design notes

**Whole-block writes.** The device takes configuration in three blocks
(sensor/CPI, polling/power/filters, button table). There is no way to change a
single field — every write resends its entire block. Both front-ends therefore
read the device's stored configuration first and modify it, so a write never
invents values it didn't read.

**Two fields live in two places.** Each button's multiclick-filter value appears
both in the cmd `0x15` payload and in byte +6 of that button's record in the
cmd `0x16` table. Writing one with a stale copy of the other silently reverts
it, so `syncFilters()` is called before every button-table write. Don't remove
it.

**The length field lies, deliberately.** Cmd `0x15` sends eleven payload bytes
but declares ten, and the device acts on the eleventh (sensor glass mode). This
is what the vendor tool does, and it is reproduced exactly — see
`kPowerDeclaredLength`.

**Serialised access.** The device does not tolerate interleaved or pipelined
commands, and answers `status = 0x03` while busy. `egg::Device` guards every
exchange with a mutex and implements the vendor tool's retry and back-off rules.
Don't bypass it.

**Nothing is polled over USB.** The dongle pushes 8-byte notifications
(report ID `0x03`) carrying battery level, signal level and radio link state.
The GUI subscribes to that channel: a 1-second timer drains the local hidraw
queue, which costs nothing, and the device itself is only queried at startup,
on an explicit Reload, or when a link-up event says there is fresh state worth
reading. `egg-cli listen` prints the same stream.

**Two links, two states.** The dongle stays reachable over USB while the mouse
is asleep, so `0x0D` (dongle firmware) keeps answering while `0x0E` (mouse
firmware) and `0xB4` (battery) do not. Both front-ends use that split to show
an "Asleep" state instead of reporting an error.

## Status

The protocol layer is derived from static analysis, 43 USB captures and
hardware verification; every decoded value was checked against the vendor
tool's own UI, and the writable ones round-trip through the device.

**Builds and runs against live hardware** — OP1w 4k v2, mouse firmware 1.07 /
dongle 1.01.

Other models: `kModels` in `src/egg/protocol.h` carries the PIDs for the OP1w
4k, XM2w 4k and XM2w 4k v2, extracted from their vendor tools, along with
per-model capability flags. **None of those three has been tested** — and over
the dongle the model cannot be identified at all, since all four wireless mice
share PID `0x1970`. See `../re/PROTOCOL.md` §10.

Not implemented: firmware update (commands not investigated), and the
`0x71`/`0x72` pairing commands, which exist in the vendor binary but are
unreachable from its UI.
