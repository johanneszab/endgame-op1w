# Endgame Gear firmware update protocol

How the vendor's firmware updaters flash an OP1w 4k, and everything a
reimplementation needs. `src/fw/` (`egg-fw`) is that reimplementation; it has
performed complete updates of both generations.

Subjects:

| binary | size |
|---|---|
| `OP1w4k/Endgame_Gear_OP1w_4k_Firmware_Updater_v1.10-660998ec.exe` | 2,481,664 |
| `OP1w4kv2/firmware/Endgame Gear OP1w 4k v2 Firmware Updater v1.07.exe` | 2,785,280 |

Claims are tagged as elsewhere in this project: **[BIN]** disassembly,
**[HID]** report descriptor, **[CAP]** USB capture, **[DEV]** observed on
hardware, **[?]** inference. §7 says which parts are proven and which are not.

---

## 1. Summary

| Question | Answer |
|---|---|
| Is there an embedded payload? | Yes — a **PE resource**, locatable without a fixed offset **[BIN]** |
| Does the host transform it? | **No.** Plain byte copy, `LockResource` → `HidD_SetFeature` **[CAP]** |
| Is it checksummed? | Yes — a 16-bit additive sum per 1024-byte block, in the request **[CAP]** |
| Is there a whole-image checksum? | **No.** None host-side, and no command carries one **[BIN]** |
| Flashed over radio? | **No — cable only.** The dongle PID is never opened **[BIN]** |
| Is a failed flash recoverable? | **Partly established** — the recovery path works and both test mice came back through it **[DEV]**, but no half-written image has ever been tested (§5) |
| Is it one-shot? | **No.** No version gating; it re-flashes on demand, same version included **[DEV]** |
| Must a Linux flasher break the encryption? | **No.** The device decrypts; the host is a pipe **[CAP]** |

The image is encrypted and stays encrypted. The host reads a resource out of its
own `.exe` and streams it verbatim in 1024-byte blocks — verified by reassembling
205 captured blocks into a byte-identical copy of that resource (§7). **The whole
problem is therefore identification, not cryptography:** picking the right image
out of several decoys, and the right hidraw node out of several that look alike.

## 2. The payload

```
type   "FWFILE"   (string-named resource type, language 2052)
size   209,920 bytes (0x33400) = 205 × 1024 exactly — every candidate
```

For the v1 v1.10 updater the flashed resource is `FWFILE/134` at file offset
`0x001D29E8`, RVA `0x1D91E8`, sha256
`ad612be22f77907162429e1053a6fad53c91bd59a7f0916c9715970e56aa2b27`.

The file's **overlay is 0 bytes** (the last section ends exactly at EOF) and the
SECURITY directory is `(0,0)` — not Authenticode-signed. Nothing is appended;
everything lives in `.rsrc`. **[BIN]**

Block count is `SizeofResource() >> 10` (`SHR ECX,0x0A` @ `0x4041B8`); a nonzero
remainder would be zero-padded and the count incremented. Neither the length nor
the offset is stored anywhere else — a scan for `0x33400` and `0xCD` finds no
relevant hits. **[BIN]**

### The trap: which resource

Each updater carries **several FWFILE resources of identical size**, and the one
it flashes is neither the first nor the last: **[BIN]**

| updater | FWFILE ids present | flashed | code site |
|---|---|---|---|
| OP1w 4k v1.10 | 134, 135 | **134** | rva `0x4190`, thunk `0x567298` |
| OP1w 4k v2 v1.07 | 134, 135, 136, 137, 139 | **136** | rva `0x32B7`, thunk `0x51B230` |

Every naive rule fails on the second sample — lowest picks 134, highest picks
139, largest ties five ways, and a fixed file offset is meaningless. The ID is a
hardcoded `PUSH` feeding the only `FindResourceW(NULL, <id>, L"FWFILE")` in the
binary (v1: `PUSH 0x86` @ `0x00404195`, with the `L"FWFILE"` literal at VA
`0x0059ADF0` having exactly one code reference; there is no `PUSH 0x87`
anywhere). The code site moved between the two binaries, so an
instruction-pattern scan is doing real work rather than matching a constant
layout.

