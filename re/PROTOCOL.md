# Endgame Gear OP1w 4k / XM2w 4k — USB configuration protocol

Covers both generations. Decoded on an OP1w 4k v2 and checked against an
OP1w 4k v1; section 12 is the complete list of what differs.

Reverse-engineered for interoperability: enough to write an independent Linux
configuration tool. Every claim is tagged with how it was established:

- **[BIN]** — read out of the vendor tool's disassembly
- **[HID]** — read from the device's own HID report descriptor
- **[CAP]** — confirmed by USB capture (differential analysis, 44 captures)
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
   1     1    status:  0x01 = ready/OK,  0x03 = busy — retry,
                         0x07 = not applicable on this connection (§13)
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
| `0x0E` | `A1 0E 00 00` | read | 14 B | **mouse info** — the mouse's own VID/PID and firmware |
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
+2   u8   lift-off distance     v2 ONLY — v1 uses whole millimetres, see §12
                                index from 0.7 mm in 0.1 mm steps:
                                value = round(mm × 10) − 7   (1.0 mm = 3, 1.5 mm = 8)
                                range 0.7-1.7 mm, i.e. 0..10 — see §12
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

### Glass mode changes the lift-off scale **[BIN]** **[CAP]**

On the v2, **sensor glass mode selects which lift-off encoding is in use.** The
tool's combo builder `FUN_00411280` branches on a byte that only the glass-mode
control writes:

| glass mode | list | wire value |
|---|---|---|
| off | eleven entries, 0.7–1.7 mm | `round(mm×10) − 7`, i.e. `0x00`–`0x0A` |
| on | two entries, 1.0 mm / 2.0 mm | **whole millimetres, `1` or `2`** |

It rewrites the stored byte across the switch, in both directions: entering
glass mode maps `≤7 → 1` and `8..10 → 2`; leaving it maps `1 → 3` and
`2 → 10`.

This retires a long-standing loose end. §4 previously recorded that "enabling
sensor glass mode makes the vendor tool drop LOD from 1.0 mm to 0.8 mm" and
called it a UI quirk. It is not a quirk and nothing was dropped: capture
`35_SensorGlassModeOffToOn` shows the tool writing cmd `0x14` `+2 = 0x01` with
the same capture's cmd `0x15` carrying glass mode `= 1`. LOD was 1.0 mm, which
is byte `3` on the tenths scale; the tool rewrote it to `1`, meaning **1 mm on
the millimetre scale**. Reading that `1` back as tenths is what produced the
phantom "0.8 mm". **[CAP]**

> **The device does not reinterpret the byte — the vendor's label is
> cosmetic.** Tested on an OP1w 4k v2, firmware 1.07, probing with a 0.76 mm
> ISO/IEC 7810 card: the mouse rests on the card and is slid across a gap to a
> second one, giving a steady 0.76 mm standoff rather than a hand-held lift.
> **[DEV]**
>
> | | lift-off byte | glass | tracking at 0.76 mm |
> |---|---|---|---|
> | A | `0x02` | **on** | marginal — only at some angles |
> | B | `0x02` | off | solid |
> | C | `0x0A` | off | solid |
>
> A and B differ only in the glass bit. If the device read `0x02` as 2.0 mm
> under glass mode, A would have had the **highest** lift-off of the three —
> above C's nominal 1.7 mm. It had the lowest. So the scale switch is the
> vendor tool's UI convention, and what actually changed in A is glass mode's
> own effect on the sensor, which measurably lowers lift-off at a fixed byte.
>
> Two limits on that. B and C did not differ, so this probe saturates above
> 0.76 mm and says nothing about the byte's effect *within* the tenths scale.
> And strictly, reinterpretation combined with a glass-mode penalty large
> enough to more than halve lift-off would fit the same three observations —
> possible, but it has to explain why 2.0 mm reads lower than 0.9 mm.
>
> **Practical consequence:** toggling glass mode in the vendor tool changes the
> user's real lift-off distance twice over — once because the tool rewrites the
> byte (1.0 mm becomes `1`, which is 0.8 mm on the scale the device actually
> uses), and again through glass mode's own effect. It is not a no-op, and it
> is not announced.
>
> **What this means for a port.** No vendor tool ever writes `0x03`–`0x0A` to a mouse in glass mode, so
> a client that does is writing an encoding with no precedent. This tool
> follows the switch: `effectiveLodEncoding(model, glassMode)` resolves the
> scale, `lodConvertForGlassMode()` translates the byte with the vendor's own
> mapping, and because the glass bit lives in cmd `0x15` while the lift-off
> byte lives in cmd `0x14`, both blocks are written together **whenever glass
> mode changes** — writing one alone would leave the device holding a lift-off
> value in the other scale, and either Apply closes the gap so it cannot be
> split by using the "wrong" tab. Note how narrow that condition has to be:
> writing `0x14` on *every* `0x15` write would restate the active CPI stage and
> the CPI records from a possibly stale copy, silently reverting a stage the
> user had just changed with the button under the mouse.
>
> The conversion is lossy and not an involution — `0`, `1` and `2` all map to
> `1` on the way in, and `1` maps back to `3` — so toggling glass mode twice
> does not restore the original lift-off. The vendor behaves the same way; this
> tool at least names the byte it changed.
>
> Note this makes lift-off the one setting whose encoding is **not** fixed per
> model — so `ModelInfo::lod` is necessary but not sufficient.

