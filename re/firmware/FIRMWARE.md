# OP1w 4k (v1) firmware update — static analysis

Subject: `OP1w4k/Endgame_Gear_OP1w_4k_Firmware_Updater_v1.10-660998ec.exe`
(2,481,664 bytes). Nothing was run and nothing was flashed — this is a read of
the binary, cross-checked against the v1.03/v1.04 config tools.

**Verification status.** Five independent analyses completed and agree on
everything below. The adversarial verify pass did **not** run — it was cut off
by a usage limit. So these claims are single-source: well-evidenced (exact
addresses throughout, reproducible by the extractor in `re/tools/`), but they
have not had the second pair of eyes the rest of `PROTOCOL.md` has had. Raw
output: `raw-journal.jsonl`.

Tags follow the project convention: **[BIN]** disassembly, **[?]** inference.

---

## 1. The headline answers

| Question | Answer |
|---|---|
| Is there an embedded payload? | Yes — a **PE resource**, so locatable without a fixed offset **[BIN]** |
| Does the host transform it? | **No.** Plain byte copy, `LockResource` → `HidD_SetFeature` **[BIN]** |
| Is it checksummed? | Yes — a 16-bit additive sum per 1024-byte block, in the request **[BIN]** |
| Is there a whole-image checksum? | **No.** None host-side, and no command carries one **[BIN]** |
| Flashed over radio? | **No — cable only.** The dongle PID is never opened **[BIN]** |
| Is a failed flash recoverable? | **Probably** — the updater has a purpose-built recovery path, but one link is unproven |
| Is the capture one-shot? | **No.** No version gating; it re-flashes on demand **[BIN]** |

Feasibility verdict: a Linux flasher is **buildable without breaking any
crypto**. The device decrypts; the host is a pipe. The blocking gap is not the
payload, it is the bootloader's HID report descriptor, which only a capture can
supply.

## 2. The payload

```
type   "FWFILE"   (string-named resource type, language 2052)
id     134 (0x86)  <- the one actually flashed
id     135 (0x87)  <- present, never referenced by any code path
size   209,920 bytes (0x33400) = 205 × 1024 exactly — both resources
```

`FWFILE/134` file offset `0x001D29E8`, RVA `0x1D91E8`,
sha256 `ad612be22f77907162429e1053a6fad53c91bd59a7f0916c9715970e56aa2b27`.
The file's **overlay is 0 bytes** (last section ends exactly at EOF) and the
SECURITY directory is `(0,0)` — not Authenticode-signed. Nothing is appended;
everything lives in `.rsrc`. **[BIN]**

Block count is `SizeofResource() >> 10` (`SHR ECX,0x0A` @ `0x4041B8`); a nonzero
remainder would be zero-padded and the count incremented. Neither the length nor
the offset is stored anywhere else — a scan for `0x33400` and `0xCD` finds no
relevant hits. **[BIN]**

### The trap: which resource ID

There are *two* FWFILE resources and only 134 is ever sent. The ID is a
hardcoded `PUSH 0x86` at `0x00404195`, feeding the only
`FindResourceW(NULL, 134, L"FWFILE")` in the binary; the `L"FWFILE"` literal at
VA `0x0059ADF0` has exactly one code reference. There is no `PUSH 0x87`
anywhere. **[BIN]**

So "take the only FWFILE resource" fails, "take the highest ID" picks the wrong
one, and "take the largest" ties. **An extractor must recover the ID from the
code and refuse to run if it cannot pin exactly one.**

#### Confirmed against a second updater, and the trap is worse than it looked

`OP1w4kv2/firmware/Endgame Gear OP1w 4k v2 Firmware Updater v1.07.exe`
(2,785,280 bytes) carries **five** FWFILE resources, and the one it flashes is
neither the first nor the last: **[BIN]**

| updater | FWFILE ids present | flashed | code site |
|---|---|---|---|
| OP1w 4k v1.10 | 134, 135 | **134** | rva `0x4190`, thunk `0x567298` |
| OP1w 4k v2 v1.07 | 134, 135, 136, 137, 139 | **136** | rva `0x32B7`, thunk `0x51B230` |