**An extractor must recover the ID from the code and refuse to run if it cannot
pin exactly one.** `re/tools/extract_fw.py` does that — stdlib-only Python, runs
anywhere, no hardcoded offsets. It matches

```
68 <VA of L"FWFILE">   PUSH string
68 <imm32>             PUSH resource id      <- reads this
6a 00                  PUSH 0
ff 15 <IAT slot>       CALL [FindResourceW]
```

and takes the size from the resource directory, never from a constant. It fails
loudly unless the scan pins exactly one ID, so a recompile with different
argument marshalling would refuse rather than guess. `src/fw/pe.cpp` is the C++
port `egg-fw` uses; both agree on both updaters.

All seven images across the two updaters are 209,920 bytes and **mutually
distinct** (sha256), so size never discriminates and none is a duplicate. They
fall into shape classes by record structure, which fingerprints the *plaintext*
since each image is keyed separately:

```
135 distinct records, most repeated ×71 : v1/134, v1/135, v2/134, v2/135
138 distinct records, most repeated ×68 : v2/136, v2/137
136 distinct records, most repeated ×70 : v2/139
```

The pairing is suggestive — ids arrive two at a time with matching shape, and the
flashed one is the lower of its pair — but **nothing establishes what the spares
are for**, and a wrong pick would flash a valid-looking image built for something
else. What the extractor does *not* rest on is any claim about a specific number:
134 is not "the firmware id". **[?]**

### Encryption

Entropy 7.9788 bits/byte over the whole image, all 256 byte values present, 742
zero bytes (uniform expectation ≈820). Not compressed, not plaintext, no
container magic — first bytes `b2 06 aa d6 5b ca 25 f8`. Structure:

```
records 0..132     unique
records 133..203   71 records, all byte-identical
record  204        unique
```

71 identical incompressible kilobytes rule out compression, and show the
transform **resets at every 1024-byte boundary with no chaining across
records**. Within a single record no 16-byte sub-block repeats (and none up to
64 bytes), so it is not 16-byte ECB. Those 71 identical ciphertexts go to 71
*different* destination indices, so the state is not derived from the destination
either. **[BIN]**

A repeating 1024-byte XOR keystream was tested and **refuted**: deriving a
candidate key from the filler record and applying it leaves the rest at random
entropy. The mode is otherwise unknown — consistent with a chained or counter
mode re-initialised per record. Images 134 and 135 share no records at all, not
even the filler, so they are keyed differently. **[?]**

No crypto primitives exist in the binary at all: no AES S-box or T-table, no
CRC32 table, no MD5/SHA IVs, no TEA delta, no CryptoAPI or bcrypt imports —
across the full 2.4 MB, and the same for the config tools. **[BIN]**

Consequence for integrity: the encryption provides **none**. Nothing chains
across block boundaries, so truncation, dropped blocks and reordering all still
decrypt normally. It prevents forgery, not damage.

## 3. The protocol

Same transport as the config channel: `HidD_SetFeature`/`HidD_GetFeature`
(`hid_send_feature_report`/`hid_get_feature_report`), report **`0xA0`**,
**1041 bytes** (`EDX = 0x411` at every call site), payload at `+16`, status at
response `+1`. **[BIN]** + **[CAP]**

> The bootloader's status codes differ from the config protocol's:
> **`0x01` = OK, `0x04` = busy** (`FUN_00401F50` re-reads with increasing delay
> up to ~1 s). The config protocol's busy code is `0x03`.

| Step | Request | Host's check |
|---|---|---|
| a. enter bootloader | `A0 3A 00 00 \| 00 5A A5 32 \| zeros` | none; waits for re-enumeration |
| b. re-enumerate | close, open the bootloader PID, retry 300 ms up to 3000 ms | else "Open bldr device request failed" |
| c. echo test | `A0 01 01 00` + `fw[0:1024]` at `+16` | byte 1 == `0x01` **and** `+16..` echoed byte-for-byte |
| d. start | `A0 03 00 00`, `[16]` = block count (**low byte only**), `[17..20]` = u32 | byte 1 == `0x01` |
| e. write block ×205 | `A0 06`, `[2..3]` = LE16(index + `0x34`), `[4..5]` = sum16, `[16..1039]` = 1024 B | byte 1 == `0x01`, else resend (5 tries) then abort |
| f. complete | `A0 09 00 00` + zeros | byte 1 == `0x01` |
| g. re-enumerate | open the application PID, retry 500 ms up to 5000 ms | else "Update failed, try again" |
| h. post-update | `A1 13 00 00` on the **64-byte** `0xA1` report | reply read, not inspected |

