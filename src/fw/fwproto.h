// Endgame Gear firmware bootloader protocol.
//
// Deliberately NOT part of src/egg/. The configuration library selects its
// hidraw node by usage page, which is right for talking to a mouse and wrong
// for flashing one: the dongle exposes the same 0xFF01/0x02 collection and the
// same report IDs, so usage-based selection can attach to 3367:1970 and stream
// 205 KB of mouse firmware at it. Everything here gates on VID *and* PID.
// See re/firmware/FIRMWARE.md section 5.
//
// Every constant below is verified against a real update of an OP1w 4k v1 from
// firmware 1.08 to 1.10, captured in
// re/captures/firmware/fw_op1w4k_v108_to_v110_from_bootloader.pcap. [CAP]

#pragma once

#include <cstddef>
#include <cstdint>

namespace fw {

inline constexpr uint16_t kVendorId = 0x3367;

// A mouse and the bootloader it reboots into are two different USB devices.
struct Target {
    uint16_t    appPid;     // the mouse running its firmware
    uint16_t    bldrPid;    // the same mouse in DFU, "EGG Bootloader"
    const char* name;
    const char* key;        // what the user types for --model
    bool        verified;   // has a full flash actually been done on this one?
};

// Only models whose bootloader identity is known. The XM2w pair is deliberately
// absent: nothing establishes their bootloader PIDs, and guessing "app minus
// one" at a device that is about to be overwritten is not a risk worth taking.
inline constexpr Target kTargets[] = {
    // OP1w 4k v1. Flashed end to end on hardware. [CAP]
    { 0x1972, 0x1971, "OP1w 4k",    "op1w4k",   true  },
    // OP1w 4k v2. The bootloader PID is read from cmd 0x0E payload +4..+5 on a
    // real v2 [DEV], but no update has been captured, so the sequence below is
    // assumed rather than observed for this model.
    { 0x1984, 0x1983, "OP1w 4k v2", "op1w4kv2", false },
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

// The magic that makes the running firmware reboot into DFU: payload +0..+3.
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

// The start command sends the block count as ONE byte, so the device cannot be
// told about more than 255 blocks. [BIN]
inline constexpr size_t kMaxBlocks = 255;

// Retry rules, from the vendor tool. [BIN]
inline constexpr int kBlockAttempts    = 5;
inline constexpr int kBlockRetryMs     = 70;
inline constexpr int kBusyRetryMs      = 100;
inline constexpr int kReenumerateMs    = 5000;  // wait for the app to come back
inline constexpr int kBootloaderWaitMs = 3000;  // wait for DFU to appear

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
