#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "fwproto.h"

struct hid_device_;
using hid_device = hid_device_;

namespace fw {

// What a firmware image has to look like before we will send it anywhere.
struct ImageCheck {
    bool        ok = false;
    std::string why;            // populated when !ok
    size_t      blocks = 0;
    std::string sha256;
    double      entropy = 0.0;
};

ImageCheck inspectImage(const std::vector<uint8_t>& image);

struct Options {
    const Target* target      = nullptr;
    bool          dryRun      = true;   // nothing is written unless this is false
    bool          factoryReset = true;  // the vendor always does; see flash()
    bool          verbose     = false;
};

// Drives one firmware update. One instance, one device, one flash.
class Flasher {
public:
    // Progress and narration. Both default to silence so tests stay quiet.
    std::function<void(const std::string&)> log;
    std::function<void(size_t done, size_t total)> progress;

    explicit Flasher(Options opt) : opt_(opt) {}
    ~Flasher();

    Flasher(const Flasher&)            = delete;
    Flasher& operator=(const Flasher&) = delete;

    // Which of kTargets is plugged in, if any, and in which mode. Returns null
    // when nothing recognised is attached. Never looks at the dongle.
    static const Target* detect(bool* inBootloader);

    const std::string& lastError() const { return error_; }

    // The whole sequence: enter the bootloader if the mouse is running its
    // firmware, stream the image, complete, wait for the mouse to come back.
    //
    // Safe to re-run after a failure. A half-written image leaves the device in
    // the bootloader, which this will find and flash directly — that is exactly
    // what the vendor tool's own recovery branch does, and what recovered this
    // project's test mouse. [BIN]
    bool flash(const std::vector<uint8_t>& image);

private:
    bool openPid(uint16_t pid);
    void close();
    bool waitFor(uint16_t pid, int timeoutMs);

    // One request/reply round trip on the 0xA0 report. `reply` may be null.
    bool transact(Cmd cmd, const uint8_t* report, uint8_t* reply);
    bool enterBootloader();
    bool sendStart(size_t blocks);
    bool sendBlock(size_t index, const uint8_t* data);
    bool sendComplete();
    bool sendFactoryReset();

    void setError(const std::string& s);
    void say(const std::string& s) const { if (log) log(s); }

    Options     opt_;
    hid_device* dev_ = nullptr;
    std::string error_;
};

}  // namespace fw