Step (a) carries its magic at `+4`, **not** inside the `+16` payload. Step (h) is
cmd `0x13` — **factory reset** (`PROTOCOL.md` §3). *The updater wipes the user's
configuration after flashing;* read and save the config blob first.

### The reboot command's own result means nothing

Step (a) is the one request whose outcome cannot be read from the request. The
device stops being the device as it obeys, and the two host platforms fail in
*opposite* directions:

| | what the host sees when the command **works** |
|---|---|
| Windows | `HidD_SetFeature` returns SUCCESS; only the following `GET_REPORT` is cancelled **[CAP]** |
| Linux | `ioctl(HIDIOCSFEATURE)` returns **`-ETIMEDOUT`** after 5 s **[DEV]** |

On Linux the kernel waits for the control transfer's STATUS stage, which a
rebooting device never sends, so it waits out `USB_CTRL_SET_TIMEOUT` (5 s) and
reports `Connection timed out`. **An error there is the normal outcome of a
command that succeeded.** Observed on an OP1w 4k v1 on bare metal: the ioctl
returned that error while `dmesg` showed the mouse disconnecting and coming back
as `3367:1971`.

A refusal and a success are therefore indistinguishable from the request on
either OS. **Only "did the bootloader appear?" can tell them apart**, so that is
the only thing a reimplementation should branch on. `egg-fw` shipped with this
wrong — it trusted the Windows behaviour and reported "the mouse refused the
enter-bootloader command; nothing has been written and it is still running its
firmware", three false claims about a mouse already sitting in DFU.

Step (c) is read-only and runs *before* the erase, which makes it the right place
to prove the transport works. No capture contains one — the only captured update
entered through the recovery branch, which skips it — so its framing came from
the disassembly alone, and it worked unmodified on the first run. **[DEV]**

Step (d) **erases the application region while holding its reply.** The device
took 3.9 s to answer in the capture. A reimplementation must not read a slow
reply here as a refusal, and must not tell the user nothing has been written: by
the time that reply is late, the old firmware is already gone. **[CAP]**

### The block header

```
[0]        0xA0
[1]        0x06
[2..3]     LE16 destination index = block + 0x34      MOV [base+2],CL @ 0x402655
[4..5]     sum16 of [16..0x40F]                       @ 0x4026FF / 0x40270E
[6..15]    0x00
[16..1039] 1024 firmware bytes                        REP MOVSD, ECX=0x100
[1040]     0x00
```

`[2..3]` is **one 16-bit little-endian field**, not the config protocol's
`target`/`length` pair — proven by `ADD ECX,0x34` @ `0x404952` with the block
counter in ECX, so the field runs `0x0034`..`0x0100` and crosses the byte
boundary at block 204. Confirmed on the wire. Base `0x34` puts the image at
flash offset `0x34 × 1024 = 0xD000`. **[BIN]** + **[CAP]**

The checksum (`FUN_00402640`) is a plain 16-bit wrapping **additive** sum of the
1024 payload bytes, written little-endian into `[4..5]` and zero-extended to a
dword at `[4..7]`. **[BIN]**

Because each block carries its own destination index, **retries are safe and
idempotent** — a resend cannot land in the wrong place. But a 5th consecutive
failure **aborts mid-image** with no rollback and no restart, leaving the device
in the bootloader with a partial write. **[BIN]**

### The reply says more than the vendor tool reads

The host looks only at byte 1 and discards the rest. Every block acknowledgement
in fact carries: **[CAP]**

```
50 01 34 00 00 00 65 fb 00 ...
│  │  └──┬──┘       └──┬──┘
│  │     │             └──── the block's 16-bit checksum, echoed
│  │     └────────────────── the block's destination index, echoed
│  └──────────────────────── status, 0x01 = OK
└─────────────────────────── 0x50, not an echo of the 0xA0 request id
```

