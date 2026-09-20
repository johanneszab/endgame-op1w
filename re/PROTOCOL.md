# Endgame Gear OP1w 4k v2 — USB configuration protocol

Reverse-engineered for interoperability: enough to write an independent Linux
configuration tool. Every claim is tagged with how it was established:

- **[BIN]** — read out of the vendor tool's disassembly
- **[HID]** — read from the device's own HID report descriptor
- **[CAP]** — confirmed by USB capture (differential analysis, 43 captures)
- **[DEV]** — confirmed against live hardware on Linux: a setting is changed
  with `egg-cli` and the 1024-byte blob diffed before and after
- **[UI]** — read off the vendor tool's own interface
- **[?]** — inference, not yet confirmed

Source binary: `Endgame_Gear_OP1w_4k_v2_Configuration_Tool_v1_02-61846009.exe`
SHA-256 `D17C973AB415852BB6FB7C7CB30106C3F4F02DCC614B8B869A5D3A1EA97FA391`,
v1.0.2.0, 32-bit MFC, statically linked **hidapi**.

Reference device: mouse firmware 1.07, dongle firmware 1.01.

---

## 1. Device topology

Dongle: **VID `0x3367` / PID `0x1970`**, "Endgame Gear HS Dongle", USB composite,
2 interfaces, 6 HID top-level collections. **[HID]** **[BIN]**

| Node | Usage page / usage | Reports | Role |
|---|---|---|---|
| MI_00 | `0x0001` / `0x0002` Mouse | In `0x01`, 8 B | movement (16-bit X/Y), wheel, AC Pan, 8 buttons |
| MI_01 COL01 | `0x0001` / `0x0006` Keyboard | In `0x02`, 8 B | keystrokes emitted by remapped buttons |
| **MI_01 COL02** | **`0xFF01` / `0x0002` vendor** | **Feat `0xA1` 64 B, Feat `0xA0` 1041 B** | **configuration channel** |
| MI_01 COL03 | `0x000C` / `0x0001` Consumer | In `0x06`, 3 B | media keys from remapped buttons |
| MI_01 COL04 | `0xFF02` / `0x0001` vendor | In, 8 B | async events |
| MI_01 COL05 | `0xFF02` / `0x0002` vendor | In, 64 B | async events |

The tool selects its endpoint by walking all HID interfaces, matching
`HidD_GetAttributes` VID/PID, then `HidP_GetCaps` with
`UsagePage == 0xFF01 && Usage == 0x02`. **[BIN]** `FUN_004034e0`

> Match on usage page/usage, not on interface or collection index — that is the
> portable way to find this node on Linux too.

The binary also opens **`0x3367` / `0x1984`** on one code path, guarded by a
connection-mode flag the UI renders as *Wired* / *Wireless* — the mouse's own
identity when cabled. A port should try both.

`0x1970` is **not** specific to this model: all four wireless tools open it, so
the dongle alone does not say which mouse is paired. See §10.

---

## 2. Transport

All configuration traffic is **HID feature reports** on the `0xFF01/0x02`
collection — `HidD_SetFeature` / `HidD_GetFeature`, i.e. USB control transfers
`SET_REPORT` / `GET_REPORT`, report type `Feature`. Interrupt endpoints are not
used for configuration. **[BIN]** **[CAP]**

| Report ID | Total size | Use |
|---|---|---|
| `0xA1` | 64 B (1 ID + 63) | command / response — everything except the config blob |
| `0xA0` | 1041 B (1 ID + 1040) | bulk: the 1024-byte config blob (cmd `0x12`) |

### Request framing (report `0xA1`) **[BIN]** **[CAP]**

```
offset  size  field
   0     1    report ID, always 0xA1
   1     1    command
   2     1    target selector: 0x0F = mouse, 0x01 = dongle, 0x00 = n/a
   3     1    payload length
   4     2    zero
   6     1    chunk index, 1-based (cmd 0x16 only)
   7     9    zero
  16    ..    payload
  ..    ..    zero padding to 64 bytes
```

> **The length field is not always the truth.** Cmd `0x15` writes **eleven**
> payload bytes but puts `0x0A` (ten) in byte 3, and the device acts on the
> eleventh. Replicate both exactly. **[BIN]** `FUN_00404b80` **[CAP]**

### Response framing **[BIN]** **[CAP]**

Write a 64-byte buffer with byte 0 = `0xA1` (or `0xA0` for the bulk report),
issue `GET_REPORT`, and the device fills it in:

```
offset  size  field
   0     1    report ID echo
   1     1    status:  0x01 = ready/OK,  0x03 = busy — retry
  16    ..    payload
```

**Payload always begins at offset 16**, both directions.

Two things to know about responses:

- The device **holds the last response** until a new command is issued. Reading
  twice returns the same bytes. **[CAP]**
- Bytes past a response's meaningful length are **stale content from the
  previous response**, not zero padding. Only trust as many bytes as the command
  defines. **[CAP]**

### There is no checksum