Every naive rule fails on the second sample — lowest picks 134, highest picks
139, largest ties five ways, and a fixed file offset is meaningless. The code
site moved between the two binaries, so the instruction-pattern scan is doing
real work rather than accidentally matching a constant layout.

All seven images across the two updaters are **209,920 bytes and mutually
distinct** (sha256), so size never discriminates and none is a duplicate. They
fall into shape classes by record structure, which is a fingerprint of the
*plaintext* since each image is keyed separately:

```
135 distinct records, most repeated x71 : v1/134, v1/135, v2/134, v2/135
138 distinct records, most repeated x68 : v2/136, v2/137
136 distinct records, most repeated x70 : v2/139
```

The pairing is suggestive — ids arrive two at a time with matching shape, and
the flashed one is the lower of its pair — but nothing here establishes what
the spares are for, and a wrong pick would flash a valid-looking image built
for something else. **[?]**

This retires the caveat this document previously carried, that cross-version
stability was unverified with n=1. The *method* now has n=2 and holds. What is
still n=1 is any claim about a specific id: 134 is not "the firmware id".

`re/tools/extract_fw.py` does exactly that — stdlib-only Python, runs on Linux,
no hardcoded offsets. It matches the call sequence

```
68 <VA of L"FWFILE">   PUSH string
68 <imm32>             PUSH resource id      <- reads this
6a 00                  PUSH 0
ff 15 <IAT slot>       CALL [FindResourceW]
```

and takes the size from the resource directory, never from a constant. It fails
loudly if the scan does not pin exactly one ID — a recompile with different
argument marshalling would return zero sites and it would (correctly) refuse.
Verified output for v1.10 matches every value in this section.

### Encryption

Entropy 7.9788 bits/byte over the whole image, all 256 byte values present,
742 zero bytes (uniform expectation ≈820). Not compressed, not plaintext, no
container magic — first bytes `b2 06 aa d6 5b ca 25 f8`. Structure:

```
records 0..132     unique
records 133..203   71 records, all byte-identical
record  204        unique
```

71 identical incompressible kilobytes rules out compression, and shows the
transform **resets at every 1024-byte boundary with no chaining across
records**. Within a single record no 16-byte sub-block repeats (and none up to
64 bytes), so it is not 16-byte ECB. Those 71 identical ciphertexts go to 71
*different* destination indices, so the state is not derived from the
destination either. **[BIN]**

A simple repeating 1024-byte XOR keystream was tested and **refuted**: deriving
a candidate key from the filler record and applying it leaves the rest at random
entropy. The mode is otherwise **unknown** — consistent with a chained or
counter mode re-initialised per record. Images 134 and 135 share no records at
all, not even the filler, so they are keyed differently. **[?]**

No crypto primitives exist in the binary at all: no AES S-box or T-table, no
CRC32 table, no MD5/SHA IVs, no TEA delta, no CryptoAPI or bcrypt imports —
across the full 2.4 MB, and the same for the config tools. **[BIN]**

**You do not need the key.** The device decrypts.

## 3. The protocol

Same transport as the config channel: `HidD_SetFeature`/`HidD_GetFeature`,
report `0xA0`, **1041 bytes** (`EDX = 0x411` at every call site), payload at
`+16`, status at response `+1`. Device matching is VID `0x3367` + PID +
`HidP_GetCaps` UsagePage `0xFF01` / Usage `0x02` (`FUN_00401C00`). **[BIN]**

> The bootloader's status codes differ from the config protocol's:
> **`0x01` = OK, `0x04` = busy** (`FUN_00401F50` re-reads with increasing delay
> up to ~1 s). The config protocol's busy code is `0x03`.

| Step | Request | Host's check |
|---|---|---|
| a. enter bootloader | `A0 3A 00 00 \| 00 5A A5 32 \| zeros` | none; waits for re-enumeration |
| b. re-enumerate | close, open `3367:1971`, retry 300 ms up to 3000 ms | else "Open bldr device request failed" |
| c. echo test | `A0 01 01 00` + `fw[0:1024]` at `+16` | byte 1 == `0x01` **and** `+16..` echoed byte-for-byte |
| d. start | `A0 03 00 00`, `[16]` = block count (**low byte only**), `[17..20]` = u32 | byte 1 == `0x01` |
| e. write block ×205 | `A0 06`, `[2..3]` = LE16(index + `0x34`), `[4..5]` = sum16, `[16..1039]` = 1024 B | byte 1 == `0x01`, else resend (5 tries) then abort |
| f. complete | `A0 09 00 00` + zeros | byte 1 == `0x01` |
| g. re-enumerate | open `3367:1972`, retry 500 ms up to 5000 ms | else "Update failed, try again" |
| h. post-update | `A1 13 00 00` on the **64-byte** `0xA1` report | reply read, not inspected |

