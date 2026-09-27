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
uses incompatible scales** (v1 is whole millimetres, 1 or 2; v2 is 0.7–1.7 mm in
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

Everything the vendor tool exposes, firmware updates included:

- **CPI** — 4 stages, independent X/Y, stage count, and on v2 the active stage
  (the v1 firmware ignores that field — it switches stages with the button
  under the mouse, or any button bound to *CPI cycle*). Each stage's LED
  colour is shown: 1 blue, 2 green, 3 yellow, 4 red.
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
- **Firmware updates** — `egg-fw` flashes the vendor's own updater executable,
  so no Windows is needed. See below.

Works over the dongle or with the mouse plugged in by USB-C. A cabled mouse
reports no dongle firmware and a meaningless battery/signal reading, because
there is no radio link to describe.

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

## Firmware updates

`egg-fw` takes the vendor's updater executable exactly as downloaded from
endgamegear.com. There is no separate extraction step, and there should not be:
one updater ships five candidate firmware images of identical size and flashes
the third, so choosing by hand is a coin flip. `egg-fw` recovers the right one
from the instruction that passes it to `FindResourceW`, and refuses to run if
it cannot pin exactly one.

**The mouse must be connected by its USB-C cable, not through the dongle** —
the firmware path is cable-only, and `egg-fw` will not touch the dongle.

```bash
./build/egg-fw info                                    # what is attached
./build/egg-fw verify  Endgame_..._Updater_v1.10.exe   # inspect, touch nothing
./build/egg-fw flash   Endgame_..._Updater_v1.10.exe   # rehearsal
./build/egg-fw flash   Endgame_..._Updater_v1.10.exe --yes
```

Without `--yes` nothing is written: it opens the mouse, proves the firmware
report actually reaches it, and stops. The updater names its own model, which
is checked against the mouse in front of you, so a v2 firmware cannot be
flashed into a v1.

**It ends with a factory reset**, exactly as the vendor's updater does, so save
your settings first with `egg-cli blob` while the dongle is still connected.

If an update is interrupted the mouse stays in its bootloader and enumerates as
a separate USB device, `EGG Bootloader`. Run `egg-fw info` to confirm, then
flash again — it writes from the start, which is the vendor tool's own recovery
path. Note that `70-endgamegear.rules` must be installed for this to work: the
bootloader is a different product ID and needs its own rule.

> Only the OP1w 4k (v1) has been flashed end to end with this tool's protocol
> verified against a capture of a real update. The v2's product IDs are known
> and the sequence is believed identical, but it has not been done; `egg-fw`
> says so before it starts. Neither XM2w model is supported at all, because
> nothing establishes their bootloader identities and guessing at a device
> about to be overwritten is not worth it.

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

The protocol layer is derived from static analysis, 44 USB captures and
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
