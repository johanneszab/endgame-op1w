# endgame-op1w

A Linux configuration tool for **Endgame Gear wireless mice** — an independent
implementation of the protocol used by the vendor's Windows-only tool,
reverse-engineered for interoperability.

Protocol documentation: [`re/PROTOCOL.md`](re/PROTOCOL.md).

## Supported models

| Model | Status |
|---|---|
| OP1w 4k v2 | verified on hardware |
| OP1w 4k (v1) | verified on hardware |
| XM2w 4k v2 | should work — untested, no hardware |
| XM2w 4k (v1) | should work — untested, no hardware |

The two generations differ in more than a few checkboxes: **lift-off distance
uses incompatible scales** (v1 is whole millimetres, 1 or 2; v2 is 0.7–2.0 mm in
0.1 mm steps), the polling options differ, and one filter flag bit moved. The
tool therefore identifies the mouse before writing anything model-specific.

That identification is not from USB — every wireless dongle enumerates as the
same `3367:1970` regardless of which mouse is paired to it. It comes from cmd
`0x0E`, which reports the mouse's own product ID. While the mouse is asleep
that query fails, so lift-off distance and polling rate stay disabled until it
wakes; everything else works meanwhile.

Only one mouse-and-dongle pair should be connected at a time.

__Note__: The code is completely vibe coded with Opus 5. Thus, if you submit
any changes/updates, or tested with the XM2, I am likely to accept the change.
However, I am pretty sure the reverse engineered protocol is correct, and
captured every settings change myself with [pcap](https://www.winpcap.org/).

## What it does

Everything the vendor tool exposes, except firmware update:

- **CPI** — 4 stages, independent X/Y, stage count, active stage
- **Sensor** — lift-off distance, angle snapping, ripple control, motion sync,
  plus angle tuning / glass mode / force max sensor FPS on v2, and the motion
  jitter filter on v1
- **Polling** — 4000 / 2000 / 1000 Hz, plus 1000 Hz with wireless power saving
  and 125 Hz office mode on v2
- **Power** — power-saving and deep-sleep inactivity timeouts (1–120 min)
- **Clicks** — slamclick filter, multiclick filter, per-button filter value,
  and the SPDT GX Safe / GX Speed modes on the left and right buttons
- **Buttons** — remap to mouse buttons, wheel, media keys, CPI cycle, a fixed
  CPI value, or nothing; left-handed mode
- **Device** — battery and signal level, mouse and dongle firmware, live
  sleep/wake state, re-pair, factory reset

## Building

### Debian based (apt)
```bash
sudo apt install build-essential cmake pkg-config libhidapi-dev qt6-base-dev
cmake -B build -S .
cmake --build build -j
```

### Red hat based (dnf)
```bash
sudo dnf install @c-development @development-tools
sudo dnf install build-essential cmake pkgconf-pkg-config hidapi-devel qt6-qtbase-devel
cmake -B build -S .
cmake --build build -j
```

The GUI is optional — if Qt 6 isn't found, only `egg-cli` is built.

## Screenshot:
![GUI](images/gui.png?raw=true "GUI")

## Permissions

`hidraw` nodes are root-only by default. Install the udev rule if you want to use the application as non-root user:

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

**Builds and runs against live hardware** — OP1w 4k v2 (mouse firmware 1.07)
and OP1w 4k v1 (firmware 1.08), both on dongle firmware 1.01.

The two XM2w models are in `kModels` (`src/egg/protocol.h`) with PIDs and
capability flags taken from their vendor tools, which are the same builds as
the OP1w ones with different constants. **Neither has been tested** — if you
own one, reports are welcome.

Model identification comes from cmd `0x0E`, not from USB: every wireless dongle
enumerates as `3367:1970`. While the mouse is asleep that query fails, and the
device layer then refuses any write whose encoding depends on the model, rather
than guessing. See [`re/PROTOCOL.md`](re/PROTOCOL.md) §12.

Not implemented: firmware update (commands not investigated), and the
`0x71`/`0x72` pairing commands, which exist in the vendor binary but are
unreachable from its UI.