Step (h) is cmd `0x13` — **factory reset** (`PROTOCOL.md` §3). *The updater wipes
the user's configuration after flashing.* Read and save the config blob first.

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
boundary at block 204. Base `0x34` puts the image at flash offset
`0x34 × 1024 = 0xD000`. **[BIN]**

The checksum (`FUN_00402640`) is a plain 16-bit wrapping **additive** sum of the
1024 payload bytes, written little-endian into `[4..5]` and zero-extended to a
dword at `[4..7]`. **[BIN]**

Because each block carries its own destination index, **retries are safe and
idempotent** — a resend cannot land in the wrong place. But a 5th consecutive
failure **aborts mid-image** with no rollback and no restart, leaving the device
in the bootloader with a partial write. **[BIN]**

### Unknowns in the protocol

- **`[17..20]` of the start command** comes from `this+0x3EA60`, which is
  referenced *exactly once* in the entire 1.46 MB `.text` — the read at
  `0x4047E8`. Nothing writes it. It is a declared-but-never-assigned member of a
  stack object whose constructor initialises only the vptr, three CStrings and
  an icon. Predicted `00 00 00 00` on the wire because Windows zero-fills fresh
  stack pages; the capture settles it. **Proven regardless: it is not derived
  from the image, so it is neither a length nor a checksum.** **[BIN]** / **[?]**
- What resource 135 is for.
- The bootloader's own HID report descriptor — not derivable statically.

## 4. Recoverability

**The bootloader is a distinct USB identity: `3367:1971`** — the mouse PID minus
one. There are exactly 9 call sites to the device-open helper `FUN_00401C00`,
each immediately preceded by a literal `MOV EDX,imm32`: `0x1972` (application)
at five sites, `0x1971` (bootloader) at four. The updater knows exactly two
identities. **[BIN]**

> This upgrades `PROTOCOL.md` §8 item 4 from **[?]** to **[BIN]**: cmd `0x0E`
> payload `+4..+5` is the bootloader PID.

**The updater has a purpose-built recovery path.** In the button handler
`FUN_004040F0`, it probes `0x1972` first; failing that it probes `0x1971`, and
if the *bootloader* answers it calls straight into the block loop — skipping
both the `A0 3A` bootloader-entry and the echo test. A vendor writes that branch
only because devices do end up stranded in the bootloader and must be
recoverable by re-running the tool. The abort-and-do-nothing behaviour on block
failure is only a sane design under the same assumption. **[BIN]**

**Cable only.** PID `0x1970` (the dongle) is never passed to the open helper,
and none of the wireless machinery appears — no `0x0F` target probe, no `0x0E`,
no `0xB4`, and the enter-bootloader command carries target byte `0x00`, not
`0x0F` ("mouse via dongle"). The failure mode of losing the 2.4 GHz link
mid-write does not exist on this path. **[BIN]**

**No version gating.** The version is read from `HidD_GetAttributes`'
`VersionNumber` (bcdDevice ÷ 100), formatted into a label, and never compared to
anything. There is no "already up to date" string in the binary. Clicking the
button always flashes. So captures are **repeatable** — you do not get one shot
at the capture, only one shot at each capture being clean. **[BIN]**

### What is still unknown — the load-bearing gap

- **Whether the bootloader survives a half-written application.** The only
  *proven* route into it is `A0 3A`, handled by the *running application*. If a
  partial image will not run, recovery depends on the bootloader staying in DFU
  mode by itself. That is the standard pattern, and the `0x1971` probe strongly
  implies it, but it is not proven.
- **Whether the device validates the decrypted image before jumping to it.**
  Nothing host-side answers this, and it is the difference between "retry" and
  "brick". After `A0 09` the host waits ~5 s for `0x1972` and otherwise prints
  "Update failed, try again" — which is what a bootloader *refusing* a bad image
  looks like, and equally what a bootloader that *jumped* to a corrupt image
  looks like. Indistinguishable from the binary, opposite consequences.