All 205 replies echoed both the index and the checksum correctly. **A
reimplementation can therefore verify each block landed where it was aimed and
that the device summed the same payload** — a stronger check than the vendor's.
`egg-fw` makes it inside the retry loop, so a transient costs a resend rather
than an abort. Byte 0 is `0x50` on every bootloader reply; the one
config-protocol reply in the trace (`A1 13`) instead echoes `0xA1`.

### The start command's `[17..20]` is uninitialised memory

It comes from `this+0x3EA60`, referenced *exactly once* in the entire 1.46 MB
`.text` — the read at `0x4047E8`. Nothing writes it: it is a
declared-but-never-assigned member of a stack object whose constructor
initialises only the vptr, three CStrings and an icon. **[BIN]**

On the wire it is `b4 fe ef 00` — `0x00EFFEB4`, a stack address. **[CAP]**

```
a0 03 00 00 ... [16]=cd  [17..20]= b4 fe ef 00
```

So **the device accepted an update with garbage in that field**: it is neither a
length nor a checksum, and a reimplementation may put anything there. `egg-fw`
sends zero.

### Timing

Coded sleeps in the updater: 20·n, 70, 150, 200, 500, 1000, 1080 ms. Measured in
the capture: blocks ran t=21.9 s → 56.7 s, i.e. 205 blocks in ~34.8 s or ~170 ms
each against the coded 70 ms; start → first block 4.2 s; `complete` → the
application device reappearing 19.7 s of guest-observable time. **[CAP]**

That capture ran in a VM, so part of the overhead is passthrough rather than the
mouse. The conclusion is one-directional and safe either way: **the vendor's
coded sleeps are not minimums.** A budget with no margin turns a *successful*
flash into a reported failure, which invites the user to run another
erase-and-write cycle. `egg-fw` therefore waits far longer than the vendor at
every step — 30 s for DFU to appear, 45 s for re-enumeration — because waiting
costs nothing and a false failure does not.

**How long the bootloader takes to appear, measured on bare metal** (Fedora,
`xhci_hcd`, OP1w 4k v1, from `dmesg`): **[DEV]**

```
18707.337  usb 1-3: USB disconnect              <- the mouse obeys step (a)
18712.528  usb 1-3: new full-speed USB device   <- +5.19 s
18712.853  idProduct=1971  Product: Bootloader  <- +5.52 s
18712.968  hid-generic ... hidraw0              <- +5.63 s, usable
```

So a host tool needs **~5.6 s** from losing the mouse to having a node it can
open — and **the vendor's 3 s budget cannot make it**, since that budget starts
at the send, which is at or before the disconnect. This does not separate the
device's boot time from the kernel's enumeration, and it does not need to: the
figure a reimplementation must cover is the whole interval.

This is the first *controlled* measurement of that interval. An earlier claim of
the same shape was withdrawn as a VM artefact (§7), and the 30 s `egg-fw` waits
was justified on the open-retry argument above rather than on any number. The
number now agrees with it.

## 4. The three USB identities

| device | VID:PID | what it is |
|---|---|---|
| dongle | `3367:1970` | **never** a firmware target; see §6 |
| mouse, application | `3367:1972` (v1), `3367:1984` (v2) | running its firmware |
| mouse, bootloader | `3367:1971` (v1), `3367:1983` (v2) | in DFU, `iProduct` "EGG Bootloader" |

The bootloader PID is the application PID minus one, and is also readable from a
running mouse: cmd `0x0E` payload `+4..+5` (`PROTOCOL.md` §8). There are exactly
9 call sites to the updater's device-open helper `FUN_00401C00`, each immediately
preceded by a literal `MOV EDX,imm32`: `0x1972` at five, `0x1971` at four. **The
updater knows exactly two identities.** **[BIN]** Both bootloader PIDs have since
been seen to enumerate. **[DEV]**

Device matching inside that helper is VID `0x3367` + PID + `HidP_GetCaps`
UsagePage `0xFF01` / Usage `0x02`. **[BIN]**

### The bootloader's descriptors **[CAP]** **[HID]**

