// Transport layer: finds the configuration collection and performs one
// command/response exchange at a time, with the vendor tool's retry rules.
#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "protocol.h"

struct hid_device_;
using hid_device = hid_device_;

namespace vole {

struct DeviceInfo {
    std::string path;
    uint16_t    vendorId  = 0;
    uint16_t    productId = 0;
    int         interfaceNumber = -1;
    uint16_t    usagePage = 0;
    uint16_t    usage     = 0;
    std::string product;
    bool        wired = false;
};

struct Version {
    uint8_t major = 0;
    uint8_t minor = 0;
    std::string toString() const;
};

// A 64-byte command response. `payload()` points at offset 16.
struct Response {
    std::array<uint8_t, kCmdReportSize> raw{};
    uint8_t status() const { return raw[kOffStatus]; }
    const uint8_t* payload() const { return raw.data() + kPayloadOffset; }
};

// An unsolicited 8-byte report pushed by the dongle.
struct Notification {
    std::array<uint8_t, kNotifyReportSize> raw{};

    uint8_t code() const { return raw[kEvtCode]; }
    bool isBattery() const   { return code() == static_cast<uint8_t>(EventCode::Battery); }
    bool isLinkState() const { return code() == static_cast<uint8_t>(EventCode::LinkState); }

    // The mouse reports its polling rate changing on its own. The v1 vendor
    // tool acts on this; it was never triggered in a capture, so the decode is
    // from that tool's dispatcher rather than from the wire. [BIN]
    bool isPollingChanged() const {
        return code() == static_cast<uint8_t>(EventCode::PollingChanged);
    }
    uint8_t pollingModeByte() const { return raw[kEvtBattery]; }   // byte 2

    // Battery events.
    uint8_t batteryPercent() const { return raw[kEvtBattery]; }
    uint8_t undecoded1() const     { return raw[kEvtUndecoded1]; }

    // Link-state events. Link-down is the deep-sleep timeout firing. The v1
    // tool accepts 0xF1 as a second down code; no capture shows it. [BIN]
    bool linkUp() const { return raw[kEvtLink] == kLinkUp; }
    bool linkDown() const {
        return raw[kEvtLink] == kLinkDown || raw[kEvtLink] == kLinkDownAlt;
    }

    std::string toHex() const;
};

class Device {
public:
    Device() = default;
    ~Device();

    Device(const Device&)            = delete;
    Device& operator=(const Device&) = delete;

    // Every candidate hidraw node for this vendor, best match first.
    static std::vector<DeviceInfo> enumerate();

    // Opens the first candidate that answers a probe. Returns false and sets
    // lastError() if none does.
    bool open();
    bool openPath(const std::string& path);
    void close();
    bool isOpen() const { return dev_ != nullptr; }

    const DeviceInfo& info() const { return info_; }
    std::string lastError() const;

    // Which mouse is on the other end. Every wireless dongle enumerates as the
    // same PID, so this comes from cmd 0x0E, not from USB. Until that succeeds
    // — it fails while the mouse is asleep — this is kUnknownModel and every
    // model-specific field is disabled.
    const ModelInfo& model() const { return model_ ? *model_ : kUnknownModel; }
    bool modelIdentified() const { return model_ != nullptr; }

    // Asks the mouse for its own VID/PID and latches the matching entry from
    // kModels. Called by open(), and worth re-calling after a link-up event if
    // the mouse was asleep at startup.
    bool identifyModel();

    // --- low level -------------------------------------------------------
    //
    // Sends one command and reads the response. `payload` may be null.
    //
    // `declaredLength` is what goes in header byte 3. It is normally equal to
    // payloadLen, but cmd 0x15 deliberately under-declares: the vendor tool
    // writes 11 bytes and declares 10, and the device honours the 11th. Pass 0
    // to mean "same as payloadLen".
    //
    // Thread-safe; calls are serialised, as the device requires.
    bool command(Cmd cmd, Target target,
                 const uint8_t* payload, size_t payloadLen,
                 uint8_t chunkIndex, Response* out,
                 uint8_t declaredLength = 0);

