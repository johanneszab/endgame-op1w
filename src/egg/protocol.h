// Endgame Gear OP1w 4k v2 — wire protocol constants.
//
// See re/PROTOCOL.md for how each of these was established. Anything marked
// "unconfirmed" there is not relied upon here.
#pragma once

#include <cstddef>
#include <cstdint>

namespace egg {

// ---------------------------------------------------------------- device ---

inline constexpr uint16_t kVendorId = 0x3367;

// The 2.4 GHz dongle. Statically extracted from all four wireless vendor
// tools (OP1w 4k, OP1w 4k v2, XM2w 4k, XM2w 4k v2) — every one of them opens
// this same PID, so **the dongle does not identify which mouse is paired to
// it**. See PROTOCOL.md section 10.
inline constexpr uint16_t kProductDongle = 0x1970;

// How a model encodes cmd 0x14 payload +2, the lift-off distance. The two
// generations use genuinely different scales, not offset variants: under v1's
// encoding the bytes 1 and 2 mean 1.0 mm and 2.0 mm, whereas under v2's they
// would mean 0.8 mm and 0.9 mm. No value is safe under both readings.
enum class LodEncoding {
    Millimetres,   // v1: value = millimetres, only 1 and 2 exist
    TenthsFrom07,  // v2: value = round(mm * 10) - 7, i.e. 0.7 .. 2.0 mm
};

// What a model implements. The generations differ in more than a missing
// checkbox or two — see PROTOCOL.md section 12.
struct ModelInfo {
    uint16_t    mousePid;   // as reported by cmd 0x0E payload +2..+3
    uint16_t    cabledPid;  // its own USB PID when plugged in by cable
    const char* name;

    LodEncoding lod;
    uint8_t     powerPayloadLen;   // bytes actually written for cmd 0x15