### cmd `0x15` — polling, power & click filters, **11**-byte payload on v2, **10** on v1

```
+0   u8   motion sync           0 / 1
+1   u8   polling / power mode  see the table below
+2   u8   flags                 bit0 = slamclick filter      (both)
                                bit4 = motion jitter filter  (v1 only, §12)
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

> The eleventh byte is v2-only: the v1 tool writes ten and has no glass mode.
> Sending ten to a v2 is NOT "omit the field" — the report is zero-filled and
> the v2 acts on `+10` regardless of the declared length, so ten bytes means
> "glass mode off". Know the model before writing this block. See §12.

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

One concrete lead remains: the vendor tool **does** read this region —
`FUN_004041e0` copies blob `0x0F`–`0x22` into the settings object at
`+0x6E`–`+0x81`. **[BIN]** Finding what dereferences `obj+0x6E` would settle it,
and unlike the LED test it needs no hardware.

What the region is *not* is now settled: it is not what paints the vendor UI.

### The stage → colour mapping is fixed, and hardcoded in the tool

```
CPI 1  blue      CPI 2  green      CPI 3  yellow      CPI 4  red
```

The colour belongs to the **stage**, not to the stage's CPI value:

- All five vendor binaries — OP1w 4k v1.03 and v1.04, OP1w 4k v2 v1.02, XM2w 4k
  v1.03, XM2w 4k v2 v1.02 — contain the identical four-instruction sequence
  writing this palette, in stage order, into four swatch controls at stride
  `0x9C`. The palette is a literal, not a read: **[BIN]**

  ```
  c7 86 7c 10 00 00  00 00 ff 00    MOV [ESI+0x107C], 0x00FF0000   blue
  c7 86 18 11 00 00  00 ff 00 00    MOV [ESI+0x1118], 0x0000FF00   green
  c7 86 b4 11 00 00  ff ff 00 00    MOV [ESI+0x11B4], 0x0000FFFF   yellow
  c7 86 50 12 00 00  ff 00 00 00    MOV [ESI+0x1250], 0x000000FF   red
  ```

  (v1.04 at file offset `0x00C282`, v2 at `0x00C695`, both XM2w likewise.
  `COLORREF` is `0x00BBGGRR`, so these read blue, green, yellow, red.)
- The vendor UI keeps CPI 2's swatch green with that stage set to **1480** CPI,
  so the swatch does not track the value. **[UI]**
- It matches the LED on hardware. **[DEV]**

Endgame Gear's public documentation lists the colours against 400 / 800 / 1600 /
3200 — but it is describing the factory defaults ("come pre-programmed with
default … levels, which are color-coded as follows"), and those are exactly the
default values of stages 1–4. It is not a value→colour rule.

Since the mapping is a literal in every binary and identical across all four
models, a port should hardcode it too rather than trying to derive it from the
blob. `kStageColours` in `src/egg/protocol.h` does.


### Read responses

| cmd | payload | notes |
|---|---|---|
| `0xB4` | `+0` battery %, `+1` signal level, `+2` target (`0x0F`) | `0x41` = 65 %, matching the UI exactly. Same three bytes a battery notification carries — see §4a |
| `0x0D` | `+0..+1` = dongle firmware | `01 01` = v1.01 |
| `0x0E` | `+0..+1` VID, `+2..+3` **mouse PID**, `+4..+5` PID−1 **[?]**, `+6..+7` firmware | v1 `67 33 72 19 71 19 01 08`, v2 `67 33 84 19 83 19 01 07` **[DEV]** |

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

The dispatcher in the v1 tool (`FUN_00415b70`, reached from the listener
through the callback in `DAT_00583430`) switches on exactly **four** codes —
`0x06`, `0x0E`, `0xB1`, `0xB4` — and a code outside that set reaches a
do-nothing path, so the vendor software drops it. It reads byte 1 as the code,
byte 2 as a sub-value and bytes 3–4 as a little-endian short. **[BIN]** The
same four are dispatched by the v1.04 and XM2w v1.03 tools; the v2 tools add a
fifth arm, `0x02`, which they read as the 0-based active CPI stage and stage
into cmd `0x14` payload `+7`. Whether any device ever *emits* `0x02` is
unestablished — it appears in none of the 44 captures. **[BIN]** **[?]**

Two of the v1's four are not in the captures above:

- **`0x06` — polling rate changed on the mouse.** Byte 2 is the polling byte
  and only `0x08`/`0x04`/`0x02` are handled; the tool selects the matching
  combo entry and stores the raw value into its settings struct. So the mouse
  can change its own polling rate and say so. **[BIN]**
- **`0x0E` — mouse info.** The tool compares bytes 3–4 against `0x1972`, its
  own mouse PID, and greys the whole control set when they differ. Note this is
  bytes 3–4, not the `+2..+3` of a cmd `0x0E` *response*; the event's own layout
  is not established. **[BIN]**

It also accepts `0xF1` as a second link-down sub-code alongside `0xF0`. **[BIN]**

**`0x30` is observed but undecoded.** It is in the captures, four times, all
8 bytes on endpoint `0x82`, and no vendor tool dispatches on it: **[CAP]**

```
03 30 ff 00 08 00 00 00   41_PairDongle, ~10.0 s after the cmd 0x70
03 30 09 00 1b 00 00 00   42_OnOffOnOffCycle
03 30 0e 00 1b 00 00 00   42_OnOffOnOffCycle
03 30 00 00 1b 00 00 00   43_OnOffOnOffCycle
```

Byte 2 varies (`0xFF`, `0x09`, `0x0E`, `0x00`), byte 3 is always `0x00`, and
byte 4 is `0x08` for the pairing case against `0x1B` for the power cycles —
the position a battery event uses for the target selector. Every sighting is
during link establishment, which is suggestive but not enough to name it.

**`0x31` is observed but undecoded.** An OP1w 4k v1 emitted
`03 31 14 10 00 00 00 00` immediately after a cmd `0x14` write. **[DEV]** It
appears in none of the 44 v2 captures, and **no vendor tool dispatches on it** —
so it is dropped by the vendor software too, and cannot be an error the tool
would need to act on. Under the layout above it decodes as code `0x31`,
sub-value `0x14`, short `0x0010`; `0x14` being exactly the command that had
just been written is suggestive of a write-acknowledgement, but one sample
cannot distinguish that from a coincidence. The decisive test is cheap: run
`egg-cli listen` on a v1 and apply a `0x15` change, then a `0x16` change.

**That test has now been run, and it rules the acknowledgement out.** One cmd
`0x14` write (`set cpi`), one `0x15` (`set slamclick`) and one `0x16`
(`map forward mouse back`), in that order, produced **two** events and both
were byte-identical to the original sighting: **[DEV]**

```
03 31 14 10 00 00 00 00
03 31 14 10 00 00 00 00
```

No `03 31 15 …`, no `03 31 16 …`. Byte 2 is `0x14` in every sighting to date,
so if it names a command it names that one specifically — this is not a
per-block write acknowledgement.

Isolating each write in its own `listen` session settles the trigger: **[DEV]**

| write | block | events |
|---|---|---|
| `set cpi 1 400` | cmd `0x14` | 1 |
| `set slamclick on` | cmd `0x15` | 1 |
| `map forward mouse forward` | cmd `0x16` | 0 |

So the earlier run's two events were one from the `0x14` and one from the
`0x15`; nothing fires it twice. Two things follow.

**Byte 2 is not the command number.** It is `0x14` even when the write was cmd
`0x15`. The whole payload is invariant — `14 10 00 00 00 00` in every sighting,
whichever of the two blocks was written and whatever value changed.

**The trigger is a sensor reconfiguration.** Cmds `0x14` and `0x15` are exactly
the two blocks that reprogram the sensor — CPI, lift-off, polling rate, motion
sync, the filters — while `0x16` only rewrites the button table and touches no
sensor register. The event reads as the mouse announcing that its sensor was
re-initialised, with a constant payload. That is an inference from the trigger
set, not a decode: nothing establishes what `14 10` denotes, and because no
vendor tool has a dispatch arm for `0x31` there is no disassembly to decode it
against. **[?]**

**It is a genuine generational difference.** The same test on a v2 — `listen`
plus `set cpi 1 600` — produced **no event at all**, while `show` confirmed the
stage had changed from 400 to 600. So the write landed and the silence is real,
not an artefact of nothing having been written. **[DEV]**

That upgrades a claim that had been resting on weak ground. The original
"appears in no capture" evidence was nearly worthless here: only 8 of the 44
files contain any packet on the notification endpoint, and **none of the 16
carrying a cmd `0x14` write recorded that endpoint at all**, so the captures
never had the opportunity to show a v2 emitting it. The direct test replaces an
absence of evidence with evidence of absence.

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

   > The corollary bites if you build this tool anywhere else: on Windows the
   > interface is split into one device path per collection, each with its own
   > read queue, so a `hid_read` on the config collection never sees a
   > notification. `MI_01` of the dongle presents as four HID children —
   > `Col02` (config, usage `0xFF01`/`0x02`) plus `Col03`, `Col04`, `Col05`.
   > **`egg-cli listen` therefore returns nothing on a Windows build**, not
   > even a battery event after a `0xB4`, and that is a property of the
   > platform rather than a fault in the device or the tool. Reproducing it
   > there would need a second handle opened on the `0xFF02`/`0x0001`
   > collection, exactly as the vendor tool does. **[DEV]**

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
4. ~~cmd `0x0E` response bytes `+4..+5`~~ — **resolved.** They are the mouse's
   **bootloader/DFU product ID** (`0x1971` on v1, `0x1983` on v2 — the mouse PID
   minus one). The v1 firmware updater opens exactly two USB identities, and all
   nine call sites of its device-open helper are immediately preceded by a
   literal `MOV EDX,imm32` of either `0x1972` (application) or `0x1971`
   (bootloader). **[BIN]** See `firmware/FIRMWARE.md` §4.
5. Commands `0x71` ("Pair Default") and `0x72` ("Get pair data") — present in
   the binary but unreachable from this version of the UI.
6. Blob bytes `0x0F`–`0x22`: four RGB colours in 5-byte records with an
   incrementing index (§4). They are the CPI LED's four colours, but not in
   stage order — reading them positionally is disproved against the LED — so
   what dereferences them is still open. The vendor UI is no longer a lead:
   its swatches are a hardcoded literal, not a read of this region (§4).
   Nothing this tool does depends on it. Blob byte `0x02`, formerly thought
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
> The OP1w 4k and OP1w 4k v2 pairs are both confirmed against hardware; the two
> XM2w entries are inferred from identical code structure.

So **over the dongle, USB IDs do not identify which mouse is attached** — a tool
sees `3367:1970` whichever of the four is paired. That is resolved, but not by
USB: **cmd `0x0E` reports the mouse's own VID/PID**, which is the discriminator.
See §12.

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
binaries are the same size to the byte.

> An earlier revision of this section, written from string dumps alone, said
> the delta was three v2-only features and nothing else. **That was wrong**, in
> two ways that matter for writing bytes. Strings are present in a binary
> whether or not its UI uses them, and a shared field can still carry a
> different encoding. The corrected, hardware-checked comparison is in §12:
> v1 additionally has a **motion jitter filter** (flags bit 4, a string the v2
> binary also carries but never displays), and **lift-off distance uses an
> incompatible scale** between the generations.

The undeclared eleventh byte in §2 is explained by the split: glass mode is the
v2-only field at payload `+10`, one past the declared ten. **Ten was the truth
on v1** — its tool writes exactly ten — **and v2 appended a byte without
updating the length field.** Confirmed in both binaries. **[BIN]**

### What this means for supporting them

Superseded by §12, which has the hardware-checked delta and the cmd `0x0E`
discriminator. Discovery by PID is necessary but nowhere near sufficient: the
lift-off scale and the polling option set differ, so the model must be known
*before* those bytes are written, and over the dongle only cmd `0x0E` supplies
it.

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

---

## 12. The v1 generation, and telling the models apart

The OP1w 4k (v1) was tested on hardware alongside the v2. The two are **not**
feature-subset compatible: two fields use different encodings, so a client that
treats v1 as "v2 minus some checkboxes" will write wrong values.

### Identifying the model

Both dongles enumerate as `3367:1970` with identical descriptors, revision,
strings (empty serial, product "Endgame Gear HS Dongle") and collection layout.
USB cannot tell them apart. **[DEV]**

**Cmd `0x0E` can.** Its payload carries the mouse's own USB identity:

```
v1  67 33 72 19 71 19 01 08     VID 0x3367  PID 0x1972  fw 1.08
v2  67 33 84 19 83 19 01 07     VID 0x3367  PID 0x1984  fw 1.07
```

`+2..+3` is the mouse PID, and it matches the cabled PID extracted from each
vendor binary (§10). That is the discriminator. **[DEV]**

It fails while the mouse is asleep, so a client cannot always identify the model
at startup — it must either wait for a link-up notification (§4a) or refuse
model-specific writes until it can.

### What differs

| | v1 (OP1w 4k, XM2w 4k) | v2 (OP1w 4k v2, XM2w 4k v2) |
|---|---|---|
| Mouse PID (`0x0E` +2) | `0x1972` / `0x1968` | `0x1984` / `0x1982` |
| **LOD** (cmd `0x14` +2) | **millimetres: `1`, `2` only** | **`round(mm×10) − 7`, 0.7–1.7 mm** |
| Polling (cmd `0x15` +1) | `0x08`/`0x04`/`0x02` only | plus `0x80` (1000 Hz power saving), `0x40` (125 Hz office) |
| Flags bit 4 (`0x10`) | **motion jitter filter** | unused |
| Flags bit 5 (`0x20`) | unused | multiclick acknowledgement |
| Flags bit 6 (`0x40`) | unused | force max sensor FPS |
| cmd `0x14` +5 | never written, stays `0x00` | sensor angle tuning |
| cmd `0x14` +7 | **written, but ignored by the device** | active CPI stage |
| Event `0x31` after a `0x14`/`0x15` write | **emitted** | not emitted |
| CPI range the tool allows | 50–26000, step 50 | **10–30000**, step 10 below 10000 |
| Lift-off encoding | fixed | **depends on glass mode** — see §4 |
| cmd `0x15` payload | **10 bytes** written | **11 bytes** written |
| cmd `0x15` length byte | `0x0A` | `0x0A` (under-declares) |

> **The lift-off scales are incompatible, not merely offset.** Under v1's
> encoding the bytes `1` and `2` mean 1.0 mm and 2.0 mm; under v2's they would
> mean 0.8 mm and 0.9 mm. No value is safe under both readings, which is why a
> client must know the model before writing that byte. **[BIN]**

> **The v1 cannot switch CPI stage from software.** Selecting another stage and
> applying changes nothing, while editing the active stage's CPI takes effect
> at once. **[DEV]** The v1 tool agrees in an unusual way: its cmd `0x14`
> serializer `FUN_00404d00` *does* write payload `+7` — from settings `+0x08`,
> which the config-blob parser fills from blob `0x0D` and which **nothing else
> in the binary ever writes**. `RangeXrefs` over the settings struct finds a
> single reference to `0x00583858`, the Apply handler passing its address as
> the payload base; the two writes to the field are the blob parser
> (`FUN_00404230`) and the defaults filler (`FUN_00416120`). The Basic Settings
> populate `FUN_0040fa10` does not read it either. So the v1 tool echoes the
> device's own byte back on every Apply, and its Basic tab has no radio buttons
> beside the CPI rows. **[BIN]** **[UI]**
>
> Because writes are whole-block there is no way to omit `+7`. A client must
> send back exactly what it read — which is what `hasCpiStageSelect = false`
> makes `harvestSensor()` do. The stage is changed with the CPI button under
> the mouse instead; §4's colour table is how the user tells which one is live.

### The vendor's own changelog confirms the list — and reframes it

Shipped beside the v2 firmware updater
(`OP1w4kv2/firmware/instructions_changelog.txt.txt`), the v2 mouse firmware
changelog reads, verbatim:

```
Version 1.02:
- Added: Force max Sensor fps option
- Added: Sensor Angle Tuning
- Added: 1000Hz Power Saving and 125Hz Office Mode to the Polling Rate dropdown
- Added: CPI switching via software
```

That is four of this section's differences, independently confirmed from the
vendor, and it matches `hasForceMaxFps`, `hasAngleTuning`, `hasPowerSavePolling`
and `hasCpiStageSelect` exactly. Notably it states in the vendor's own words
that software CPI switching is something the firmware *gained* — corroborating
the hardware result that a v1 ignores cmd `0x14` `+7`.

> **But they are firmware capabilities, not model capabilities, and `kModels`
> keys on the PID.** A v2 still running firmware 1.01 would be detected as
> capable of all four and is not. Nothing in this project has ever seen such a
> device — both test mice are well past those versions — and cmd `0x0E` does
> report the firmware version at payload `+6..+7`, so the check is available if
> it ever matters. Recorded as a known limitation rather than fixed
> speculatively. **[?]**

The same file's update instructions ("unplug USB-C from the dongle, plug USB-C
into the mouse") independently confirm the firmware path is **cable-only**,
which `firmware/FIRMWARE.md` established from the disassembly. **[UI]**

The v1 tool's LOD combo is populated from a `DLGINIT` resource holding exactly
`"1mm"` and `"2mm"`, and the only instructions writing that settings byte
produce `0x01` or `0x02`; anything unexpected on read displays as `1mm`. The v2
tool builds its list in code instead — and that list is **eleven** entries,
0.7 mm to 1.7 mm, not the 0.7–2.0 mm the formula would extend to. Its combo
builder `FUN_00411280` issues exactly eleven `CB_ADDSTRING` calls, for the
literals at `0x0055E070`–`0x0055E0E8` (`"0.7mm"`…`"1.7mm"`). **[BIN]**

So `0x0B`–`0x0D` are bytes **no vendor tool has ever written**, and this tool
does not write them either: `kLodMaxIndexV2` caps the list at `0x0A`.

> That same function has a second branch, and it is **not** a v1 list — see
> "Glass mode changes the lift-off scale" in §4. It matters here because it is
> where the vendor converts between the two encodings, which pins the maximum:
> coming back to the tenths scale it maps a stored `2` to **`10`** — its own
> maximum, 1.7 mm — rather than to the `13` the formula would extrapolate for
> 2.0 mm. The vendor clamps; it does not extend. **[BIN]**

### What is identical

Command set and header bytes, blob offsets, the 5-byte CPI records, the 8×7
button table and its type codes, SPDT `0xF0`/`0xF1`, the multiclick filter
bytes, both inactivity timeouts, pairing, factory reset, and the CPI stage LED
colours (§4).

Two caveats on that list. The **notification channel** is identical in
transport and framing, but the *event set* is not: a v1 emits `0x31` after
every `0x14` and `0x15` write and a v2 emits nothing, tested directly on both
(§4a). **[DEV]** The tools differ too — the v1's dispatches `0x06` and accepts
`0xF1` (§4a), and the v2 tool's own dispatcher has never been decompiled, only
its listener loop. The **lift-off option sets**
and **polling option sets** differ in length and order, so a combo index from
one generation is meaningless on the other; only the wire values transfer.

### Consequence for a port

Read cmd `0x0E`, map `+2..+3` through a model table, and gate:
the LOD scale, the polling option set, flag bits 4/5/6, cmd `0x14` `+5`, cmd
`0x14` `+7`, and the cmd `0x15` payload length. Until `0x0E` answers, treat the
model as unknown and refuse those writes rather than guessing — every other
setting (CPI, timeouts, click filters, button mapping) is model-independent and
safe meanwhile.

Two of those are gates on *offering* the setting rather than on the write:
`+5` and `+7` are sent on both generations because the block is written whole.
A client keeps them correct by never letting the UI change a field the model
does not implement, so the value read from the device is the value written
back.

---

## 13. Cabled operation

A mouse connected by USB-C instead of through the dongle speaks the same
configuration protocol, and this tool now supports it: `egg-cli info`, `show`
and `blob` all work against a cabled OP1w 4k v1. **[DEV]**

### Two commands are refused, with a status code we had not seen

Sweeping every command against every target byte on a cabled v1 gives:

| cmd | target `0x00` | `0x01` | `0x0F` |
|---|---|---|---|
| `0x0F` probe | **`0x07`** | **`0x07`** | **`0x07`** |
| `0x0D` dongle info | **`0x07`** | **`0x07`** | **`0x07`** |
| `0x0E` mouse info | `0x01` | `0x01` | `0x01` |
| `0xB4` battery | `0x01` | `0x01` | `0x01` |

So **`0x07` is a fourth status code**, meaning something like "not applicable on
this connection". Both commands it rejects are about the radio link: `0x0D`
reports the dongle's firmware, and `0x0F` selects which device behind the
dongle a command is for. Over a cable there is no dongle for either to talk
about. **[DEV]**

### The target byte is ignored, not wrong

An earlier version of this section guessed that a cabled mouse wanted target
`0x00`, by analogy with the firmware updater, which is cable-only and uses
`0x00`. That guess was wrong. `0x0E` and `0xB4` answer **identically for all
three target values**, so a cabled mouse ignores the selector entirely — there
is nothing to select. Nothing needs to change about how commands are addressed.

### What actually had to change

One thing: the liveness probe. `Device::probe()` issued only cmd `0x0F`, which
is precisely one of the two commands a cabled mouse refuses, so `open()`
rejected a perfectly healthy device. It now falls back to `0x0E`, which answers
on both connections.

The fallback is tried **second**, so the wireless path is unchanged — including
the case that matters, a sleeping mouse, where `0x0F` answers from the dongle
and `0x0E` does not (§12).

### What a cabled mouse reports differently

- **No dongle firmware.** `0x0D` is refused, so that field is simply absent.
- **Battery reads 100 % and the signal figure is meaningless.** The mouse is
  charging, and there is no radio link to measure. Observed `0x64` (100) and a
  signal byte drifting around `0x75`.
- `0x0E` payload `+6..+7` still carries the firmware version, which is how the
  model is identified either way.

### A formatting bug this uncovered

The firmware version is **hex digits, not a decimal pair**: 1.10 is `0x0110`,
1.08 is `0x0108`. That is why the vendor's updater formats the whole
`bcdDevice` with `"%x"` before dividing by 100 (`firmware/FIRMWARE.md` §5).

`Version::toString()` printed the minor byte as decimal, so firmware 1.10 came
out as **"1.16"**. The bug was invisible for the life of the project because
every firmware seen until now — 1.01, 1.07, 1.08 — has a minor digit of 9 or
less, where the two readings agree. Updating a mouse to 1.10 is what exposed
it. **[DEV]**

### Writing is confirmed too

All three write blocks were exercised on a cabled OP1w 4k v1 and read back at
the byte level, then restored to a blob byte-identical to the one they started
from: **[DEV]**

| block | blob byte | change | meaning |
|---|---|---|---|
| cmd `0x14` | `0x24` | `90 01` → `f4 01` | CPI stage 1, 400 → 500 |
| cmd `0x15` | `0x06` | `11` → `10` | slamclick cleared; bit 4 untouched |
| cmd `0x16` | `0x54` | `10` → `08` | forward button, Forward → Back |

Two things worth drawing out. The `0x15` write moved **only** the bit asked
for, leaving the v1-only motion-jitter bit 4 alone — the whole-block read-
modify-write discipline behaving as intended on a live device. And all three
commands carry `Target::Mouse` (`0x0F`), the *wireless* selector, unchanged
from the dongle path: they work anyway, which confirms from the write side
that a cabled mouse ignores the target byte rather than wanting a different
one.

So cabled support needs nothing beyond the probe fallback described above.