- The encryption gives **no** integrity protection: nothing chains across block
  boundaries, so truncation, dropped blocks and reordering all still decrypt
  normally. It prevents forgery, not damage.
- Record 204 (destination index `0x100` = flash `0x40000`, exactly 256 KB, one
  page past an application region of `0x34`..`0xFF`) is plausibly a
  trailer/metadata slot — where a whole-image CRC would live if one exists. It
  is encrypted, so this is **speculation**. **[?]**

## 5. The worst avoidable mistake

> **The dongle, the mouse application and the bootloader all expose the same
> vendor collection `0xFF01`/`0x02` and the same report IDs.**

`PROTOCOL.md` §1 correctly tells a *configuration* tool to select its hidraw node
by usage page. For flashing that is dangerous: the same logic can attach to
`3367:1970` — the dongle — and stream 205 KB of mouse firmware at it. The dongle
is a different chip from a different bootloader vendor (`0x22D4`, PIDs
`0x1502`/`0x1503`) with its own updater, shipped as the `USBDATA` resource
inside every config tool (90,112 bytes, a complete 32-bit PE, byte-identical
across all five tools, sha256 `db06d7af…`). Bricking the dongle bricks all
wireless use of the mouse.

**`egg::Device`'s node-selection logic must not be reused for flashing without a
hard VID/PID gate.** Likewise, never emit a destination index below `0x34` —
the vendor flow never does, and whether the device range-checks one is unknown.

## 6. What the capture must nail

Repeatable, so get a clean one rather than a hurried one. Priority order:

1. **The bootloader's own descriptors**, including its HID report descriptor,
   for `3367:1971`. Not derivable statically, not obtainable any other way, and
   without it a Linux tool cannot pick the right hidraw node or know the
   feature-report sizes.
2. Byte-compare the first outgoing block against record 0 of `FWFILE_134.bin`
   (first bytes `b2 06 aa d6 5b ca 25 f8`). A match proves extraction, ID choice
   and "stream it verbatim" end to end — the whole feasibility claim.
3. The exact `start` request bytes, to settle `[17..20]`.
4. Per-block responses — **capture the full 1041 bytes**, not just byte 1. The
   host reads only byte 1 and discards the rest, so the capture is the only
   chance to learn whether the device reports a *reason* for a rejection.
5. Re-enumeration back to `3367:1972` and the new `bcdDevice` (expect `0x6E`).
6. Real inter-command timing (coded sleeps: 20·n, 70, 150, 200, 500, 1000,
   1080 ms).
7. Confirm the trailing `A1 13` factory reset.

### Falsifiable predictions

The first outgoing blocks should be exactly:

| block | `[2]` | `[3]` | `[4]` | `[5]` |
|---|---|---|---|---|
| 0 | `34` | `00` | `65` | `fb` |
| 1 | `35` | `00` | `90` | `f8` |
| 2 | `36` | `00` | `a1` | `f9` |
| 3 | `37` | `00` | `86` | `fb` |
| 203 | `ff` | `00` | `05` | `00` |
| 204 | `00` | `01` | `23` | `06` |

and `[16..1039]` == `FWFILE_134.bin[index*1024 : index*1024+1024]`.

Use `re/tools/capture-firmware.ps1` — it deliberately does **not** filter by
device, because the bootloader appears as a new one mid-session.

**Do not deliberately interrupt a flash to test recoverability** until a clean
end-to-end trace exists. And note what the capture will *not* tell you: whether
the device would refuse a corrupt image. Only sending one answers that.

---

## 8. First wire evidence — a failed run, 2026-09-27

An attempt on the cabled v1 (firmware 1.08) stopped at the first step with the
updater reporting `send bldr request failed`. Nothing was written to flash, and
the capture is only 30 packets, but it confirms several predictions that until
now rested entirely on the disassembly. Parse it with
`re/tools/parse_fwcap.py`; capture at
`re/captures/firmware/fw_op1w4k_v108_to_v110.pcap`.

**The enter-bootloader command is exactly as predicted.** **[CAP]**