```
DEVICE   VID 3367  PID 1971  bcdDevice 0021  iProduct "EGG Bootloader"
CONFIG   1 interface, 100 mA, bus-powered
IFACE    #0  alt 0  1 endpoint  class 03 sub 01 proto 02  (HID, boot, mouse)
HID      bcdHID 0111  wDescriptorLength 68
ENDPOINT 0x81  interrupt IN  64 bytes  1 ms
```

The 68-byte report descriptor, read from a v1 in DFU via
`/sys/class/hidraw/hidrawN/device/report_descriptor`, has **two** top-level
collections:

```
05 01  09 02  a1 01        Generic Desktop / Mouse, Application
  85 01                      report ID 1
  09 01  a1 00              Pointer, Physical
    05 09 19 01 29 08          buttons 1..8
    15 00 25 01 95 08 75 01 81 02
    05 01 09 30 09 31          X, Y
    15 80 25 7f 75 08 95 02 81 06
  c0  c0
06 01 ff  09 02  a1 01     VENDOR 0xFF01 / usage 0x02, Application
  85 a0                      report ID 0xA0
  75 80  95 41               size 128 bits × count 65 = 1040 bytes
  15 00 25 01 09 22 b1 03    FEATURE
c0
```

Three things follow. **The 1041-byte report is confirmed from the bootloader's
own descriptor**, not merely from the application's. **There is no report `0xA1`
here** — the bootloader carries `0xA0` only, so the closing factory reset cannot
be sent to it, which is why the vendor sends `A1 13` only after re-enumeration.
And the interface declares the boot-mouse subclass and protocol while carrying
vendor feature reports, so **do not infer the usage page from the interface
class.**

### Node selection is the second trap

The application exposes five HID collections and the dongle four; Linux gives one
hidraw node per USB *interface*, Windows one path per *collection*. Reports
`0xA0`/`0xA1` are carried by exactly one node, and it is not the first: of six
nodes for `3367:1972`, `GET_REPORT(0xA0)` answers on one. **[DEV]**

On the bootloader there is only one node — but three things make a
descriptor-based filter the wrong tool anyway:

- **The vendor collection is the SECOND top-level collection.** hidapi reports a
  single `usage_page`/`usage` pair per node, commonly from the *first*
  collection — here Generic Desktop / Mouse (`0x0001`/`0x0002`). A client testing
  `usage_page == 0xFF01` may simply not match. **[HID]**
- **The bootloader's one interface is numbered 0**, so an "interface 1" fallback
  cannot match either. **[CAP]**
- **An unsolicited `GET_REPORT` fails on the bootloader.** It works on the
  application and is refused in DFU — on a device that then flashed perfectly.
  Consistent with the device holding its last response (CLAUDE.md invariant 6):
  every `GET` in the capture follows a `SET`, and a freshly entered bootloader
  has no last response to hold. **[DEV]**

So: **rank** nodes by descriptor as a hint, with a catch-all rank so none is
excluded outright, and accept on a *functional* test. In DFU the only sound
functional test is the echo test (§3 step c) — a `SET` followed by a `GET`, which
is the pattern the device implements and the vendor uses.

Both halves of this were predicted by an adversarial review of `egg-fw` before
the descriptor was read, and the descriptor confirms the filter would have fired
on the first real Linux run: the tool would have put a mouse into DFU and then
been unable to reopen it.

## 5. Recoverability, and what is not known

**The updater has a purpose-built recovery path.** In the button handler
`FUN_004040F0` it probes the application PID first; failing that it probes the
bootloader PID, and if the *bootloader* answers it calls straight into the block
loop — skipping both `A0 3A` and the echo test. A vendor writes that branch only
because devices do end up stranded in DFU and must be recoverable by re-running
the tool. The abort-and-do-nothing behaviour on block failure is only sane under
the same assumption. **[BIN]**

**That branch works:** both of `egg-fw`'s complete flashes ran through it, and
both mice came back (§7). **[DEV]** Note what that does and does not show — in
both cases the mouse had been put into DFU *deliberately*, by a clean `A0 3A`,
and was holding an intact application image. Flashing a mouse stranded by a
failure part-way through a write is the same code path, but it starts from a
state nothing here has produced on purpose.

**No version gating.** The version is read from `HidD_GetAttributes`'
`VersionNumber` (bcdDevice ÷ 100), formatted into a label, and never compared to
anything. There is no "already up to date" string in the binary. Clicking the
button always flashes, so re-flashing the installed version is legitimate — and
is the natural repair action. **[BIN]** + **[DEV]**