Nothing is computed over the packet in the send path **[BIN]**, and every byte
past the declared payload length is zero in all captured writes. **[CAP]**

### Retry / pacing rules **[BIN]**

Not guessable from a capture; worth copying verbatim.

- **Write** (`FUN_00403740`): on `SET_FEATURE` failure, if `GetLastError()` is
  `0x15` (NOT_READY), `0x17` (CRC), `0x1D` (WRITE_FAULT), `0x57`
  (INVALID_PARAMETER) or `0x65B` (FUNCTION_FAILED), restore the buffer and retry,
  up to 4 attempts, 50 ms apart.
- **Read** (`FUN_00403810`): same error set, up to 3 attempts, 50 ms apart.
  Additionally, if the read succeeds but **status byte 1 is `0x03`**, sleep an
  increasing delay (`+200 ms` per round) and re-read, up to 3 more times, until
  status is `0x01`.
- Every command is wrapped in a process-wide critical section — the device does
  **not** tolerate interleaved commands. Serialise all access.
- Each command carries an explicit inter-command `Sleep` of 150–360 ms. Don't
  pipeline.

---

## 3. Command set **[BIN]** **[CAP]**

| Cmd | Header | Dir | Payload | Meaning |
|---|---|---|---|---|
| `0x0D` | `A1 0D 00 00` | read | 14 B | **dongle info** (firmware version) |
| `0x0E` | `A1 0E 00 00` | read | 14 B | **mouse info** (address + firmware version) |
| `0x0F` | `A1 0F 01 00` | probe | — | **connection probe / target select** |
| `0x12` | `A1 12 00 00` | read | **1024 B via report `0xA0`** | **read whole config blob** |
| `0x13` | `A1 13 00 00` | write | — | **factory reset** |
| `0x14` | `A1 14 0F 1C` | write | 28 B | **sensor & CPI block** |
| `0x15` | `A1 15 0F 0A` | write | **11 B** (declares 10) | **polling, power & click filters** |
| `0x16` | `A1 16 0F 1C` | write | 28 B × 2 chunks | **button table** |
| `0x70` | `A1 70 00 00` | write | — | **re-pair mouse to dongle** |
| `0x71` | `A1 71 00 00` | write | — | "Pair Default" — present in the binary, not reachable from the UI |
| `0x72` | `A1 72 00 00` | read | 16 B | "Get pair data" — likewise unreachable |
| `0xB4` | `A1 B4 00 00` | read | 3 B | **battery %, signal level, target** |

`0x13` is **factory reset**, not a commit — confirmed by capturing the Factory
Reset button. **[CAP]** See §5 on persistence.

The tool issues `0x0F` with target `0x01` before every real command, as a
liveness probe; if that fails it retries with target `0x0F`. **[BIN]** **[CAP]**
A port can skip it, but sending it costs nothing and matches the vendor tool.

### Startup sequence **[CAP]**

```
0F → 0D → 0F → 0E → 0F → B4 → 0F → 12
     dongle   mouse    battery   full config blob
```

Pressing **Pair Dongle** sends `0x70` and then repeats that whole read sequence
twice. Despite the name, what it does is re-establish the radio link — the mouse
drops and reconnects. Settings are unaffected. **[CAP]**

---

## 4. Payload layouts

### cmd `0x14` — sensor & CPI block, 28-byte payload

```
+0        ?                     always 0x00 in every capture
+1   u8   LED on lift-off       1 = LED lights on lift-off, 0 = disabled
+2   u8   lift-off distance     index from 0.7 mm in 0.1 mm steps:
                                value = round(mm × 10) − 7   (1.0 mm = 3, 1.5 mm = 8)
+3   u8   angle snapping        0 / 1
+4   u8   ripple control        0 / 1
+5   i8   sensor angle tuning   signed degrees: +10 = 0x0A, −10 = 0xF6
+6   u8   CPI levels            number of active CPI stages (1–4)
+7   u8   active CPI stage      0-based
+8   ..   4 × 5-byte CPI record:
            +0  u8    xySplit — 0 = Y follows X, 1 = axes independent **[DEV]**
            +1  u16le CPI X, in CPI units
            +3  u16le CPI Y, in CPI units
```

Factory defaults — 400/800/1600/3200, 4 levels, stage 1 active, LOD 1.0 mm:

```
00 01 03 00 00 00 04 00  00 90 01 90 01  00 20 03 20 03  00 40 06 40 06  00 80 0C 80 0C
```

**CPI is a plain little-endian `uint16` in CPI units** — no register encoding, no
scaling. Verified by 400→1000 (`0190`→`03E8`) and 800→1000 (`0320`→`03E8`). X and
Y are independent. **[CAP]**

The field order is independently confirmed by the serializer `FUN_00404ce0`,
which gathers the payload from struct offsets
`+0x02, +0x0A, +0x0B, +0x03, +0x04, +0x07, +0x01, +0x00`, then four CPI records
at stride 6. **[BIN]**

