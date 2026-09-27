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
    // Re-runnable after a failure: a device left in the bootloader is found and
    // flashed directly, which is the vendor tool's own recovery branch [BIN]
    // and is how this project's test mouse was recovered [DEV].
    //
    // That is NOT the same as "cannot be bricked". Whether the bootloader
    // always survives a half-written application is FIRMWARE.md section 4's
    // named unknown, and that document records that its own adversarial pass
    // never ran. Do not let this comment become a guarantee.
    bool flash(const std::vector<uint8_t>& image);

    // Put the mouse into its bootloader and stop there. Exists so the
    // bootloader can be inspected without flashing anything -- notably to read
    // its HID report descriptor, which no capture contains and which the node
    // selection in tryOpen() currently has to work around.
    //
    // Getting BACK out is not free: the only established route is to flash.
    // Whether a power cycle leaves DFU has never been tested.
    bool enterDfu();

private:
    bool openPid(uint16_t pid, int timeoutMs = 0);
    void tryOpen(uint16_t pid);
    bool reportRoutable();
    void close();
    // Reads the device's bcdDevice without opening it, so a flash can report
    // the version before and after instead of asserting success.
    static bool firmwareOf(uint16_t pid, uint16_t* out);
    bool waitFor(uint16_t pid, int timeoutMs);

    // One request/reply round trip on the 0xA0 report. `reply` may be null.
    // One request/reply round trip. `retryOnBadReply` distinguishes a block
    // write, where a resend is safe because the block carries its own
    // destination, from Start and Complete, where it is not established.
    bool transact(Cmd cmd, const uint8_t* report, uint8_t* reply,
                  bool retryOnBadReply, uint16_t expectIndex = 0,
                  uint16_t expectSum = 0, bool checkEcho = false,
                  bool checkPrefix = true);
    bool echoTest(const uint8_t* firstBlock);
    bool enterBootloader();
    bool sendStart(size_t blocks);
    bool sendBlock(size_t index, const uint8_t* data);
    bool sendComplete();
    bool sendFactoryReset();

    void setError(const std::string& s);
    void setErrorHid(const std::string& s);
    void say(const std::string& s) const { if (log) log(s); }

    Options     opt_;
    hid_device* dev_ = nullptr;
    std::string error_;
};

}  // namespace fw
