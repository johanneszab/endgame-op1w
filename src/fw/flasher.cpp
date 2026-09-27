#include "flasher.h"

// Distributions disagree on whether the header sits at <hidapi.h> or
// <hidapi/hidapi.h>; the configuration library makes the same choice.
#if __has_include(<hidapi.h>)
#  include <hidapi.h>
#else
#  include <hidapi/hidapi.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <thread>

namespace fw {
namespace {

void sleepMs(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void put16(uint8_t* p, uint16_t v)
{
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>(v >> 8);
}

uint16_t get16(const uint8_t* p)
{
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

// ---------------------------------------------------------------- sha256 ---
// Self-contained so the tool has no dependency beyond hidapi. Used only to
// print an image's identity for the operator to compare against the
// extractor's output; nothing branches on it.

struct Sha256 {
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                     0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    uint64_t len = 0;
    uint8_t  buf[64]{};
    size_t   have = 0;

    static uint32_t ror(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

    void block(const uint8_t* p)
    {
        static const uint32_t k[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (uint32_t(p[i * 4]) << 24) | (uint32_t(p[i * 4 + 1]) << 16) |
                   (uint32_t(p[i * 4 + 2]) << 8) | uint32_t(p[i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = ror(w[i-15],7) ^ ror(w[i-15],18) ^ (w[i-15] >> 3);
            uint32_t s1 = ror(w[i-2],17) ^ ror(w[i-2],19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for (int i = 0; i < 64; ++i) {
            uint32_t S1 = ror(e,6) ^ ror(e,11) ^ ror(e,25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t t1 = hh + S1 + ch + k[i] + w[i];
            uint32_t S0 = ror(a,2) ^ ror(a,13) ^ ror(a,22);
            uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t t2 = S0 + mj;
            hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }

    void update(const uint8_t* p, size_t n)
    {
        len += n;
        while (n) {
            size_t take = std::min(n, sizeof buf - have);
            std::memcpy(buf + have, p, take);
            have += take; p += take; n -= take;
            if (have == sizeof buf) { block(buf); have = 0; }
        }
    }

    std::string hex()
    {
        uint64_t bits = len * 8;
        uint8_t pad = 0x80;
        update(&pad, 1);
        uint8_t z = 0;
        while (have != 56) update(&z, 1);
        uint8_t be[8];
        for (int i = 0; i < 8; ++i) be[i] = uint8_t(bits >> (56 - i * 8));
        len -= 8;                     // the length field is not part of the message
        update(be, 8);
        char out[65];
        for (int i = 0; i < 8; ++i) std::snprintf(out + i * 8, 9, "%08x", h[i]);
        return std::string(out, 64);
    }
};

}  // namespace

// ------------------------------------------------------------ inspection ---

ImageCheck inspectImage(const std::vector<uint8_t>& image)
{
    ImageCheck c;
    if (image.empty()) {
        c.why = "image is empty";
        return c;
    }
    if (image.size() % kBlockBytes != 0) {
        char b[128];
        std::snprintf(b, sizeof b,
                      "image is %zu bytes, not a whole number of %zu-byte blocks "
                      "(the vendor's images always are)",
                      image.size(), kBlockBytes);
        c.why = b;
        return c;
    }
    c.blocks = image.size() / kBlockBytes;
    if (c.blocks > kMaxBlocks) {
        char b[160];
        std::snprintf(b, sizeof b,
                      "image needs %zu blocks; the start command carries the count "
                      "in a single byte, so the device cannot be told about more "
                      "than %zu", c.blocks, kMaxBlocks);
        c.why = b;
        return c;
    }

    // An encrypted image is high entropy. A PE header or low entropy means the
    // wrong blob was extracted -- most likely one of the decoy FWFILE resources
    // or an unrelated resource entirely.
    std::array<size_t, 256> freq{};
    for (uint8_t v : image) ++freq[v];
    for (size_t f : freq) {
        if (!f) continue;
        double p = double(f) / double(image.size());
        c.entropy -= p * std::log2(p);
    }
    if (image[0] == 'M' && image[1] == 'Z') {
        c.why = "image starts with \"MZ\": this is a PE file, not a firmware image";
        return c;
    }
    if (c.entropy < 7.5) {
        char b[160];
        std::snprintf(b, sizeof b,
                      "image entropy is %.4f bits/byte; a real image is ~7.98 "
                      "because it is encrypted. This is probably the wrong blob",
                      c.entropy);
        c.why = b;
        return c;
    }

    Sha256 s;
    s.update(image.data(), image.size());
    c.sha256 = s.hex();
    c.ok = true;
    return c;
}

// ---------------------------------------------------------------- device ---

Flasher::~Flasher() { close(); }

const Target* Flasher::detect(bool* inBootloader)
{
    for (const Target& t : kTargets) {
        for (int mode = 0; mode < 2; ++mode) {
            const uint16_t pid = mode ? t.bldrPid : t.appPid;
            hid_device_info* list = hid_enumerate(kVendorId, pid);
            if (list) {
                hid_free_enumeration(list);
                if (inBootloader) *inBootloader = (mode == 1);
                return &t;
            }
        }
    }
    return nullptr;
}

bool Flasher::openPid(uint16_t pid)
{
    close();
    // Gate hard on VID *and* PID. Never by usage page: the dongle carries the
    // same collection, and streaming firmware at it would brick a different
    // chip entirely. FIRMWARE.md section 5.
    if (pid != opt_.target->appPid && pid != opt_.target->bldrPid) {
        setError("refusing to open a product ID that is not this model's mouse "
                 "or its bootloader");
        return false;
    }
    hid_device_info* list = hid_enumerate(kVendorId, pid);
    for (hid_device_info* it = list; it; it = it->next) {
        hid_device* d = hid_open_path(it->path);
        if (d) {
            dev_ = d;
            break;
        }
    }
    if (list) hid_free_enumeration(list);
    if (!dev_) {
        char b[200];
        std::snprintf(b, sizeof b,
                      "cannot open %04X:%04X — is the udev rule installed? "
                      "The bootloader is a separate product ID and needs its own "
                      "line (see udev/70-endgamegear.rules)", kVendorId, pid);
        setError(b);
        return false;
    }
    return true;
}

void Flasher::close()
{
    if (dev_) { hid_close(dev_); dev_ = nullptr; }
}

bool Flasher::waitFor(uint16_t pid, int timeoutMs)
{
    const int step = 100;
    for (int waited = 0; waited <= timeoutMs; waited += step) {
        hid_device_info* list = hid_enumerate(kVendorId, pid);
        if (list) { hid_free_enumeration(list); return true; }
        sleepMs(step);
    }
    return false;
}

// -------------------------------------------------------------- protocol ---

bool Flasher::transact(Cmd cmd, const uint8_t* report, uint8_t* reply)
{
    if (opt_.dryRun) return true;

    for (int attempt = 1; attempt <= kBlockAttempts; ++attempt) {
        const int written = hid_send_feature_report(dev_, report, kBldrReportSize);
        if (written < 0) {
            setError("write failed");
            sleepMs(kBlockRetryMs);
            continue;
        }

        uint8_t buf[kBldrReportSize];
        std::memset(buf, 0, sizeof buf);
        buf[0] = kBldrReportId;
        const int read = hid_get_feature_report(dev_, buf, sizeof buf);
        if (read < 0) {
            // Expected exactly once: the device reboots in response to
            // EnterBootloader and never answers. The caller decides.
            setError("no reply");
            sleepMs(kBlockRetryMs);
            continue;
        }

        if (buf[kReplyStatus] == kStatusBusy) {
            sleepMs(kBusyRetryMs * attempt);
            continue;
        }
        if (buf[kReplyStatus] != kStatusOk) {
            char b[96];
            std::snprintf(b, sizeof b, "device answered status 0x%02X to command 0x%02X",
                          buf[kReplyStatus], static_cast<unsigned>(cmd));
            setError(b);
            sleepMs(kBlockRetryMs);
            continue;
        }
        if (reply) std::memcpy(reply, buf, kBldrReportSize);
        return true;
    }
    return false;
}

bool Flasher::enterBootloader()
{
    uint8_t r[kBldrReportSize] = {};
    r[0] = kBldrReportId;
    r[1] = static_cast<uint8_t>(Cmd::EnterBootloader);
    std::memcpy(r + 4, kEnterMagic, sizeof kEnterMagic);

    say("asking the mouse to reboot into its bootloader");
    if (opt_.dryRun) return true;

    // The device obeys and then stops answering, so the follow-up read failing
    // is the normal case, not an error. Capture section 8: the SET_REPORT
    // completes, the GET_REPORT is cancelled when the device re-enumerates.
    hid_send_feature_report(dev_, r, kBldrReportSize);
    uint8_t buf[kBldrReportSize];
    std::memset(buf, 0, sizeof buf);
    buf[0] = kBldrReportId;
    hid_get_feature_report(dev_, buf, sizeof buf);
    close();
    return true;
}

bool Flasher::sendStart(size_t blocks)
{
    uint8_t r[kBldrReportSize] = {};
    r[0] = kBldrReportId;
    r[1] = static_cast<uint8_t>(Cmd::Start);
    // Only the low byte is sent; inspectImage() has already refused anything
    // that would not fit.
    r[kPayload] = static_cast<uint8_t>(blocks & 0xFF);
    // Payload +1..+4 is an uninitialised stack member in the vendor tool -- the
    // capture shows it sending a stack address, and the device accepted it. We
    // send zero. FIRMWARE.md section 9.
    return transact(Cmd::Start, r, nullptr);
}

bool Flasher::sendBlock(size_t i, const uint8_t* data)
{
    const uint16_t index = static_cast<uint16_t>(kFirstBlockIndex + i);
    const uint16_t sum   = checksum(data, kBlockBytes);

    uint8_t r[kBldrReportSize] = {};
    r[0] = kBldrReportId;
    r[1] = static_cast<uint8_t>(Cmd::WriteBlock);
    put16(r + kReqIndex, index);
    put16(r + kReqChecksum, sum);
    std::memcpy(r + kPayload, data, kBlockBytes);

    uint8_t reply[kBldrReportSize];
    if (!transact(Cmd::WriteBlock, r, reply)) return false;
    if (opt_.dryRun) return true;

    // The vendor tool reads byte 1 and throws the rest away. The device
    // actually echoes where it put the block and what it summed over, so check
    // both -- it is the only per-block confirmation the protocol offers that
    // the data arrived intact and in the right place. FIRMWARE.md section 9.
    const uint16_t gotIndex = get16(reply + kReplyIndex);
    const uint16_t gotSum   = get16(reply + kReplyChecksum);
    if (gotIndex != index || gotSum != sum) {
        char b[200];
        std::snprintf(b, sizeof b,
                      "block %zu: device echoed index 0x%04X sum 0x%04X, expected "
                      "0x%04X / 0x%04X", i, gotIndex, gotSum, index, sum);
        setError(b);
        return false;
    }
    return true;
}

bool Flasher::sendComplete()
{
    uint8_t r[kBldrReportSize] = {};
    r[0] = kBldrReportId;
    r[1] = static_cast<uint8_t>(Cmd::Complete);
    return transact(Cmd::Complete, r, nullptr);
}

bool Flasher::sendFactoryReset()
{
    if (opt_.dryRun) return true;
    uint8_t r[kCfgReportSize] = {};
    r[0] = kCfgReportId;
    r[1] = static_cast<uint8_t>(Cmd::FactoryReset);
    return hid_send_feature_report(dev_, r, kCfgReportSize) >= 0;
}

// ----------------------------------------------------------------- drive ---

bool Flasher::flash(const std::vector<uint8_t>& image)
{
    const ImageCheck chk = inspectImage(image);
    if (!chk.ok) { setError(chk.why); return false; }

    bool inBootloader = false;
    const Target* found = detect(&inBootloader);
    if (!found) { setError("no supported mouse found"); return false; }
    if (found != opt_.target) {
        char b[200];
        // Deliberately does not mention --model: the target is normally
        // resolved from the updater's own product string, and blaming a flag
        // the user did not pass sends them looking in the wrong place.
        std::snprintf(b, sizeof b,
                      "a %s is plugged in, but this firmware is for the %s. "
                      "The image is encrypted and identifies nothing by itself, "
                      "so egg-fw will not flash it at a mouse it was not built "
                      "for", found->name, opt_.target->name);
        setError(b);
        return false;
    }

    if (!inBootloader) {
        if (!openPid(opt_.target->appPid)) return false;
        if (!enterBootloader()) return false;
        say("waiting for the bootloader to appear");
        if (!opt_.dryRun && !waitFor(opt_.target->bldrPid, kBootloaderWaitMs)) {
            setError("the mouse did not come back as its bootloader. If this is "
                     "a virtual machine, the bootloader is a different USB "
                     "device and the passthrough has to forward it too");
            return false;
        }
    } else {
        say("mouse is already in its bootloader; flashing directly");
    }

    if (!opt_.dryRun && !openPid(opt_.target->bldrPid)) return false;

    if (!sendStart(chk.blocks)) {
        setError("start refused: " + error_);
        return false;
    }

    for (size_t i = 0; i < chk.blocks; ++i) {
        if (!sendBlock(i, image.data() + i * kBlockBytes)) {
            char b[256];
            std::snprintf(b, sizeof b,
                          "block %zu of %zu failed (%s). The mouse is still in "
                          "its bootloader and re-running this will pick up from "
                          "the start -- it is not bricked",
                          i, chk.blocks, error_.c_str());
            setError(b);
            return false;
        }
        if (progress) progress(i + 1, chk.blocks);
    }

    if (!sendComplete()) { setError("complete refused: " + error_); return false; }

    say("waiting for the mouse to come back");
    close();
    if (!opt_.dryRun && !waitFor(opt_.target->appPid, kReenumerateMs)) {
        setError("the image was written and accepted, but the mouse has not "
                 "re-appeared. In a VM this is usually the passthrough again, "
                 "not a failed update");
        return false;
    }

    if (opt_.factoryReset) {
        if (!opt_.dryRun) sleepMs(1080);   // what the vendor waits
        if (openPid(opt_.target->appPid)) {
            say(opt_.dryRun
                    ? "would send the factory reset the vendor tool sends"
                    : "sending the factory reset the vendor tool sends");
            sendFactoryReset();
            close();
        }
    }
    return true;
}

void Flasher::setError(const std::string& s) { error_ = s; }

}  // namespace fw