> Enabling sensor glass mode makes the vendor tool drop LOD as a side effect
> (1.0 mm → 0.8 mm in capture 35). That is UI behaviour, not a device rule.

### cmd `0x15` — polling, power & click filters, **11**-byte payload

```
+0   u8   motion sync           0 / 1
+1   u8   polling / power mode  see the table below
+2   u8   flags                 bit0 = slamclick filter
                                bit5 = multiclick filter enable
                                bit6 = force max sensor FPS
+3   u8   power saving          timeout in minutes; bit7 set = disabled
+4   u8   left button    ┐      0x01..0x0F = multiclick filter count
+5   u8   right button   │      0xF0 = SPDT "GX Safe Mode"   (left/right only)
+6   u8   middle button  │      0xF1 = SPDT "GX Speed Mode"  (left/right only)
+7   u8   back button    │
+8   u8   forward button ┘
+9   u8   deep sleep            timeout in minutes; bit7 set = disabled
+10  u8   sensor glass mode     0 / 1   — past the declared length, still honoured
```

Button order is left, right, middle, **back, forward** — the same order as the
button table, not the order the vendor UI lists them in. **[BIN]** The five
values sit at struct offsets `+0x34, +0x3C, +0x44, +0x4C, +0x54`, stride 8,
which is the stride of the button records. `FUN_00404b80` gathers the whole
payload from `+0x0F, +0x04, +0x07, +0x05, <the five>, +0x06, +0x10`.

Timeouts are 1–120 minutes in the UI. **[UI]**

The SPDT modes are offered on the **left and right buttons only**. **[UI]**

#### Polling rate **[CAP]** **[UI]**

The vendor dropdown combines rate with a wireless power-saving mode, so this is
an enum, not a divisor — 1000 Hz appears twice with different encodings.

| Value | Dropdown entry |
|---|---|
| `0x02` | 4000 Hz (no Power Saving) |
| `0x04` | 2000 Hz (no Power Saving) |
| `0x08` | 1000 Hz (no Power Saving) |
| `0x40` | 125 Hz (Office Mode) |
| `0x80` | 1000 Hz (Power Saving) |

Power saving is only offered at 1000 Hz and below, which is why 1000 Hz appears
twice. Four of the five values equal `8000 / rate`; the power-saving variant
does not, so treat the field as an enum rather than a divisor. This is entirely
separate from the *Power Settings* tab's inactivity timers.

### cmd `0x16` — button table, 2 chunks × 28 B = 8 entries × 7 B

Chunk 1 carries entries 0–3, chunk 2 entries 4–7, chunk index in header byte 6.

```
entry:  +0 u8  type
        +1 u8  code       (for keyboard: modifier mask)
        +2 u8  key        (keyboard HID usage, else 0)
        +3..+5            zero
        +6 u8  multiclick filter for this button — the same value cmd 0x15 writes
```

| type | meaning | code |
|---|---|---|
| `0x00` | mouse button | HID button bitmask: `01` L, `02` R, `04` middle, `08` back, `10` forward |
| `0x01` | wheel | signed delta: `01` = up, `FF` = down |
| `0x02` | keyboard | `+1` = HID modifier mask (bit1 = LShift), `+2` = HID key usage (`04` = `a`) |
| `0x09` | special function | `F1` = cycle CPI stage — the CPI button, not exposed in the vendor UI |
| `0x0C` | **FIXED CPI** — set a specific CPI | payload is a CPI record: `xySplit`, X `u16le`, Y `u16le` **[DEV]** |
| `0x20` | consumer control | HID consumer usage low byte (`EA` = Volume Decrement, `96` = browser, `94` = file manager) |
| `0xFF` | disabled | `00` |

Type `0x0C` explains the record's shape: the 5 bytes after the type byte are a
CPI record, which is why every other mapping leaves four of them zero.

> A decoder that keeps only bytes `+1` and `+2` will **truncate a FIXED CPI
> binding** the next time it writes the table. Carry all five.

Verified on hardware: the vendor tool's *FIXED CPI* dialog, with *X/Y Settings*
ticked and X = 600 / Y = 400, bound to the back button, reads back through this
layout as exactly 600/400. **[DEV]** The `xySplit` byte in that dialog is the
same flag the CPI stage records carry (§4), which is independent corroboration
of the naming taken from §9.

The vendor bind menu, which is the complete set of actions the UI can produce
**[UI]**:

| Menu | Entries | Protocol |
|---|---|---|
| MOUSE | left, right, middle, forward, back | type `0x00`, bitmask |
| MOUSE | scroll up, scroll down | type `0x01`, signed delta |
| KEYBOARD KEY | any key, with modifiers | type `0x02` |
| CPI | CPI LOOP | type `0x09`, code `0xF1` |
| CPI | FIXED CPI | type `0x0C` |
| MEDIA | play/pause, next, previous, mute, volume up, volume down, browser, explorer | type `0x20` |
| DISABLE | — | type `0xFF` |