```
setup    21 09 a0 03 01 00 11 04
         │  │  │     │     └──── wLength 1041
         │  │  │     └────────── wIndex 1 (interface)
         │  │  └──────────────── wValue 0x03A0 = Feature report, id 0xA0
         │  └─────────────────── bRequest 0x09 SET_REPORT
         └────────────────────── bmRequestType 0x21 host->device, class, interface
payload  a0 3a 00 00 00 5a a5 32 00 00 00 ... (zeros to 1041)
```

Byte for byte what §3 derived from `FUN_004044B0`, including the `00 5A A5 32`
magic. The 1041-byte report size and the `0xA1`/`0x09` request pair are
confirmed too.

**The version mechanism is confirmed.** The device descriptor carries
`bcdDevice = 0x0108`, and the updater displays "1.08" — so reading
`HidD_GetAttributes().VersionNumber` and dividing by 100 is right. **[CAP]**

**The device accepts the command and then leaves.** The `SET_REPORT` completes
with `USBD_STATUS_SUCCESS`. The host then issues the `GET_REPORT` to read the
status, and that request never completes — it and every pending endpoint are
cancelled 2.5 s later with `USBD_STATUS_CANCELED`. A device told to reboot into
its bootloader, which then stops answering on the old identity, is behaving
correctly; the tool's own error message is just the least informative way it
could have said so. **[CAP]**

### The trap for anyone repeating this in a VM

The mouse did not come back, and the bootloader never appeared in the capture.
**This is a USB passthrough artefact, not a device fault.** The bootloader is a
*different USB device* — `3367:1971`, §4 — so a hypervisor rule that forwards
`3367:1972` stops forwarding at exactly the moment the update begins. From
inside the guest the device simply vanishes mid-conversation.

To capture a whole update in a VM the passthrough must forward **both**
identities, or better, forward the physical port rather than a VID/PID pair.
Otherwise every attempt will fail at the same place, and it will look like the
device is refusing the command when in fact it obeyed it.

**The bootloader identifies itself by name.** On the host it enumerates as
`EGG Bootloader [3367:1971]`, so the iProduct string is a second, independent
way to recognise it — useful for a Linux flasher, which should gate on VID/PID
anyway but can sanity-check the string. This is also direct confirmation of the
PID that §4 derived from the nine `MOV EDX,imm32` call sites. **[DEV]**

The upside is that the state this leaves behind is the one §4 calls recoverable:
the mouse should be sitting in DFU mode as `3367:1971`, which the updater
explicitly probes for and handles by flashing directly, skipping the
enter-bootloader step. Re-running the updater with that identity passed through
should both recover the mouse and capture the part that matters.

---

## 9. Verified against hardware — a complete update, 2026-09-27

An OP1w 4k v1 was flashed 1.08 → 1.10 with the capture running. **The update
succeeded**, and the trace confirms the protocol end to end. Capture:
`re/captures/firmware/fw_op1w4k_v108_to_v110_from_bootloader.pcap`
(474,565 bytes, 896 packets). This run entered through the recovery branch —
the mouse was already in the bootloader from the previous attempt — so it
covers everything except `A0 3A`, which §8 captured separately.

### The feasibility claim is proven

The 205 block payloads, reassembled in index order, are **byte-identical to the
PE resource** that `extract_fw.py` pulls out of the updater: **[CAP]**

```
205 / 205 records match,  0 mismatches
length          209,920 = resource length
sha256 on wire  ad612be22f77907162429e1053a6fad53c91bd59a7f0916c9715970e56aa2b27
sha256 resource ad612be22f77907162429e1053a6fad53c91bd59a7f0916c9715970e56aa2b27
```

So §2's central claim — extract the resource, stream it verbatim, no key
needed — is no longer an inference. A Linux flasher does not have to break the
encryption, and the extractor picks the right one of the two resources.

### Predictions that held

| prediction (§3, §6) | result |
|---|---|
| 205 blocks, indices `0x0034`..`0x0100`, contiguous | exact |
| block 0 `[2..5]` = `34 00 65 fb`, payload `b2 06 aa d6…` | exact |
| blocks 1–5 checksums `90 f8`, `a1 f9`, `86 fb`, `96 ef`, `85 e7` | exact |
| block 204 = index `0x0100`, sum `0x0623` | exact |
| `[2..3]` is one LE16 crossing the byte boundary at block 204 | confirmed |
| start `[16]` = `0xCD` = 205 | exact |
| `A0 09` complete, then re-enumeration, then `A1 13` on the 64-byte report | exact |
| status byte at response `+1`, `0x01` = OK | exact, all 208 replies |

