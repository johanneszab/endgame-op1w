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

namespace egg {

struct DeviceInfo {
    std::string path;
    uint16_t    vendorId  = 0;
    uint16_t    productId = 0;
    int         interfaceNumber = -1;
    uint16_t    usagePage = 0;
    uint16_t    usage     = 0;
    std::string product;
    bool        wired = false;
    const ModelInfo* model = nullptr;   // never null once enumerate() filled it
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

    // Battery events.
    uint8_t batteryPercent() const { return raw[kEvtBattery]; }
    uint8_t signalLevel() const    { return raw[kEvtSignal]; }

    // Link-state events. Link-down is the deep-sleep timeout firing.
    bool linkUp() const { return raw[kEvtLink] == kLinkUp; }

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
    const ModelInfo&  model() const { return modelFor(info_.productId); }
    std::string lastError() const;

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
    // carries: percentage, signal level, target.
    struct BatteryStatus {
        uint8_t percent = 0;
        uint8_t signal  = 0;
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
    bool sendReport(const uint8_t* buf, size_t len);
    bool getReport(uint8_t* buf, size_t len);
    void setError(std::string msg);

    hid_device*        dev_ = nullptr;
    DeviceInfo         info_;
    mutable std::mutex mutex_;
    mutable std::mutex errorMutex_;
    std::string        error_;
};

// LOD is an index from 0.7 mm in 0.1 mm steps.
uint8_t lodMillimetresToIndex(double mm);
double  lodIndexToMillimetres(uint8_t index);

}  // namespace egg