Scroll up/down sit under MOUSE in the UI but use the wheel type on the wire, so
the menu grouping is not the protocol's grouping. The eighth table entry — the
CPI button — has no menu at all.

Entry order is left, right, middle, back, forward, special, wheel-up, wheel-down.
Confirmed by left-handed mode swapping entries 0 and 1, and by remapping the back
button (entry 3) and wheel-down (entry 7). **[CAP]**

**Left-handed mode is not a flag** — it is simply entries 0 and 1 swapped. **[CAP]**

> Because byte +6 is shared with cmd `0x15`, writing the button table with a
> stale value silently reverts that button's click filter. Keep them in step.

### cmd `0x12` — config blob, 1024 bytes via report `0xA0`

The device's stored configuration, and the **only** way to read settings back —
there are no per-setting read commands. Every writable field is in here, so a
port can do a true read-modify-write.

All offsets below validated by diffing blob reads against known device state. **[CAP]**

| Offset | Field |
|---|---|
| `0x01` | sensor angle tuning (i8) |
| `0x02` | unknown — `0x00` on the reference device |
| `0x03` | deep sleep — minutes, bit7 = disabled |
| `0x04` | power saving — minutes, bit7 = disabled |
| `0x05` | polling / power mode |
| `0x06` | flags (slamclick / multiclick / force max FPS) |
| `0x08` | LED on lift-off |
| `0x09` | lift-off distance |
| `0x0A` | angle snapping |
| `0x0B` | ripple control |
| `0x0C` | motion sync |
| `0x0D` | active CPI stage (0-based) |
| `0x0E` | CPI levels |
| `0x23` + 5·n | CPI stage n — `flag u8, X u16le, Y u16le` |
| `0x37` + 7·n | button entry n — identical format to a cmd `0x16` entry |
| `0x6F` | sensor glass mode **[DEV]** |

`0x03`–`0x06` and `0x0C` are confirmed by composing the blob parser
`FUN_004041e0` with the cmd `0x15` serializer through the shared settings
struct. **[BIN]** The rest are from blob diffs.

> **Sensor glass mode is at `0x6F`, not `0x02`.** `0x02` was an inference and it
> is wrong. Toggling glass mode moves byte `0x6F` — the byte immediately after
> the eight button records (`0x37 + 8×7 = 0x6F`) — and nothing else in the
> blob. This also settles the cmd `0x15` question in §2: the device really does
> act on the eleventh payload byte that the length field does not declare.
> **[DEV]** firmware 1.07.

### Bytes `0x0F`–`0x22` — four colours, but not a stage table

Twenty bytes that parse cleanly as **four 5-byte records**, the same stride as
the CPI table at `0x23`:

```
+0..+2  u8[3]  colour, R G B
+3      u8     flag, 0x01 throughout
+4      u8     index, 1-based and strictly increasing
```

On the reference device:

```
0x0F  ff ff 00  01  01     yellow
0x14  00 00 ff  01  02     blue
0x19  ff 00 00  01  03     red
0x1E  00 ff 00  01  04     green
```

The structure is not in doubt — the trailing bytes increment 1, 2, 3, 4 and the
three leading bytes are exactly the four colours the CPI LED uses. It is
tempting to read this as "stage n lights colour n", and that reading is
**wrong**: it was tested against the LED and disproved. **[DEV]**

| Stage | This table, read positionally | LED actually shows |
|---|---|---|
| CPI 1 | yellow | **blue** |
| CPI 2 | blue | **green** |
| CPI 3 | red | **yellow** |
| CPI 4 | green | **red** |

So the region holds the right four colours in the wrong order for a positional
stage lookup. Either the index byte means something other than a CPI stage, or
the table is a palette the firmware dereferences through a mapping that is not
in these twenty bytes. Nothing here is written by this tool.

Written out as a permutation, stage *n* uses record *p(n)*:

```
stage    1  2  3  4
record   2  4  1  3        (inverse: record 1 2 3 4 -> stage 3 1 4 2)
```

That is a single 4-cycle, not a rotation or a reversal, so it is unlikely to be
an off-by-one or an endianness slip — it looks like a genuine indirection.

Two concrete leads, neither followed yet:

1. The vendor tool **does** read this region: `FUN_004041e0` copies blob
   `0x0F`–`0x22` into the settings object at `+0x6E`–`+0x81`. **[BIN]** Finding
   what dereferences `obj+0x6E` would settle it, and unlike the LED test it
   needs no hardware.
2. The vendor UI's own swatches — blue, green, yellow, red beside CPI 1–4 —
   match the LED, not this table. So either they are hardcoded, or they are
   read through the same indirection the firmware uses.


### Read responses

| cmd | payload | notes |
|---|---|---|
| `0xB4` | `+0` battery %, `+1` signal level, `+2` target (`0x0F`) | `0x41` = 65 %, matching the UI exactly. Same three bytes a battery notification carries — see §4a |
| `0x0D` | `+0..+1` = dongle firmware | `01 01` = v1.01 |
| `0x0E` | `+0..+5` = 6-byte address **[?]**, `+6..+7` = mouse firmware | `01 07` = v1.07 |