No block was ever resent and no reply carried the busy code `0x04`, so the
retry and back-off paths went unexercised.

### Predictions that were wrong

**The start command's `[17..20]` is not zero.** §3 predicted `00 00 00 00` on
the reasoning that the field is a never-written stack member and Windows
zero-fills fresh stack pages. On the wire it is `b4 fe ef 00`: **[CAP]**

```
a0 03 00 00 ... [16]=cd  [17..20]= b4 fe ef 00
```

`0x00EFFEB4` is a stack address — uninitialised memory, exactly as the analysis
said, just not zeroed. The load-bearing half of that claim survives and is now
stronger: **the device accepted the update with garbage in that field**, so it
is neither a length nor a checksum, and a reimplementation may put anything
there. Prefer zero.

**The post-update `bcdDevice` is `0x0110`, not `0x6E`.** §6 expected 110
decimal. The device encodes the version as hex digits — 1.08 is `0x0108`, 1.10
is `0x0110` — which is precisely why the updater formats it with `"%x"` before
`_wtol` and the divide by 100. The mechanism §3 described was right; the
predicted value applied it wrongly. **[CAP]**

### The device says more than the vendor tool reads

§6 asked for the full replies, because the host looks only at byte 1. It was
worth asking. Every block acknowledgement carries: **[CAP]**

```
50 01 34 00 00 00 65 fb 00 ...
│  │  └─┬─┘       └─┬─┘
│  │    │           └──── the block's 16-bit checksum, echoed
│  │    └──────────────── the block's destination index, echoed
│  └───────────────────── status, 0x01 = OK
└──────────────────────── 0x50, not an echo of the 0xA0 request id
```

**All 205 replies echoed both the index and the checksum correctly.** A Linux
flasher can therefore verify each block was received where it was aimed and
with the payload the device computed the same sum over — a much stronger check
than the vendor tool's, which discards all of it. Byte 0 is `0x50` on every
bootloader reply; the one config-protocol reply in the trace (`A1 13`) instead
echoes `0xA1`.

### The bootloader's USB identity

```
DEVICE   VID 3367  PID 1971  bcdDevice 0021  iProduct "EGG Bootloader"
CONFIG   1 interface, 100 mA, bus-powered
IFACE    #0  alt 0  1 endpoint  class 03 sub 01 proto 02  (HID, boot, mouse)
HID      bcdHID 0111  wDescriptorLength 68
ENDPOINT 0x81  interrupt IN  64 bytes  1 ms
```

**One interface and one endpoint.** The application exposes five HID
collections and the dongle four, so node selection is fiddly there; on the
bootloader it is unambiguous — there is exactly one hidraw node. **[CAP]**

**The bootloader exposes the vendor collection — deduced, not guessed.** The
updater's single device-open helper `FUN_00401C00` requires
`UsagePage == 0xFF01` and `Usage == 0x02` before returning success, and all nine
call sites go through it, including the four that open `0x1971` (§4). This
update ran through the recovery branch, which opens the bootloader with that
helper and then flashes. It flashed. So the bootloader satisfies the usage
check, and §5's premise holds for it. **[BIN]** + **[CAP]**

That is most of what the report descriptor would have told us, without needing
it.

The report descriptor's *content* is still missing: the bootloader was already
enumerated when recording started, so only its length (68 bytes) was injected.
It is cheap to get and needs no capture — put the mouse in the bootloader on
Linux and read
`/sys/class/hidraw/hidrawN/device/report_descriptor`. Note the interface
declares the boot-mouse subclass and protocol while carrying the vendor feature
reports, so do not infer the usage page from the interface class.

### Timing

Blocks ran from t=21.9 s to t=56.7 s — 205 blocks in ~34.8 s, ~170 ms each
against the coded 70 ms, the difference being USB passthrough overhead in the
VM. Start → first block was 4.2 s. A reimplementation should not assume the
coded sleeps are minimums.
