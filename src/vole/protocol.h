// Endgame Gear OP1w 4k / XM2w 4k, v1 and v2 — wire protocol constants.
//
// See re/PROTOCOL.md for how each of these was established. Anything marked
// "unconfirmed" there is not relied upon here.
#pragma once

#include <cstddef>
#include <cstdint>

namespace vole {

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
// Highest lift-off index the v2 generation offers: 1.7 mm. Its vendor tool
// populates the combo with exactly eleven entries, 0.7 mm to 1.7 mm, so the
// formula below is only established over that range and 11-13 are bytes no
// vendor tool writes. Corroborated by the same function's OTHER branch (see
// PROTOCOL.md section 4, "Glass mode changes the lift-off scale"), which
// converts a stored 2 mm back to 10 — this maximum — rather than to the 13
// the formula would extrapolate. [BIN]
inline constexpr int kLodMaxIndexV2 = 10;

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

    // cmd 0x14 payload +7. Present in the payload on both generations, but the
    // v1 firmware ignores a change to it — confirmed on hardware, where
    // selecting another stage and applying leaves the pointer speed alone
    // while editing the active stage's CPI takes effect immediately. The v1
    // vendor tool agrees: its cmd 0x14 serializer FUN_00404d00 does write +7,
    // but the only code that ever sets the field it reads is the config-blob
    // parser, so the tool echoes the device's own value straight back. No
    // control writes it and its Basic Settings tab does not read it either,
    // which is why that tab has no radio buttons beside the CPI rows. [BIN]
    //
    // Whole-block writes leave no way to omit the byte, so when this is false
    // a client must send back exactly what it read.
    bool hasCpiStageSelect;

    // What the vendor tool lets you set a CPI stage to. A clean generational
    // split, established by counting clamp constants: 26000 (0x6590) occurs 50
    // times in each v1 tool and never in a v2, and 30000 (0x7530) occurs 10
    // times in each v2 tool and never in a v1. [BIN]
    //
    // The wire format does not care — CPI is a plain little-endian uint16 in
    // CPI units — so these are the vendor's limits, not the device's, and what
    // the device does outside them is untested. The v2 tool also rounds to
    // multiples of 10 below 10000 and of 50 above; that rounding is a UI
    // convention and is not enforced here.
    uint16_t cpiMin;
    uint16_t cpiMax;
    uint16_t cpiStep;
};

// Keyed by the PID the mouse reports through cmd 0x0E, which is the only
// signal that distinguishes the models: every wireless dongle enumerates as
// 0x1970 regardless of which mouse is paired to it.
//
// OP1w 4k and OP1w 4k v2 are confirmed against hardware. The two XM2w entries
// are extrapolated from their vendor tools, which are the same builds as the
// corresponding OP1w tools with different constants. **[?]**
inline constexpr ModelInfo kModels[] = {
    // mousePid cabledPid name                lod                        len  tune   glass  jitter ack    maxfps psPoll stage  cpiMin cpiMax cpiStep
    {  0x1972,  0x1972,  "OP1w 4k",          LodEncoding::Millimetres,   10,  false, false, true,  false, false, false, false,    50, 26000,   50 },
    {  0x1984,  0x1984,  "OP1w 4k v2",       LodEncoding::TenthsFrom07,  11,  true,  true,  false, true,  true,  true,  true,     10, 30000,   10 },
    {  0x1968,  0x1968,  "XM2w 4k",          LodEncoding::Millimetres,   10,  false, false, true,  false, false, false, false,    50, 26000,   50 },
    {  0x1982,  0x1982,  "XM2w 4k v2",       LodEncoding::TenthsFrom07,  11,  true,  true,  false, true,  true,  true,  true,     10, 30000,   10 },
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
    false, false, false, false, false, false, false,
    // The intersection of both generations, so nothing offered before the
    // mouse names itself can be out of range for whatever it turns out to be.
    50, 26000, 50
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

// ------------------------------------------------------ CPI stage colours ---
//
// The colour the underside LED shows for each stage, and the swatch every
// vendor tool paints beside it. Fixed per STAGE, not derived from the stage's
// CPI value: the vendor tool still shows CPI 2 as green with that stage set to
// 1480. Endgame Gear's own documentation lists the colours against 400 / 800 /
// 1600 / 3200 only because those are the factory defaults for stages 1-4.
//
// All five vendor binaries (both OP1w, both XM2w, all versions) hardcode this
// palette as four consecutive `MOV dword ptr [ESI+n], imm32` in stage order —
// blue, green, yellow, red — into four swatch controls at stride 0x9C. They do
// not read the four-colour table in the config blob at 0x0F-0x22, whose order
// is different and whose dereference is still unexplained (PROTOCOL.md section
// 4). So this is model-independent and safe to hardcode here too.  [BIN] [UI]
// Confirmed against the LED itself on an OP1w 4k v1.  [DEV]

struct StageColour {
    uint8_t     r, g, b;
    const char* name;
};

inline constexpr StageColour kStageColours[kCpiStageCount] = {
    { 0x00, 0x00, 0xFF, "blue"   },
    { 0x00, 0xFF, 0x00, "green"  },
    { 0xFF, 0xFF, 0x00, "yellow" },
    { 0xFF, 0x00, 0x00, "red"    },
};

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
// Feeds cmd 0x14 payload +0, the byte that is 0x00 in every capture. Located
// by composing the v1 tool's blob -> settings -> payload chain
// (re/v1/serializers.c); the same composition reproduces every other offset in
// this list, which is the cross-check. [BIN]
inline constexpr size_t kUnknown0      = 0x07;
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

// Event codes are their own namespace: 0xB1 and 0xB4 mirror command numbers,
// 0x06 does not. The complete set the v1 vendor tool dispatches on is 0x06,
// 0x0E, 0xB1 and 0xB4 (re/v1/notify.c, FUN_00415b70); the v2 tool adds an arm
// for 0x02. A code outside a tool's set reaches a do-nothing path, so the
// vendor software drops it — which is what ours does with anything below. [BIN]
//
// Codes are NOT limited to this list. 0x30 and 0x31 both occur on the wire and
// no vendor tool acts on either; see PROTOCOL.md section 4a.
enum class EventCode : uint8_t {
    PollingChanged = 0x06,
    MouseInfo      = 0x0E,
    LinkState      = 0xB1,
    Battery        = 0xB4,
};

// Byte 2 of a LinkState event.
inline constexpr uint8_t kLinkUp   = 0x01;
inline constexpr uint8_t kLinkDown = 0xF0;
// The v1 tool also treats 0xF1 as link-down. Not seen in any capture. [BIN]
inline constexpr uint8_t kLinkDownAlt = 0xF1;

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

}  // namespace vole