---

## 4a. The notification channel

**The device is event-driven, not polled.** A four-minute idle capture spanning
both the power-saving (1 min) and deep-sleep (3 min) transitions contained
exactly two USB packets: one 8-byte interrupt IN and the URB that re-armed the
endpoint. No feature reports at all. **[CAP]**

That single event, on endpoint `0x82` — the `0xFF02` / `0x0001` collection:

```
report ID 0x03, 8 bytes:   03 B1 F0 0A 00 00 00 00
```

The vendor tool consumes it through a dedicated listener. `FUN_00416ad0` is a
blocking read loop on a **second** hidapi handle (`DAT_00587180`, distinct from
the feature-report handle `DAT_005862a4`), which pre-loads the buffer with
report ID `3`, reads 8 bytes, repacks bytes 1–7 and dispatches them to a
callback: **[BIN]**

```c
DAT_00587190 = 3;                                   // buffer[0] = report ID
while (DAT_00587180 != NULL &&
       hid_read_timeout(DAT_00587180, &DAT_00587190, 8, ...) != -1) {
    ...
    (*DAT_00586290)(...);                           // dispatch
    Sleep(10);
}
```

This is how the tool notices the mouse sleeping without ever polling: the
dongle pushes a notification when the radio link changes state. With the mouse
asleep, the tool shows `Connection: N/A`, blank mouse firmware and blank
battery, while **Dongle Firmware stays populated** — the USB link to the dongle
is unaffected; only the radio link to the mouse is down. **[UI]**

### Event format **[CAP]**

Two captures covering four sleep/wake cycles decode it completely. **Byte 1
reuses the feature-report command numbering, and the rest of the payload is
laid out like that command's response** — a notification is an unsolicited
command reply.

```
03 B4 <battery> <signal> 0F 00 00 00     battery / signal update
03 B1 01 00 00 00 00 00                  radio link up   (mouse awake)
03 B1 F0 0A 00 00 00 00                  radio link down (deep sleep)
```

| Byte | `0xB4` battery event | `0xB1` link event |
|---|---|---|
| 1 | `B4` — same code as the battery command | `B1` |
| 2 | battery percentage — `0x41` = 65 %, matching the UI | `0x01` up / `0xF0` down |
| 3 | signal level **[?]** — drifts between `0x29` and `0x3A` while in use | `0x00` up / `0x0A` down |
| 4 | `0x0F` — the mouse target selector, as in a request header | — |

Bytes 2–4 of a battery event are **byte-for-byte the payload of a `0xB4`
response**, which retrospectively decodes that response too: `+0` battery, `+1`
signal, `+2` target.

Timing confirms the link event is the deep-sleep timer firing. With deep sleep
set to 3 minutes, link-down arrived 181.8 s after the last mouse input in three
separate cycles. The 1-minute power-saving timeout produces **no** event — it
only lowers the report rate, the link stays up.

Battery events arrive unprompted on wake, and also ~47 ms after every `0xB4`
command — the dongle answers the feature report from cache, then pushes the
fresh value once the mouse replies.

Consequences for a port:

1. **Nothing needs polling.** Battery, signal and link state all arrive on their
   own. `0xB4` is only needed for an initial value at startup.
2. **Distinguish the two links.** `0x0D` (dongle info) answers while the mouse
   sleeps; `0x0E` (mouse info) and `0xB4` do not. That split is the cleanest
   test for "is the mouse awake" when no event has been seen yet.
3. **On Linux this is easier than on Windows.** One hidraw node covers the whole
   USB interface, so the notification collection shares a node with the config
   collection — no second handle is needed, just a read on the one already open.
   Input reports from remapped buttons (keyboard report `0x02`, consumer `0x06`)
   arrive on the same node and must be filtered out by report ID.

---

## 5. Persistence

**Settings persist across a mouse power cycle with no explicit commit.** A blob
read (`0x12`) after power-cycling the mouse returned byte-for-byte the same
configuration as before. **[CAP]** Writing `0x14`/`0x15`/`0x16` is sufficient;
do **not** send `0x13` expecting a save — it is a factory reset.

---

## 6. Feature surface

Everything the vendor tool exposes, and where it lives:

| Tab | Setting | Command |
|---|---|---|
| Basic | LOD, CPI levels, angle snapping, ripple control, LED on lift-off, CPI 1–4 with X/Y, active stage | `0x14` |
| Advanced | polling rate, motion sync, force max sensor FPS, sensor glass mode, sensor angle tuning, slamclick filter, multiclick filter, per-button click filter, SPDT | `0x14` + `0x15` |
| Power | power saving timeout, deep sleep timeout | `0x15` |
| Buttons | left-handed mode, remap of right/middle/forward/back/wheel-up/wheel-down | `0x16` |
| Pairing | Pair Dongle | `0x70` |
| Info | battery, firmware versions, factory reset | `0xB4`, `0x0D`, `0x0E`, `0x13` |

