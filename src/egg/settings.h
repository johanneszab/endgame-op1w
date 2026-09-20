// Settings model: the three writable blocks, and a full decode of the device's
// stored configuration blob.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "protocol.h"

namespace egg {

// --------------------------------------------------------------- cmd 0x14 ---

struct CpiStage {
    uint16_t x    = 800;
    uint16_t y    = 800;
    // 0 = Y follows X, 1 = the axes are independent. Named by the independent
    // UnofficialEGGMouseConfig project, then confirmed on hardware both ways:
    // a vendor-written 410/1480 stage reads back correctly here, and a stage
    // written from here reads back correctly in the vendor tool.
    //
    // No test isolates the byte itself — every observation is equally
    // explained by the device ignoring it and using X and Y directly. That
    // does not matter as long as the flag is always derived from x != y, which
    // is what both front-ends do. Don't set it independently of the values.
    uint8_t  xySplit = 0;
};

struct SensorBlock {
    uint8_t  unknown0      = 0;      // always 0; preserved
    bool     ledOnLiftOff  = true;
    uint8_t  lodIndex      = 3;      // 0 = 0.7 mm, step 0.1 mm
    bool     angleSnapping = false;
    bool     rippleControl = false;
    int8_t   angleTuning   = 0;      // degrees
    uint8_t  cpiLevels     = 4;      // active stages, 1..4
    uint8_t  activeStage   = 0;      // 0-based
    std::array<CpiStage, kCpiStageCount> stages{};

    void encode(uint8_t out[kSensorPayload]) const;
    static SensorBlock decodePacket(const uint8_t in[kSensorPayload]);
};

// --------------------------------------------------------------- cmd 0x15 ---

// A per-button value: either a multiclick-filter count, or an SPDT mode. Only
// the left and right buttons expose the SPDT modes in the vendor UI.
enum class ButtonFilterMode { Count, GxSafe, GxSpeed };

struct PowerBlock {
    bool    motionSync  = false;
    uint8_t pollingMode = static_cast<uint8_t>(PollingMode::Hz1000);
    uint8_t flags       = kSlamclickFilter;   // PowerFlag mask; unknown bits preserved

    uint8_t powerSavingMinutes = 1;
    bool    powerSavingEnabled = true;

    // left, right, middle, back, forward — the same order as the button table
    std::array<uint8_t, kFilterButtonCount> buttonFilter{{8, 8, 8, 8, 8}};

    uint8_t deepSleepMinutes = 3;
    bool    deepSleepEnabled = true;

    bool glassMode = false;   // payload byte 10, past the declared length

    bool slamclick() const  { return flags & kSlamclickFilter; }
    bool multiclick() const { return flags & kMulticlickFilter; }
    bool forceMaxFps() const { return flags & kForceMaxSensorFps; }
    void setFlag(uint8_t bit, bool on);

    void encode(uint8_t out[kPowerPayload]) const;
    static PowerBlock decodePacket(const uint8_t in[kPowerPayload]);
};

ButtonFilterMode filterModeOf(uint8_t raw);
uint8_t          filterRawFor(ButtonFilterMode mode, uint8_t count);

// The polling modes this tool is willing to write, with their labels.
struct PollingOption {
    PollingMode mode;
    int         hz;
    const char* label;
};
const std::vector<PollingOption>& pollingOptions();
const char* pollingLabel(uint8_t raw);

// --------------------------------------------------------------- cmd 0x16 ---

// All type codes are confirmed on this model. CpiPreset came from the
// independent UnofficialEGGMouseConfig project and was then verified against
// hardware: the vendor tool's FIXED CPI dialog set X=600 Y=400 on the back
// button, and this decoder read back exactly that.
enum class ActionType : uint8_t {
    MouseButton = 0x00,
    Wheel       = 0x01,   // code is a signed delta: 0x01 up, 0xFF down
    Keyboard    = 0x02,   // code = HID modifier mask, key = HID usage
    Special     = 0x09,   // 0xF1 = cycle CPI stage
    CpiPreset   = 0x0C,   // payload is a CPI record: xySplit, X u16, Y u16
    Consumer    = 0x20,   // code = HID consumer usage, low byte
    Disabled    = 0xFF,
};

struct ButtonEntry {
    uint8_t type = static_cast<uint8_t>(ActionType::MouseButton);
    uint8_t code = 0;   // payload byte +1
    uint8_t key  = 0;   // payload byte +2

    // Payload bytes +3..+5. Zero for every action except FIXED CPI, whose
    // payload is a complete 5-byte CPI record (xySplit, X u16le, Y u16le).
    // Dropping these would truncate such a binding on the next write.
    std::array<uint8_t, 3> rest{};

    // Byte +6 of the record. For the first five buttons this is the same value
    // cmd 0x15 writes, so the two must be kept in step.
    uint8_t multiclickFilter = 8;

    std::string describe() const;

    // Clears the whole 5-byte payload. Call before assigning a simple action
    // so leftover CPI bytes cannot survive into an unrelated binding.
    void clearPayload() { code = 0; key = 0; rest.fill(0); }

    // FIXED CPI (type 0x0C): the payload is a CPI record, same shape as a
    // stage in the sensor block. Confirmed on hardware — the vendor tool's
    // X/Y dialog set 600/400 and these accessors read it back exactly.
    bool     isFixedCpi() const;
    uint16_t fixedCpiX() const;
    uint16_t fixedCpiY() const;
    void     setFixedCpi(uint16_t x, uint16_t y);
};

// Physical order, fixed by capture:
// left, right, middle, back, forward, special, wheel up, wheel down.
struct ButtonTable {
    std::array<ButtonEntry, kButtonCount> entries{};

    void encode(uint8_t out[kButtonTableBytes]) const;
    static ButtonTable decodePacket(const uint8_t in[kButtonTableBytes]);

    // Left-handed mode is simply entries 0 and 1 swapped.
    bool isLeftHanded() const;
    void setLeftHanded(bool on);
};

const char* buttonName(size_t index);
bool        buttonHasSpdt(size_t index);   // left and right only

// ------------------------------------------------------------------ blob ---

struct DecodedConfig {
    SensorBlock sensor;
    PowerBlock  power;
    ButtonTable buttons;
};

DecodedConfig decodeBlob(const std::array<uint8_t, kBlobSize>& blob);

// Copies the per-button filter values into the button table, so that writing
// the table cannot silently revert them.
void syncFilters(const PowerBlock& power, ButtonTable& buttons);

}  // namespace egg
