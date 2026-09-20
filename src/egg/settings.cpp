#include "settings.h"

#include <algorithm>
#include <cstring>

namespace egg {
namespace {

void put16(uint8_t* p, uint16_t v)
{
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>(v >> 8);
}

uint16_t get16(const uint8_t* p)
{
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

// A timeout byte carries the value in the low 7 bits and "disabled" in bit 7.
uint8_t packTimeout(uint8_t minutes, bool enabled)
{
    return static_cast<uint8_t>((minutes & 0x7F) | (enabled ? 0x00 : 0x80));
}

std::string hex2(uint8_t v)
{
    static const char* digits = "0123456789ABCDEF";
    std::string s = "0x";
    s += digits[v >> 4];
    s += digits[v & 0x0F];
    return s;
}

constexpr uint8_t kHidKeyA = 0x04;   // HID usage for the A key
constexpr uint8_t kHidKeyZ = 0x1D;

}  // namespace

// --------------------------------------------------------------- cmd 0x14 ---

void SensorBlock::encode(uint8_t out[kSensorPayload]) const
{
    std::memset(out, 0, kSensorPayload);
    out[0] = unknown0;
    out[1] = ledOnLiftOff ? 1 : 0;
    out[2] = lodIndex;
    out[3] = angleSnapping ? 1 : 0;
    out[4] = rippleControl ? 1 : 0;
    out[5] = static_cast<uint8_t>(angleTuning);
    out[6] = cpiLevels;
    out[7] = activeStage;

    for (size_t i = 0; i < stages.size(); ++i) {
        uint8_t* rec = out + 8 + i * 5;
        rec[0] = stages[i].xySplit;
        put16(rec + 1, stages[i].x);
        put16(rec + 3, stages[i].y);
    }
}

SensorBlock SensorBlock::decodePacket(const uint8_t in[kSensorPayload])
{
    SensorBlock s;
    s.unknown0      = in[0];
    s.ledOnLiftOff  = in[1] != 0;
    s.lodIndex      = in[2];
    s.angleSnapping = in[3] != 0;
    s.rippleControl = in[4] != 0;
    s.angleTuning   = static_cast<int8_t>(in[5]);
    s.cpiLevels     = in[6];
    s.activeStage   = in[7];

    for (size_t i = 0; i < s.stages.size(); ++i) {
        const uint8_t* rec = in + 8 + i * 5;
        s.stages[i].xySplit = rec[0];
        s.stages[i].x    = get16(rec + 1);
        s.stages[i].y    = get16(rec + 3);
    }
    return s;
}

// --------------------------------------------------------------- cmd 0x15 ---

void PowerBlock::setFlag(uint8_t bit, bool on)
{
    flags = on ? static_cast<uint8_t>(flags | bit)
               : static_cast<uint8_t>(flags & ~bit);
}

void PowerBlock::encode(uint8_t out[kPowerPayload]) const
{
    std::memset(out, 0, kPowerPayload);
    out[0] = motionSync ? 1 : 0;
    out[1] = pollingMode;
    out[2] = flags;
    out[3] = packTimeout(powerSavingMinutes, powerSavingEnabled);
    for (size_t i = 0; i < buttonFilter.size(); ++i) {
        out[4 + i] = buttonFilter[i];
    }
    out[9]  = packTimeout(deepSleepMinutes, deepSleepEnabled);
    out[10] = glassMode ? 1 : 0;
}

PowerBlock PowerBlock::decodePacket(const uint8_t in[kPowerPayload])
{
    PowerBlock p;
    p.motionSync  = in[0] != 0;
    p.pollingMode = in[1];
    p.flags       = in[2];

    p.powerSavingEnabled = (in[3] & 0x80) == 0;
    p.powerSavingMinutes = in[3] & 0x7F;

    for (size_t i = 0; i < p.buttonFilter.size(); ++i) {
        p.buttonFilter[i] = in[4 + i];
    }

    p.deepSleepEnabled = (in[9] & 0x80) == 0;
    p.deepSleepMinutes = in[9] & 0x7F;
    p.glassMode        = in[10] != 0;
    return p;
}

ButtonFilterMode filterModeOf(uint8_t raw)
{
    switch (raw) {
    case 0xF0: return ButtonFilterMode::GxSafe;
    case 0xF1: return ButtonFilterMode::GxSpeed;
    default:   return ButtonFilterMode::Count;
    }
}

uint8_t filterRawFor(ButtonFilterMode mode, uint8_t count)
{
    switch (mode) {
    case ButtonFilterMode::GxSafe:  return 0xF0;
    case ButtonFilterMode::GxSpeed: return 0xF1;
    case ButtonFilterMode::Count:   break;
    }
    return static_cast<uint8_t>(std::clamp<int>(count, 1, 15));
}

const std::vector<PollingOption>& pollingOptions()
{
    // Listed high to low, matching the vendor dropdown.
    static const std::vector<PollingOption> options = {
        {PollingMode::Hz4000,          4000, "4000 Hz"},
        {PollingMode::Hz2000,          2000, "2000 Hz"},
        {PollingMode::Hz1000,          1000, "1000 Hz"},
        {PollingMode::Hz1000PowerSave, 1000, "1000 Hz (power saving)"},
        {PollingMode::Hz125Office,      125, "125 Hz (office mode)"},
    };
    return options;
}

const char* pollingLabel(uint8_t raw)
{
    for (const auto& o : pollingOptions()) {
        if (static_cast<uint8_t>(o.mode) == raw) {
            return o.label;
        }
    }
    return "unknown";
}

// --------------------------------------------------------------- cmd 0x16 ---

std::string ButtonEntry::describe() const
{
    switch (static_cast<ActionType>(type)) {
    case ActionType::MouseButton:
        switch (code) {
        case 0x01: return "Left click";
        case 0x02: return "Right click";
        case 0x04: return "Middle click";
        case 0x08: return "Back";
        case 0x10: return "Forward";
        default:   return "Mouse button " + hex2(code);
        }
    case ActionType::Wheel:
        if (code == 0x01) return "Wheel up";
        if (code == 0xFF) return "Wheel down";
        return "Wheel delta " +
               std::to_string(static_cast<int>(static_cast<int8_t>(code)));
    case ActionType::Keyboard: {
        std::string s;
        if (code & 0x01) s += "Ctrl+";
        if (code & 0x02) s += "Shift+";
        if (code & 0x04) s += "Alt+";
        if (code & 0x08) s += "Super+";
        if (code & 0x10) s += "RCtrl+";
        if (code & 0x20) s += "RShift+";
        if (code & 0x40) s += "RAlt+";
        if (code & 0x80) s += "RSuper+";
        if (key >= kHidKeyA && key <= kHidKeyZ) {
            s += static_cast<char>(0x41 + (key - kHidKeyA));
        } else if (key) {
            s += "key " + hex2(key);
        }
        return s.empty() ? std::string("Keyboard (none)") : s;
    }
    case ActionType::Special:
        return code == 0xF1 ? std::string("CPI cycle") : "Special " + hex2(code);
    case ActionType::CpiPreset: {
        // The vendor UI calls this FIXED CPI.
        const int x = fixedCpiX();
        const int y = fixedCpiY();
        std::string s = "Fixed CPI " + std::to_string(x);
        if (x != y) {
            s += "/" + std::to_string(y);
        }
        return s;
    }
    case ActionType::Consumer:
        switch (code) {
        case 0xE9: return "Volume up";
        case 0xEA: return "Volume down";
        case 0xE2: return "Mute";
        case 0xCD: return "Play/Pause";
        case 0xB5: return "Next track";
        case 0xB6: return "Previous track";
        case 0x96: return "Browser";
        case 0x94: return "File manager";
        default:   return "Consumer " + hex2(code);
        }
    case ActionType::Disabled:
        return "Disabled";
    }
    return "Unknown (type " + hex2(type) + ")";
}

bool ButtonEntry::isFixedCpi() const
{
    return type == static_cast<uint8_t>(ActionType::CpiPreset);
}

uint16_t ButtonEntry::fixedCpiX() const
{
    return static_cast<uint16_t>(key | (rest[0] << 8));
}

uint16_t ButtonEntry::fixedCpiY() const
{
    return static_cast<uint16_t>(rest[1] | (rest[2] << 8));
}

void ButtonEntry::setFixedCpi(uint16_t x, uint16_t y)
{
    type = static_cast<uint8_t>(ActionType::CpiPreset);
    // Byte +1 is the same xySplit flag the CPI stage records carry.
    code    = (x == y) ? 0 : 1;
    key     = static_cast<uint8_t>(x & 0xFF);
    rest[0] = static_cast<uint8_t>(x >> 8);
    rest[1] = static_cast<uint8_t>(y & 0xFF);
    rest[2] = static_cast<uint8_t>(y >> 8);
}

void ButtonTable::encode(uint8_t out[kButtonTableBytes]) const
{
    std::memset(out, 0, kButtonTableBytes);
    for (size_t i = 0; i < entries.size(); ++i) {
        uint8_t* rec = out + i * blob::kButtonStride;
        rec[kBtnType]     = entries[i].type;
        rec[kBtnCode]     = entries[i].code;
        rec[kBtnKey]      = entries[i].key;
        rec[kBtnRest + 0] = entries[i].rest[0];
        rec[kBtnRest + 1] = entries[i].rest[1];
        rec[kBtnRest + 2] = entries[i].rest[2];
        rec[kBtnFilter]   = entries[i].multiclickFilter;
    }
}

ButtonTable ButtonTable::decodePacket(const uint8_t in[kButtonTableBytes])
{
    ButtonTable t;
    for (size_t i = 0; i < t.entries.size(); ++i) {
        const uint8_t* rec = in + i * blob::kButtonStride;
        t.entries[i].type             = rec[kBtnType];
        t.entries[i].code             = rec[kBtnCode];
        t.entries[i].key              = rec[kBtnKey];
        t.entries[i].rest[0]          = rec[kBtnRest + 0];
        t.entries[i].rest[1]          = rec[kBtnRest + 1];
        t.entries[i].rest[2]          = rec[kBtnRest + 2];
        t.entries[i].multiclickFilter = rec[kBtnFilter];
    }
    return t;
}

bool ButtonTable::isLeftHanded() const
{
    const auto mouse = static_cast<uint8_t>(ActionType::MouseButton);
    return entries[0].type == mouse && entries[0].code == 0x02 &&
           entries[1].type == mouse && entries[1].code == 0x01;
}

void ButtonTable::setLeftHanded(bool on)
{
    if (on != isLeftHanded()) {
        std::swap(entries[0], entries[1]);
    }
}

const char* buttonName(size_t index)
{
    static const char* names[] = {
        "Left", "Right", "Middle", "Back", "Forward",
        "Special", "Wheel up", "Wheel down",
    };
    return index < kButtonCount ? names[index] : "?";
}

bool buttonHasSpdt(size_t index)
{
    return index < 2;   // the vendor UI offers SPDT on left and right only
}

// ------------------------------------------------------------------ blob ---

DecodedConfig decodeBlob(const std::array<uint8_t, kBlobSize>& b)
{
    DecodedConfig c;

    c.sensor.ledOnLiftOff  = b[blob::kLedOnLiftOff] != 0;
    c.sensor.lodIndex      = b[blob::kLod];
    c.sensor.angleSnapping = b[blob::kAngleSnapping] != 0;
    c.sensor.rippleControl = b[blob::kRippleControl] != 0;
    c.sensor.angleTuning   = static_cast<int8_t>(b[blob::kAngleTuning]);
    c.sensor.cpiLevels     = b[blob::kCpiLevels];
    c.sensor.activeStage   = b[blob::kActiveStage];

    c.power.motionSync  = b[blob::kMotionSync] != 0;
    c.power.pollingMode = b[blob::kPollingMode];
    c.power.flags       = b[blob::kFilterFlags];
    c.power.glassMode   = b[blob::kGlassMode] != 0;

    c.power.powerSavingEnabled = (b[blob::kPowerSaving] & 0x80) == 0;
    c.power.powerSavingMinutes = b[blob::kPowerSaving] & 0x7F;
    c.power.deepSleepEnabled   = (b[blob::kDeepSleep] & 0x80) == 0;
    c.power.deepSleepMinutes   = b[blob::kDeepSleep] & 0x7F;

    for (size_t i = 0; i < c.sensor.stages.size(); ++i) {
        const uint8_t* rec = b.data() + blob::kCpiBase + i * blob::kCpiStride;
        c.sensor.stages[i].xySplit = rec[0];
        c.sensor.stages[i].x    = get16(rec + 1);
        c.sensor.stages[i].y    = get16(rec + 3);
    }

    for (size_t i = 0; i < c.buttons.entries.size(); ++i) {
        const uint8_t* rec = b.data() + blob::kButtonBase + i * blob::kButtonStride;
        c.buttons.entries[i].type             = rec[kBtnType];
        c.buttons.entries[i].code             = rec[kBtnCode];
        c.buttons.entries[i].key              = rec[kBtnKey];
        c.buttons.entries[i].rest[0]          = rec[kBtnRest + 0];
        c.buttons.entries[i].rest[1]          = rec[kBtnRest + 1];
        c.buttons.entries[i].rest[2]          = rec[kBtnRest + 2];
        c.buttons.entries[i].multiclickFilter = rec[kBtnFilter];
    }

    // The per-button filter lives inside the button record; cmd 0x15 writes the
    // same five values, so seed the power block from there.
    for (size_t i = 0; i < c.power.buttonFilter.size(); ++i) {
        c.power.buttonFilter[i] = c.buttons.entries[i].multiclickFilter;
    }

    return c;
}

void syncFilters(const PowerBlock& power, ButtonTable& buttons)
{
    for (size_t i = 0; i < power.buttonFilter.size(); ++i) {
        buttons.entries[i].multiclickFilter = power.buttonFilter[i];
    }
}

}  // namespace egg