Notes:

- The vendor UI does **not** offer remapping the left button, though the
  protocol clearly supports it (entry 0). Nor does it expose entry 5, the CPI
  button.
- The CPI stage colours shown in the UI (blue/green/yellow/red) are fixed
  labels, not configurable.

### A note on the LED

This model has **one** LED — a small indicator on the underside showing battery
status, which also lights on lift-off. The only exposed control is
*Disable LED on Lift-Off* (cmd `0x14` `+1`).

It has a third job the vendor tool never mentions: pressing the **CPI button**
on the underside cycles the active CPI stage (wrapping 4 → 1, visible as blob
`0x0D`) and flashes the LED in that stage's colour. **[DEV]**

| Stage | LED |
|---|---|
| CPI 1 | blue |
| CPI 2 | green |
| CPI 3 | yellow |
| CPI 4 | red |

These are the same colours, in the same order, as the swatches the vendor UI
paints beside CPI 1–4 — so those swatches are accurate, and they are *not* what
the colour records at blob `0x0F` say (§4). The stage change is not announced on
the notification channel; it is only visible by re-reading the blob.

The binary's strings mention DPI/logo/scroll LEDs, LED effects and RGB colour
pickers (`CLedDlg`, "Apply led settings"). **None of that applies to this model** —
it is inherited dead code from the ODM's shared configuration tool. Don't build
UI for it.

---

## 7. Linux notes

The Windows tool is built on **hidapi**, so the port is close to mechanical:

| Windows | Linux |
|---|---|
| `HidD_SetFeature(h, buf, 64)` | `hid_send_feature_report(h, buf, 64)` → `HIDIOCSFEATURE` |
| `HidD_GetFeature(h, buf, 64)` | `hid_get_feature_report(h, buf, 64)` → `HIDIOCGFEATURE` |

Buffer byte 0 is the report ID in both cases, so the 64-byte layout above is used
unchanged. For cmd `0x12`, the response buffer is 1041 bytes with byte 0 = `0xA0`.

Find the node by enumerating `/dev/hidraw*` and picking the one whose report
descriptor declares usage page `0xFF01` / usage `0x02`
(`/sys/class/hidraw/hidrawN/device/report_descriptor`, or `hid_enumerate()`'s
`usage_page`/`usage` fields on recent hidapi).

> On Linux one hidraw node covers a whole USB **interface**, not one collection,
> so `hid_enumerate()` may report only the first collection's usage (the
> keyboard). Fall back to matching interface number 1 and confirm with a `0x0F`
> probe. Report IDs are unique within the interface, so `0xA1` still routes
> correctly.

No kernel driver needs detaching — `hidraw` coexists with `usbhid`, so the mouse
keeps working while the tool talks to it.

```
# /etc/udev/rules.d/70-endgamegear.rules
KERNEL=="hidraw*", ATTRS{idVendor}=="3367", ATTRS{idProduct}=="1970", TAG+="uaccess"
```

Suggested stack: **Qt 6 Widgets + hidapi (hidraw backend)**. Keep the protocol in
a UI-free static library with a synchronous, mutex-guarded `transact()`
implementing the retry rules in §2 — that mirrors the vendor tool's critical
section and is the part most likely to cause flaky behaviour if skipped.

---

## 8. Remaining unknowns

None of these block a working tool.

1. The signal-level byte in `0xB4` responses and battery events — range and
   unit unknown; it drifts between 41 and 58 while the mouse is in use.
2. The `0x0A` reason byte in a link-down event — constant across every captured
   cycle, so possibly a fixed "sleep timeout" code.
3. cmd `0x14` `+0` — always `0x00`, the last undecoded byte in the sensor block.
3a. `xySplit` — the field's *meaning* is confirmed by reading back a stage the
   vendor tool wrote (410/1480, axes visibly different), and writes from this
   tool round-trip correctly (see §11). What no test so far *isolates* is the
   byte itself: every observation is equally explained by "the device honours
   `xySplit`" and by "the device just uses X and Y and ignores the flag",
   because both predict the same behaviour whenever the flag agrees with
   `x != y`. Discriminating would mean deliberately writing `xySplit = 0` with
   X != Y and seeing whether Y is forced to X — which this tool will not emit,
   since it derives the flag from `x != y`. Academic for a port: the tool is
   correct under either reading.
4. cmd `0x0E` response bytes `+0..+5` — probably the pairing address.
5. Commands `0x71` ("Pair Default") and `0x72` ("Get pair data") — present in
   the binary but unreachable from this version of the UI.
6. Blob bytes `0x0F`–`0x22`: four RGB colours in 5-byte records with an
   incrementing index (§4). They are the CPI LED's four colours, but not in
   stage order — reading them positionally is disproved against the LED — so
   what dereferences them is still open. Blob byte `0x02`, formerly thought
   to be glass mode, is now unaccounted for.