    bool command(Cmd cmd, Target target, Response* out) {
        return command(cmd, target, nullptr, 0, 0, out);
    }

    // --- high level ------------------------------------------------------

    bool probe();

    // Cmd 0xB4 answers with the same three bytes a battery notification
    // carries: percentage, an undecoded byte, target.
    struct BatteryStatus {
        // Only meaningful when `charging` is false. While the cable is in, the
        // device does not report a usable state of charge -- see device.cpp.
        uint8_t percent = 0;
        // Cable in and not yet full. The vendor shows "Charging" and no number
        // in this state, and so should any caller.
        bool    charging = false;
        // Payload +1. Carries something; what, is not established. Do not
        // present it as signal strength. See kEvtUndecoded1 in protocol.h.
        uint8_t undecoded1 = 0;
    };
    std::optional<BatteryStatus> batteryStatus();
    std::optional<int>           batteryPercent();
    std::optional<Version> dongleFirmware();
    std::optional<Version> mouseFirmware();

    // The 1024-byte stored configuration. The only way to read settings back.
    bool readConfigBlob(std::array<uint8_t, kBlobSize>& out);

    // Returns the next pending device notification, if any. The dongle pushes
    // these when the radio link changes state and whenever it has a fresh
    // battery reading, so a caller that listens never has to poll.
    //
    // This reads the kernel's buffer, not the device: it generates no USB
    // traffic and is cheap to call on a timer. Input reports from remapped
    // buttons share the same hidraw node and are discarded here. Call in a
    // loop until it returns nullopt to drain the queue.
    std::optional<Notification> pollEvent();

    // True if the mouse itself answered. The dongle stays reachable over USB
    // while the mouse is asleep, so these are separate questions.
    bool mouseAwake();

    bool writeSensorBlock(const uint8_t* payload);   // kSensorPayload bytes
    bool writePowerBlock(const uint8_t* payload);    // kPowerPayload bytes
    bool writeButtonTable(const uint8_t* payload);   // kButtonTableBytes bytes

    // Re-pairs the mouse to the dongle. The vendor UI labels this "Pair
    // Dongle", but what it actually does is re-establish the radio link, so
    // the mouse briefly disconnects and comes back.
    bool pair();

    // Resets the mouse to factory defaults. Not a commit — writes persist on
    // their own (PROTOCOL.md section 5).
    bool factoryReset();

private:
    bool requireModel(const char* what);
    bool sendReport(const uint8_t* buf, size_t len);
    bool getReport(uint8_t* buf, size_t len);
    void setError(std::string msg);

    hid_device*        dev_ = nullptr;
    DeviceInfo         info_;
    const ModelInfo*   model_ = nullptr;   // null until cmd 0x0E identifies it
    mutable std::mutex mutex_;
    mutable std::mutex errorMutex_;
    std::string        error_;
};

// Lift-off distance conversion. The scale depends on the model — see
// LodEncoding — so these take it explicitly rather than assuming v2.
uint8_t lodMillimetresToIndex(double mm, LodEncoding enc);
double  lodIndexToMillimetres(uint8_t index, LodEncoding enc);

// The lift-off distances a model actually offers, in millimetres and in the
// order its vendor tool lists them.
std::vector<double> lodOptions(LodEncoding enc);

// Which scale is in force *right now*. Lift-off is the one setting whose
// encoding is not a property of the model alone: on a v2, turning sensor glass
// mode on switches it to whole millimetres. Always resolve the encoding
// through this rather than reading ModelInfo::lod directly.
// See PROTOCOL.md section 4, "Glass mode changes the lift-off scale".
LodEncoding effectiveLodEncoding(const ModelInfo& m, bool glassMode);

// Translate a stored lift-off byte across a glass-mode change, the way the
// vendor tool does — the scales do not share a meaning, so the byte cannot
// simply be carried over. `glassNowOn` is the state being switched *into*.
// A byte that does not map is returned unchanged for the caller to preserve.
uint8_t lodConvertForGlassMode(uint8_t index, bool glassNowOn);

}  // namespace vole