    bool hasAngleTuning;        // cmd 0x14 payload +5
    bool hasGlassMode;          // cmd 0x15 payload +10, the undeclared byte
    bool hasMotionJitter;       // cmd 0x15 flags bit 4 — v1 only
    bool hasMulticlickAck;      // cmd 0x15 flags bit 5 — v2 only
    bool hasForceMaxFps;        // cmd 0x15 flags bit 6 — v2 only
    bool hasPowerSavePolling;   // polling values 0x40 and 0x80 — v2 only
};

// Keyed by the PID the mouse reports through cmd 0x0E, which is the only
// signal that distinguishes the models: every wireless dongle enumerates as
// 0x1970 regardless of which mouse is paired to it.
//
// OP1w 4k and OP1w 4k v2 are confirmed against hardware. The two XM2w entries
// are extrapolated from their vendor tools, which are the same builds as the
// corresponding OP1w tools with different constants. **[?]**
inline constexpr ModelInfo kModels[] = {
    // mousePid cabledPid name                lod                        len  tune   glass  jitter ack    maxfps psPoll
    {  0x1972,  0x1972,  "OP1w 4k",          LodEncoding::Millimetres,   10,  false, false, true,  false, false, false },
    {  0x1984,  0x1984,  "OP1w 4k v2",       LodEncoding::TenthsFrom07,  11,  true,  true,  false, true,  true,  true  },
    {  0x1968,  0x1968,  "XM2w 4k",          LodEncoding::Millimetres,   10,  false, false, true,  false, false, false },
    {  0x1982,  0x1982,  "XM2w 4k v2",       LodEncoding::TenthsFrom07,  11,  true,  true,  false, true,  true,  true  },
};

// Returned when cmd 0x0E has not identified the mouse — typically because it
// is asleep. Everything model-specific is disabled: with two incompatible LOD
// scales in play, guessing would write a wrong lift-off distance.
//
// Its powerPayloadLen is never actually used: Device::writePowerBlock refuses
// before it reaches the length decision. Do not "simplify" by dropping that
// guard and leaning on this value — sending ten bytes to a v2 does not omit
// sensor glass mode, it turns it off.
inline constexpr ModelInfo kUnknownModel{
    0x0000, 0x0000, "unknown", LodEncoding::TenthsFrom07, 10,
    false, false, false, false, false, false
};

// Null if the PID is not one we know.
const ModelInfo* modelForMousePid(uint16_t pid);

// The configuration channel is the vendor collection with this usage.
inline constexpr uint16_t kUsagePage = 0xFF01;
inline constexpr uint16_t kUsage     = 0x0002;

// On Linux one hidraw node covers a whole USB interface, so the config
// collection shares a node with the keyboard/consumer collections on MI_01.
inline constexpr int kInterfaceNumber = 1;

// ---------------------------------------------------------------- reports ---

inline constexpr uint8_t kReportCmd  = 0xA1;  // 1 + 63 bytes
inline constexpr uint8_t kReportBulk = 0xA0;  // 1 + 1040 bytes

inline constexpr size_t kCmdReportSize  = 64;
inline constexpr size_t kBulkReportSize = 1041;
inline constexpr size_t kBlobSize       = 1024;

// Payload starts here in both requests and responses.
inline constexpr size_t kPayloadOffset = 16;

// Request header byte positions.
inline constexpr size_t kOffReportId = 0;
inline constexpr size_t kOffCommand  = 1;
inline constexpr size_t kOffTarget   = 2;
inline constexpr size_t kOffLength   = 3;
inline constexpr size_t kOffChunk    = 6;

// Response header.
inline constexpr size_t kOffStatus = 1;

inline constexpr uint8_t kStatusOk   = 0x01;
inline constexpr uint8_t kStatusBusy = 0x03;

// --------------------------------------------------------------- commands ---

enum class Cmd : uint8_t {
    DongleInfo   = 0x0D,  // read 14 B: firmware version at +0..+1
    // read 14 B: the mouse's own VID at +0..+1 and PID at +2..+3, then +4..+5
    // (PID-1, unidentified) and firmware at +6..+7. The PID is the only way to
    // tell the models apart over the shared dongle — see identifyModel().
    MouseInfo    = 0x0E,
    Probe        = 0x0F,  // liveness probe / target select, no payload
    ReadConfig   = 0x12,  // read 1024 B config blob, via report 0xA0
    FactoryReset = 0x13,  // no payload. NOT a commit.
    WriteSensor  = 0x14,  // write 28 B sensor & CPI block
    WritePower   = 0x15,  // write 11 B; length field says 10 (see below)
    WriteButtons = 0x16,  // write 2 x 28 B button table
    Pair         = 0x70,  // re-pair the mouse to the dongle; no payload
    PairDefault  = 0x71,  // present in the binary, not reachable from the UI
    PairData     = 0x72,  // present in the binary, not reachable from the UI
    Battery      = 0xB4,  // read: percentage at +0
};

// Header byte 2.
enum class Target : uint8_t {
    None   = 0x00,
    Dongle = 0x01,
    Mouse  = 0x0F,
};

// Payload sizes. Note kPowerPayload vs kPowerDeclaredLength: the vendor tool
// writes eleven payload bytes but puts 0x0A in the length field, and the
// device honours the eleventh (it carries sensor glass mode). Replicate both.
inline constexpr size_t kSensorPayload         = 28;
inline constexpr size_t kPowerPayload          = 11;
inline constexpr uint8_t kPowerDeclaredLength  = 0x0A;
inline constexpr size_t kButtonChunkPayload    = 28;
inline constexpr size_t kButtonTableBytes      = 56;
inline constexpr size_t kButtonCount           = 8;
inline constexpr size_t kFilterButtonCount     = 5;   // left, right, middle, back, forward
inline constexpr size_t kCpiStageCount         = 4;

// ----------------------------------------------------------- polling rate ---
//
// The vendor dropdown combines rate with a wireless power-saving mode, so this
// is an enum rather than a plain divisor: four of the five values happen to
// equal 8000 / rate, but the power-saving variant of 1000 Hz does not, and
// 1000 Hz appears twice. All five are captured.

enum class PollingMode : uint8_t {
    Hz4000          = 0x02,
    Hz2000          = 0x04,
    Hz1000          = 0x08,
    Hz125Office     = 0x40,   // "125Hz (Office Mode)"
    Hz1000PowerSave = 0x80,   // "1000Hz (Power Saving)"
};

// ------------------------------------------------------- cmd 0x15 flags ----

enum PowerFlag : uint8_t {
    kSlamclickFilter   = 1u << 0,
    kMotionJitterFilter = 1u << 4,  // v1 only
    kMulticlickFilter  = 1u << 5,   // the "I understand..." acknowledgement
    kForceMaxSensorFps = 1u << 6,
};

// ------------------------------------------------------- blob offsets ------
//
// Offsets inside the 1024-byte blob returned by Cmd::ReadConfig, all validated
// against captured device state. Together these cover every writable field.

namespace blob {
inline constexpr size_t kAngleTuning = 0x01;  // int8, degrees
inline constexpr size_t kDeepSleep   = 0x03;  // minutes, bit7 = disabled
inline constexpr size_t kPowerSaving = 0x04;  // minutes, bit7 = disabled
inline constexpr size_t kPollingMode = 0x05;
inline constexpr size_t kFilterFlags = 0x06;
inline constexpr size_t kLedOnLiftOff  = 0x08;
inline constexpr size_t kLod           = 0x09;
inline constexpr size_t kAngleSnapping = 0x0A;
inline constexpr size_t kRippleControl = 0x0B;
inline constexpr size_t kMotionSync    = 0x0C;
inline constexpr size_t kActiveStage   = 0x0D;
inline constexpr size_t kCpiLevels     = 0x0E;

inline constexpr size_t kCpiBase   = 0x23;  // flag u8, X u16le, Y u16le
inline constexpr size_t kCpiStride = 5;

// Same 7-byte record as a cmd 0x16 entry. Byte +6 is the button's multiclick
// filter value, which cmd 0x15 also writes.
inline constexpr size_t kButtonBase   = 0x37;
inline constexpr size_t kButtonStride = 7;

// Sensor glass mode, the field cmd 0x15 writes as its under-declared 11th
// payload byte. It sits immediately after the 8 button records
// (0x37 + 8*7 = 0x6F), not at 0x02 as first inferred: toggling it moves
// this byte and nothing else in the blob. Confirmed on firmware 1.07.
inline constexpr size_t kGlassMode = 0x6F;
}  // namespace blob

// Offsets within a button record, in both the blob and cmd 0x16.
inline constexpr size_t kBtnType   = 0;
inline constexpr size_t kBtnCode   = 1;
inline constexpr size_t kBtnKey    = 2;
inline constexpr size_t kBtnRest   = 3;   // payload bytes +3..+5
inline constexpr size_t kBtnFilter = 6;

// ---------------------------------------------- asynchronous notifications ---
//
// The dongle pushes 8-byte interrupt reports on the 0xFF02/0x0001 collection.
// Byte 1 reuses the feature-report command numbering, and the rest of the
// payload is laid out like that command's response - a notification is an
// unsolicited command reply.
//
//   03 B4 <battery> <signal> 0F 00 00 00     battery / signal update
//   03 B1 01 00 00 00 00 00                  radio link up
//   03 B1 F0 0A 00 00 00 00                  radio link down (deep sleep)
//
// Battery arrives on its own, so a port never has to poll for it. See
// PROTOCOL.md section 4a.

inline constexpr uint8_t kNotifyReportId   = 0x03;   // 8 bytes, 0xFF02/0x0001
inline constexpr size_t  kNotifyReportSize = 8;

enum class EventCode : uint8_t {
    LinkState = 0xB1,
    Battery   = 0xB4,
};

// Byte 2 of a LinkState event.
inline constexpr uint8_t kLinkUp   = 0x01;
inline constexpr uint8_t kLinkDown = 0xF0;

// Offsets within a notification.
inline constexpr size_t kEvtCode    = 1;
inline constexpr size_t kEvtBattery = 2;
inline constexpr size_t kEvtSignal  = 3;
inline constexpr size_t kEvtTarget  = 4;
inline constexpr size_t kEvtLink    = 2;
inline constexpr size_t kEvtReason  = 3;

// ---------------------------------------------------------------- timing ---
//
// From the vendor tool. The device does not tolerate interleaved or pipelined
// commands; see PROTOCOL.md section 2.

inline constexpr int kWriteRetries    = 4;
inline constexpr int kReadRetries     = 3;
inline constexpr int kRetryDelayMs    = 50;
inline constexpr int kBusyRetries     = 3;
inline constexpr int kBusyDelayStepMs = 200;
inline constexpr int kDefaultSettleMs = 150;
inline constexpr int kBulkSettleMs    = 360;

// The UI's own limits for the two inactivity timers.
inline constexpr int kTimeoutMinMinutes = 1;
inline constexpr int kTimeoutMaxMinutes = 120;

}  // namespace egg