7. Surface calibration and sensor power mode — named in the binary's strings but
   absent from the v1.02 UI.

---

## 9. Relationship to UnofficialEGGMouseConfig

[UnofficialEGGMouseConfig](https://github.com/niansa/UnofficialEGGMouseConfig)
is an independent open-source tool for Endgame Gear's **wired** mice — XM2 8k
(`0x1966`), XM2 8k v2 (`0x1980`), OP1 8k Standard (`0x1964`), OP1 8k Purple
Frost (`0x1976`) and OP1 8k v2 (`0x1978`). Same vendor ID `0x3367`, adjacent
product IDs, and it was reverse-engineered separately from this work.

### Where the two agree

Everything below was decoded independently on both sides and matches exactly —
strong evidence that both decodes are right.

| | |
|---|---|
| Report `0xA1` = 64-byte command, `0xA0` = 1041-byte bulk | identical |
| Cmd `0x12` read config, cmd `0x13` factory reset | identical |
| Blob `0x05` polling, `0x06` filter flags, `0x09` LOD, `0x0A` angle snapping, `0x0B` ripple control, `0x0C` motion sync, `0x0E` CPI levels | identical offsets and semantics |
| CPI table at blob `0x23`, 5-byte records, `u16le` in CPI units | identical |
| Button records 7 bytes; mouse bitmask `01/02/04/08/10`; type codes mouse `0`, wheel `1`, keyboard `2`, CPI-cycle `9`, consumer `0x20`, disabled `0xFF` | identical |
| SPDT `0xF0` safe / `0xF1` speed; multiclick counts below `0xF0` | identical |
| Polling divisor relationship `8000 / rate` | identical (but see below) |

### What this project gained from theirs

- The per-stage CPI flag byte is **`xySplit`** (§4). We only ever captured it as
  `0x00` because the vendor tool's *X/Y Settings* box was never ticked.
- Button mapping type **`0x0C`** binds a specific CPI value, with a full CPI
  record as its payload — which explains why a 7-byte record has four
  apparently-wasted bytes.
- Two more consumer usages: `0x96` browser, `0x94` file manager.

### Where the two differ — and why a PID is not enough

1. **Different write path.** They write the whole 1040-byte blob back with
   `storeConfig` = report `0xA0`, command `0x11`. This model is written with
   three separate block commands `0x14` / `0x15` / `0x16` on report `0xA1`;
   command `0x11` was never observed. Neither side has tested the other's.
2. **No wireless support at all.** Their tool has no battery, signal, pairing,
   sleep handling or notification channel — it is a wired-only design. That is
   most of what this model needs.
3. **Different firmware command.** Theirs is `0x02`, returning one version.
   This model uses `0x0D` (dongle) and `0x0E` (mouse) separately.
4. **Unmodelled fields.** Their `ConfigData` pads over blob `0x08` (LED on
   lift-off) and `0x0D` (active CPI stage), and has no notion of glass mode,
   force-max-sensor-FPS, the power-saving and deep-sleep timeouts, or
   left-handed mode.
5. **Filter flag bits differ.** They define bit 4 (`0x10`) as a motion jitter
   filter; on this model bit 5 (`0x20`) is the multiclick filter and bit 6
   (`0x40`) is force max sensor FPS. Bit 0 (slamclick) agrees.
6. **Polling is an enum here, not a divisor.** Their `polling_rate_divider`
   holds for four of the five values on this model, but 1000 Hz with wireless
   power saving is `0x80`, which is not `8000 / 1000`.
7. **Button table offset.** Their `ConfigData` places the table at blob `0x3D`
   with the multiclick byte first; on this model it starts at blob `0x37` with
   the type byte first. Our framing is pinned by a differential capture — a
   Shift+A binding on the **back** button landed at blob `0x4C`, which is
   entry 3 under our framing and misaligns under theirs. The two devices may
   genuinely differ, or one framing may be off by a record; it cannot be
   settled without their hardware.
8. **No retry or busy handling.** They issue single-shot feature reports with
   no `status = 0x03` back-off and no serialisation. See §2 — this model needs
   both.

---

## 10. The other wireless models

Static analysis of the vendor tools for the three sibling wireless mice. None
of that hardware is available here, so **everything in this section is from the
binaries only** — no capture, no device. **[BIN]**

Method: the tools call a cdecl `open(vid, pid)` helper, so the arguments appear
as adjacent `PUSH imm32` pairs. `re/tools/findimm.ps1` scans the raw bytes for
`68 67 33 00 00` (push `0x3367`) and reads the push before it. Validated
against the OP1w 4k v2 tool, where it recovers the two PIDs already confirmed
by hardware.

Confirmed by decompilation (`re/vidpid.txt`): all four references in each
binary are calls to the device-open helper, which is the function already known
to match on `HidD_GetAttributes` VID/PID and `UsagePage == 0xFF01`.

```
XM2w 4k v2   FUN_004034e0(0x3367, 0x1970)   FUN_004034e0(0x3367, 0x1982)
OP1w 4k      FUN_004035a0(0x3367, 0x1970)   FUN_004035a0(0x3367, 0x1972)
XM2w 4k      FUN_004035a0(0x3367, 0x1970)   FUN_004035a0(0x3367, 0x1968)
```

The XM2w 4k v2 tool calls the helper at **the same address as the OP1w 4k v2
tool**, `FUN_004034e0`, from the same three caller addresses — the two v2
binaries are the same build differing only in constants. The two v1 binaries
likewise share `FUN_004035a0` with each other.

### Product IDs

| Tool | Version | Dongle PID | Mouse PID (cabled) |
|---|---|---|---|
| OP1w 4k | v1.03 | `0x1970` | `0x1972` |
| OP1w 4k v2 | v1.02 | `0x1970` | `0x1984` |
| XM2w 4k | v1.03 | `0x1970` | `0x1968` |
| XM2w 4k v2 | v1.02 | `0x1970` | `0x1982` |

> **All four wireless models share dongle PID `0x1970`.** Each tool opens that
> same PID from three call sites, plus one model-specific PID from a fourth.
> Only the OP1w 4k v2 pair is confirmed against hardware; the rest is inferred
> from identical code structure.

The consequence is the awkward part: **over the dongle, USB IDs do not identify
which mouse is attached.** A universal tool sees `3367:1970` whichever of the
four is paired. Candidate discriminators, none tested: the 6-byte address in
the cmd `0x0E` response, the mouse firmware version, or an unidentified byte in
the config blob.

Cabled, the PID is unambiguous.

### The protocol is the same

Every command header byte-for-byte identical in all four binaries — same
commands, same target selectors, same declared payload lengths:

```
A1 0D 00 00   A1 0E 00 00   A1 0F 01 00   A1 0F 0F 00
A1 12 00 00   A1 13 00 00   A1 14 0F 1C   A1 15 0F 0A
A1 16 0F 1C   A1 70 00 00   A1 71 00 00   A1 72 00 00
A1 B4 00 00
```

`A0 11` — the whole-blob `storeConfig` used by UnofficialEGGMouseConfig (§9) —
is absent from all four.

### v1 versus v2

The two generations are separate builds (~1.954 MB vs 1.967 MB), and the v2
tools differ from each other only in constants: the OP1w 4k v2 and XM2w 4k v2
binaries are the same size to the byte. Their UI label sets are identical
within a generation, and v2 adds exactly three features over v1:

| Feature | Where it lives | v1 | v2 |
|---|---|---|---|
| Sensor angle tuning | cmd `0x14` payload `+5` | — | yes |
| Force max sensor FPS | cmd `0x15` flags bit 6 | — | yes |
| Sensor glass mode | cmd `0x15` payload `+10` | — | yes |

Everything else — CPI, LOD, angle snapping, ripple control, motion sync,
polling, both timeouts, click filters, SPDT, button mapping, left-handed mode —
is present in both.

That lines up suspiciously well with the undeclared eleventh byte in §2:
glass mode is the v2-only field that sits at payload `+10`, one past the
declared length of ten. The likeliest reading is that **ten was the truth on
v1, and v2 appended a byte without updating the length field**. **[?]**

### What this means for supporting them

Adding the cabled PIDs is enough for discovery, and this tool now does that
(`kModels` in `protocol.h`). Two caveats:

1. A v1 device must not be sent the three v2-only fields. `writePowerBlock()`
   sends ten payload bytes instead of eleven when the model has no glass mode,
   and both front-ends disable the other two.
2. Over the dongle the model is unknown, so the capability set falls back to
   v2 — the only one verified. Plugging a **v1 mouse in by cable** gets correct
   v1 handling; using it **wirelessly** would be treated as v2. Until a
   discriminator is found that gap cannot be closed, and nothing here has been
   tested on a v1 device.

---

## 11. Round-trip against the vendor tool

The strongest end-to-end check available without a bus analyser: write a
setting from this implementation on Linux, then read it in the vendor's own
Windows tool.

`egg-cli set cpi 2 1480 1480` — issued against a stage the vendor tool had
previously set to 410/1480 with *X/Y Settings* ticked — produced, in the vendor
tool: **CPI 2 = 1480 / 1480, X/Y Settings unticked**, everything else unchanged.
The mouse also stopped tracking at different speeds per axis. **[DEV]**

That exercises the whole cmd `0x14` path — header, target selector, payload
layout, CPI record encoding, little-endian `u16` values — and has it validated
by an independent implementation rather than by our own decoder. A framing or
offset error anywhere in that chain would have shown up as garbage in the
vendor UI.

The reverse direction was then checked the same way: a stage written from here
with X != Y tracks at different speeds per axis. So writes work in both
directions — combined to split and split to combined. **[DEV]**

Neither test isolates the `xySplit` byte; see §8 item 3a. That is a question
about the firmware, not about whether this implementation is correct.
