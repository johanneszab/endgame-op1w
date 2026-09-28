// Endgame Gear firmware bootloader protocol.
//
// Deliberately NOT part of src/vole/. The configuration library selects its
// hidraw node by usage page, which is right for talking to a mouse and wrong
// for flashing one: the dongle exposes the same 0xFF01/0x02 collection and the
// same report IDs, so usage-based selection can attach to 3367:1970 and stream
// 205 KB of mouse firmware at it. Everything here gates on VID *and* PID.
// See re/firmware/FIRMWARE.md §6.
//
// Every constant below is verified against a real update of an OP1w 4k v1 from
// firmware 1.08 to 1.10, captured in
// re/captures/firmware/fw_op1w4k_v108_to_v110_from_bootloader.pcap. [CAP]

#pragma once

#include <cstddef>
#include <cstdint>

namespace fw {

inline constexpr uint16_t kVendorId = 0x3367;

// The collection that carries reports 0xA0 and 0xA1. Only one node of the
// application device declares them; see Flasher::tryOpen. [HID]
inline constexpr uint16_t kVendorUsagePage = 0xFF01;
inline constexpr uint16_t kVendorUsage     = 0x0002;
// The interface the application carries it on. The BOOTLOADER has a single
// interface numbered 0 (FIRMWARE.md §4), so this is a ranking hint
// only -- never a requirement, or a mouse in DFU becomes unreachable.
inline constexpr int      kVendorInterface = 1;

// A mouse and the bootloader it reboots into are two different USB devices.
struct Target {
    uint16_t    appPid;     // the mouse running its firmware
    uint16_t    bldrPid;    // the same mouse in DFU, "EGG Bootloader"
    const char* name;
    const char* key;        // what the user types for --model
    bool        verified;   // has a full flash actually been done on this one?
    // sha256 of one image known to belong to this model. NOT an allowlist --
    // a later firmware will hash differently and must still be flashable. It
    // exists to make a positive identification of the WRONG file: an image
    // that hashes to another model's known image is refused outright.
    const char* knownImageSha256;
};

// Only models whose bootloader identity is known. The XM2w pair is deliberately
// absent: nothing establishes their bootloader PIDs, and guessing "app minus
// one" at a device that is about to be overwritten is not a risk worth taking.
inline constexpr Target kTargets[] = {
    // OP1w 4k v1. Flashed end to end on hardware. [CAP]
    { 0x1972, 0x1971, "OP1w 4k",    "op1w4k",   true,
      "ad612be22f77907162429e1053a6fad53c91bd59a7f0916c9715970e56aa2b27" },
    // OP1w 4k v2. Flashed end to end on hardware, and 0x1983 confirmed by
    // watching it enumerate rather than only by reading cmd 0x0E. [DEV]
    //
    // Both models' flashes entered through the RECOVERY path, with the mouse
    // already in DFU. flash()'s application-mode branch -- reboot and flash in
    // one invocation -- has still never run, so `verified` means "the block
    // protocol works on this model", not "every path has been exercised".
    { 0x1984, 0x1983, "OP1w 4k v2", "op1w4kv2", true,
      "92605563e19b2f933951d3766835bbc99fd0a16d6452c2abbab7632f3393ab85" },
};

// ------------------------------------------------------------ transport ---
//
// Feature reports, exactly as the configuration protocol uses, but a different
// report and a different size. The report descriptor of the mouse declares
// report 0xA0 as 65 x 16 bytes = 1040, plus the ID. [HID]

inline constexpr uint8_t kBldrReportId   = 0xA0;
inline constexpr size_t  kBldrReportSize = 1041;

// The configuration protocol's report, used only for the closing factory reset.
inline constexpr uint8_t kCfgReportId    = 0xA1;
inline constexpr size_t  kCfgReportSize  = 64;

// Where a command's payload starts inside the report.
inline constexpr size_t kPayload = 16;

// Sub-commands, report byte +1.
enum class Cmd : uint8_t {
    Echo           = 0x01,  // loopback test, writes nothing
    Start          = 0x03,  // announces the block count
    WriteBlock     = 0x06,
    Complete       = 0x09,
    EnterBootloader = 0x3A, // handled by the APPLICATION, not the bootloader
    FactoryReset   = 0x13,  // on the 0xA1 report, after re-enumeration
};

// The magic that makes the running firmware reboot into DFU. It sits at REPORT
// bytes +4..+7 -- immediately after the sub-command and the two zero bytes --
// and NOT inside the +16 payload. FIRMWARE.md §3 step (a), confirmed
// byte for byte on the wire in section 8: `a0 3a 00 00 00 5a a5 32`.
inline constexpr uint8_t kEnterMagic[4] = { 0x00, 0x5A, 0xA5, 0x32 };

// ------------------------------------------------------------- responses ---
//
// A reply is NOT an echo of the request id. Bootloader replies begin 0x50; the
// one configuration reply in the capture begins 0xA1. Nothing depends on byte
// 0, but it is checked because a device answering something else is a device we
// do not understand. [CAP]

inline constexpr uint8_t kReplyPrefix = 0x50;
inline constexpr size_t  kReplyStatus = 1;
inline constexpr uint8_t kStatusOk    = 0x01;
inline constexpr uint8_t kStatusBusy  = 0x04;   // NOT 0x03 as in the config protocol

// The device echoes back where it put the block and what it summed over. The
// vendor tool reads byte 1 and discards these; we check them. Note the offsets
// differ from the request, where the checksum sits at +4.  [CAP]
inline constexpr size_t kReplyIndex    = 2;   // u16 le
inline constexpr size_t kReplyChecksum = 6;   // u16 le

// ---------------------------------------------------------------- layout ---

inline constexpr size_t kBlockBytes = 1024;

// Request byte offsets for a WriteBlock.
inline constexpr size_t kReqIndex    = 2;   // u16 le, destination
inline constexpr size_t kReqChecksum = 4;   // u16 le, additive sum of the 1024

// The image does not start at flash offset zero: the first block is written to
// index 0x34, which is 52 KiB in, leaving the bootloader itself below. A
// reimplementation must never emit an index below this — the vendor flow never
// does, and whether the device range-checks one is unknown. [BIN]
inline constexpr uint16_t kFirstBlockIndex = 0x0034;

// The one geometry ever observed. All seven images across both vendor updaters
// are exactly 209,920 bytes, and the capture shows indices 0x0034..0x0100 with
// 0x0100 the single record past the 0x34..0xFF application region. [CAP]
//
// Bounding by the start command's one-byte count instead (255) would be the
// wrong check twice over: it would accept a TRUNCATED image, which decrypts
// and checksums perfectly because nothing chains across records, leaving the
// device half new and half old; and it would accept an oversized one, writing
// past 0x0100 into flash nobody has mapped, for which there is no DFU
// fallback. Require the exact shape and say so when refusing.
inline constexpr size_t   kImageBlocks    = 205;
inline constexpr uint16_t kLastBlockIndex = 0x0100;

// So the ceiling survives independently of the image-size check: if someone
// later relaxes the block count for a differently sized firmware, this stops
// the change from silently taking the 0x0100 limit with it.
static_assert(kFirstBlockIndex + kImageBlocks - 1 == kLastBlockIndex,
              "block geometry and the highest observed index disagree");

// Retry rules, from the vendor tool. [BIN]
inline constexpr int kBlockAttempts    = 5;
inline constexpr int kBlockRetryMs     = 70;
inline constexpr int kBusyRetryMs      = 100;
inline constexpr int kBusyAttempts     = 8;   // budget of its own; busy is not a failure
// The vendor paces blocks 70 ms apart. Our loop had no delay at all, which is
// the most likely way to provoke the busy path we are least sure about.
inline constexpr int kBlockPaceMs      = 70;
// Re-reading a reply that has not arrived yet. The capture shows the device
// holding the reply to Start for 3.9 s while it erases, and Linux gives a
// control transfer 5 s, so a slightly slower erase must not read as a refusal.
inline constexpr int kSlowReplyAttempts = 6;
inline constexpr int kSlowReplyMs       = 1000;
// Bare metal does this in 1.17 s to a usable hidraw node, twice, so the
// vendor's own 5 s budget for it is adequate and this is wildly generous.
// Deliberately so: the failure mode of being too tight here is telling a user
// that a flash which actually COMPLETED has failed, which invites them to run
// another erase-and-write cycle. Nothing is gained by trimming it.
//
// The VM capture's 19.7 s for the same interval was ~94% passthrough.
// FIRMWARE.md §3. [DEV]
inline constexpr int kReenumerateMs    = 45000;
// The vendor waits 3 s for DFU to appear, which is an open-retry budget and not
// a device specification -- so waiting longer costs nothing, and the
// alternative is telling a user their successful reboot failed.
//
// Measured on bare metal (v1, xhci, from dmesg), twice: 5.01-5.19 s from the
// mouse disconnecting to the bootloader enumerating, 5.45-5.63 s to a usable
// hidraw node. So the vendor's 3 s genuinely cannot make it -- and note this
// is ~7x longer than the same device takes to boot its application (0.71 s),
// so it is not a figure to infer from the return trip. FIRMWARE.md §3. [DEV]
//
// That figure is new. This comment previously cited an OP1w 4k v2 that "took
// longer than 3 s" -- which was a VM artefact, since the hypervisor stops
// forwarding the device at exactly this point (FIRMWARE.md §6) and the guest
// never saw it at all. Same conclusion, but the first time on evidence.
inline constexpr int kBootloaderWaitMs = 30000;

// 16-bit wrapping additive sum, the only integrity check in the protocol.
inline uint16_t checksum(const uint8_t* p, size_t n)
{
    uint16_t sum = 0;
    for (size_t i = 0; i < n; ++i) {
        sum = static_cast<uint16_t>(sum + p[i]);
    }
    return sum;
}

}  // namespace fw