**The version is hex digits, not a decimal pair** — 1.08 is `0x0108`, 1.10 is
`0x0110`, which is why the updater formats `bcdDevice` with `"%x"` before the
divide. Reading the minor byte as decimal renders 1.10 as "1.16" and stays
invisible until a minor digit exceeds 9. **[CAP]**

**Cable only.** PID `0x1970` (the dongle) is never passed to the open helper, and
none of the wireless machinery appears — no `0x0F` target probe, no `0x0E`, no
`0xB4` — and the enter-bootloader command carries target byte `0x00`, not `0x0F`
("mouse via dongle"). The failure mode of losing the 2.4 GHz link mid-write does
not exist on this path. **[BIN]**

### What is still unknown — the load-bearing gaps

- **Whether the bootloader survives a half-written application.** The only
  *proven* route into it is `A0 3A`, handled by the *running application*. If a
  partial image will not run, recovery depends on the bootloader staying in DFU
  by itself. That is the standard pattern, and the recovery branch strongly
  implies it, but no partial write has been tested. Every "you can re-run this"
  message in `egg-fw` is hedged for exactly this reason.
- **Whether the device validates the decrypted image before jumping to it.**
  Nothing host-side answers this, and it is the difference between "retry" and
  "brick". After `A0 09` the host waits ~5 s for the application PID and
  otherwise prints "Update failed, try again" — which is what a bootloader
  *refusing* a bad image looks like, and equally what a bootloader that *jumped*
  to a corrupt one looks like. Indistinguishable from outside, opposite
  consequences.
- What the spare FWFILE resources are for (§2).
- Record 204 (destination index `0x100` = flash `0x40000`, exactly 256 KB, one
  page past an application region of `0x34`..`0xFF`) is plausibly a
  trailer/metadata slot — where a whole-image CRC would live if one existed. It
  is encrypted, so this is **speculation**. **[?]**

**Do not deliberately interrupt a flash to test recoverability.** A clean
end-to-end trace now exists, so the information that would buy is small and the
stake is a mouse.

## 6. Hazards for a reimplementation

> **The dongle, the mouse application and the bootloader all expose the same
> vendor collection `0xFF01`/`0x02` and the same report IDs.**

`PROTOCOL.md` §1 correctly tells a *configuration* tool to select its hidraw node
by usage page. For flashing that is dangerous: the same logic can attach to
`3367:1970` — the dongle — and stream 205 KB of mouse firmware at it. The dongle
is a different chip from a different bootloader vendor (`0x22D4`, PIDs
`0x1502`/`0x1503`) with its own updater, shipped as the `USBDATA` resource inside
every config tool (90,112 bytes, a complete 32-bit PE, byte-identical across all
five tools, sha256 `db06d7af…`). Bricking the dongle bricks all wireless use of
the mouse.

**`egg::Device`'s node-selection logic must not be reused for flashing without a
hard VID/PID gate.** `src/fw/` is deliberately not linked against `egg` for this
reason, and gates on VID *and* PID before usage is looked at at all.

The rest, in rough order of how badly they end:

1. **Never emit a destination index below `0x34`.** The vendor flow never does,
   and whether the device range-checks one is unknown. Below it lies the
   bootloader.
2. **Require the image to be exactly 205 blocks.** A short one would be written
   *and completed*, leaving the device half new and half old — and nothing
   downstream can detect that, because the encryption resets every block (§2).
3. **Never accept a raw image with a model named on the command line.** The image
   identifies nothing by itself; the updater `.exe` does, and checking that
   against the mouse in front of you is the only thing standing between a v2
   image and a v1 mouse.
4. **Don't report "nothing was written" after step (d).** See §3.
5. **Expect to be unable to watch the reboot inside a VM.** The bootloader is a
   *different USB device*, so a hypervisor rule forwarding one VID/PID stops
   forwarding at exactly the moment the update begins, and from inside the guest
   the device vanishes mid-conversation. This looks exactly like the mouse
   refusing the command when in fact it obeyed. Forward the **physical port**, or
   work on bare metal.

## 7. What is verified, and what is not

Two captures, in `re/captures/firmware/`, parsed by `re/tools/parse_fwcap.py`:

| capture | what it covers |
|---|---|
| `fw_op1w4k_v108_to_v110.pcap` | 30 packets: the `A0 3A` enter-bootloader request, and the device leaving |
| `fw_op1w4k_v108_to_v110_from_bootloader.pcap` | 896 packets: a complete v1 1.08 → 1.10 update, via the recovery branch |

**The feasibility claim is proven.** The 205 block payloads from the complete
capture, reassembled in index order, are byte-identical to the resource
`extract_fw.py` pulls out of the updater: **[CAP]**

```
205 / 205 records match,  0 mismatches
length          209,920 = resource length
sha256 on wire  ad612be22f77907162429e1053a6fad53c91bd59a7f0916c9715970e56aa2b27
sha256 resource ad612be22f77907162429e1053a6fad53c91bd59a7f0916c9715970e56aa2b27
```

**`egg-fw` has performed complete updates of both generations** — an OP1w 4k v1
1.08 → 1.10, and an OP1w 4k v2 reflashed 1.07 over 1.07 — each verified by
reading `bcdDevice` back afterwards rather than assumed. Every step in §3 has now
been executed by that code, including `A0 01`, which no capture contains.
**[DEV]**

Two caveats on that, kept because they are the difference between evidence and a
green tick:

- **Both of those runs entered through the recovery branch**, with the mouse
  already in DFU. `flash()`'s application-mode branch — reboot and flash in one
  invocation — has since been run on bare metal, and **it did not complete**: the
  mouse rebooted correctly, but `egg-fw` misread the reboot request's own result
  as a refusal and gave up (§3). Flashing it a second time succeeded. The
  misreading is fixed; one invocation carrying the whole sequence has still not
  been observed to finish.
- The v2 run was a same-version reflash *without* a before-value, so "reports
  firmware 1.07" is, on its own, the same output a wholly failed write would
  give. What makes it evidence is the v1 run, where the version demonstrably
  changed.

Also unexercised: **the entire retry and back-off machinery.** 205/205 blocks
succeeded on both runs, with no resend, no busy status and no short reply. That
code decides what happens when a flash goes wrong, and it has never run.

### Reference vector

For validating a reimplementation against v1 firmware 1.10 without a device.
Start carries `[16]` = `0xCD` = 205; the blocks are: **[CAP]**

| block | `[2]` | `[3]` | `[4]` | `[5]` |
|---|---|---|---|---|
| 0 | `34` | `00` | `65` | `fb` |
| 1 | `35` | `00` | `90` | `f8` |
| 2 | `36` | `00` | `a1` | `f9` |
| 3 | `37` | `00` | `86` | `fb` |
| 4 | `38` | `00` | `96` | `ef` |
| 5 | `39` | `00` | `85` | `e7` |
| 203 | `ff` | `00` | `05` | `00` |
| 204 | `00` | `01` | `23` | `06` |

and `[16..1039]` == `FWFILE_134.bin[index*1024 : index*1024+1024]`.

### Three corrections worth keeping

Each was a confident prediction the hardware contradicted, and together they are
why this document tags its claims rather than asserting them.

**`[17..20]` was predicted `00 00 00 00`**, on the reasoning that Windows
zero-fills fresh stack pages. It is `b4 fe ef 00` (§3). The load-bearing half of
the claim survived and got stronger: the field is uninitialised, and the device
accepts garbage in it.

**The post-update `bcdDevice` was predicted `0x6E`**, i.e. 110 decimal. It is
`0x0110` (§5). The mechanism described was right; the predicted value applied it
wrongly.

**One [DEV] tag was withdrawn**, because the mistake behind it is easy to repeat.
An earlier revision said "3 s is not long enough — the v2 took longer to
re-enumerate". `egg-fw` did time out at 3 s on a v2 that had rebooted correctly,
but that run was inside a VM, and §6.5 is precisely why the guest never saw the
device. **No re-enumeration time was measured at all** — the number described
passthrough, not the mouse. The 30 s wait `egg-fw` uses rests on the argument in
§3 instead. Measuring how long DFU really takes to appear needs bare metal, or a
hypervisor forwarding the physical port.
