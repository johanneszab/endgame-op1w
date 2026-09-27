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
    if (c.blocks != kImageBlocks) {
        char b[240];
        std::snprintf(b, sizeof b,
                      "image is %zu blocks; every firmware this project has seen "
                      "is exactly %zu (%zu bytes). A short image would be written "
                      "and completed, leaving the device half new and half old — "
                      "nothing downstream can detect that, because the encryption "
                      "resets every block",
                      c.blocks, kImageBlocks, kImageBlocks * kBlockBytes);
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

bool Flasher::openPid(uint16_t pid, int timeoutMs)
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
    // Reports 0xA0 and 0xA1 do not exist on every node. The application
    // exposes two USB interfaces and several collections; only the one with
    // usage page 0xFF01 / usage 0x02 declares them, and opening the first node
    // that happens to succeed picks the plain mouse collection instead. A
    // feature report sent there is refused, the mouse never reboots, and the
    // tool blames the USB passthrough. Verified on hardware: of six nodes for
    // 3367:1972, GET_REPORT(0xA0) answers on exactly one. [DEV]
    //
    // This is the ranking egg::Device already uses. It does NOT reintroduce
    // the hazard of FIRMWARE.md section 5, because the dongle is excluded by
    // product ID above, before usage is looked at at all.
    // hid_enumerate can see a freshly re-enumerated node before udev has
    // applied the uaccess ACL, so a single attempt races the permission
    // change. Retry briefly rather than failing a flash that is fine.
    for (int waited = 0; !dev_; waited += 100) {
        tryOpen(pid);
        if (dev_ || waited >= timeoutMs) break;
        sleepMs(100);
    }
    if (!dev_) {
        char b[240];
        std::snprintf(b, sizeof b,
                      "cannot open the configuration interface of %04X:%04X. "
                      "Either the udev rule is missing — the bootloader is a "
                      "separate product ID and needs its own line, see "
                      "udev/70-endgamegear.rules — or no node on this device "
                      "carries the vendor collection", kVendorId, pid);
        setError(b);
        return false;
    }
    return true;
}

// A read-only routability check: ask for report 0xA0 and see whether this node
// can carry it at all. Costs nothing and changes nothing.
bool Flasher::reportRoutable()
{
    if (!dev_) return false;
    uint8_t buf[kBldrReportSize];
    std::memset(buf, 0, sizeof buf);
    buf[0] = kBldrReportId;
    return hid_get_feature_report(dev_, buf, sizeof buf) == int(kBldrReportSize);
}

void Flasher::tryOpen(uint16_t pid)
{
    // RANK the nodes, then confirm by asking the device — do not filter on the
    // descriptor alone. Two reasons:
    //
    //  - Reports 0xA0/0xA1 live on exactly one node of the application device.
    //    Opening the first that accepts a handle picks the plain mouse
    //    collection and every feature report is refused.
    //  - The bootloader has a SINGLE interface, numbered 0 (FIRMWARE.md §9),
    //    and its report descriptor has never been read. An earlier version of
    //    this function fell back to `interface_number == 1`, which the
    //    bootloader can never satisfy — so a mouse that had just been put into
    //    DFU could not be opened, by the tool that put it there.
    //
    // The hard VID/PID gate in openPid() has already excluded the dongle, so
    // considering every node of this PID is not FIRMWARE.md §5's hazard.
    hid_device_info* list = hid_enumerate(kVendorId, pid);

    const char* fallbackPath = nullptr;
    for (int rank = 0; rank < 3 && !dev_; ++rank) {
        for (hid_device_info* it = list; it; it = it->next) {
            const int thisRank =
                (it->usage_page == kVendorUsagePage && it->usage == kVendorUsage) ? 0
              : (it->interface_number == kVendorInterface)                        ? 1
                                                                                  : 2;
            if (thisRank != rank) continue;
            hid_device* d = hid_open_path(it->path);
            if (!d) continue;
            if (!fallbackPath) fallbackPath = it->path;
            dev_ = d;
            // The acceptance test is functional: can this node actually carry
            // report 0xA0? That settles it without trusting a descriptor.
            if (reportRoutable()) {
                if (list) hid_free_enumeration(list);
                return;
            }
            close();
        }
    }

    // Nothing answered the probe. Rather than give up — which would strand a
    // mouse in DFU — fall back to the best-ranked node that at least opened.
    // A fresh bootloader with no held response may legitimately not answer an
    // unsolicited read (CLAUDE.md invariant 6), and this is never worse than
    // choosing on the descriptor alone.
    if (fallbackPath) dev_ = hid_open_path(fallbackPath);
    if (list) hid_free_enumeration(list);
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

bool Flasher::transact(Cmd cmd, const uint8_t* report, uint8_t* reply,
                       bool retryOnBadReply, uint16_t expectIndex,
                       uint16_t expectSum, bool checkEcho, bool checkPrefix)
{
    // No dry-run gate here on purpose. The echo test writes nothing and is the
    // only proof the transport carries a 1024-byte payload, so a rehearsal has
    // to actually perform it. Everything that changes device state is gated in
    // its own sender instead.
    const int attempts = retryOnBadReply ? kBlockAttempts : 1;
    for (int attempt = 1; attempt <= attempts; ++attempt) {
        const int written = hid_send_feature_report(dev_, report, kBldrReportSize);
        if (written < 0 || size_t(written) != kBldrReportSize) {
            setErrorHid(written < 0 ? "write failed" : "short write");
            if (!retryOnBadReply) return false;
            sleepMs(kBlockRetryMs);
            continue;
        }

        // Busy has its own budget and, like the vendor, is answered by RE-READING
        // rather than re-sending: a resend of a state-changing command is not
        // established as safe. FIRMWARE.md section 3.
        uint8_t buf[kBldrReportSize];
        int read = -1;
        bool busy = false;
        for (int b = 1; b <= kBusyAttempts; ++b) {
            std::memset(buf, 0, sizeof buf);
            buf[0] = kBldrReportId;
            read = hid_get_feature_report(dev_, buf, sizeof buf);
            busy = (read > int(kReplyStatus) && buf[kReplyStatus] == kStatusBusy);
            if (!busy) break;
            sleepMs(kBusyRetryMs * b);
        }
        if (busy) {
            setError("device stayed busy (status 0x04) through every re-read");
            // Spend the caller's attempt budget rather than short-circuiting
            // it: for a block, a resend is safe and idempotent, and a device
            // that is slow to erase a page is exactly what this is for.
            if (!retryOnBadReply) return false;
            sleepMs(kBlockRetryMs);
            continue;
        }
        if (read < 0) {
            // A lost or late reply is re-READ, never re-sent: a second
            // GET_REPORT is harmless, a second state-changing command is not.
            // Start matters most here — the capture shows the device holding
            // that reply 3.9 s while it erases, against a 5 s kernel control
            // timeout, so a slightly slower erase must not look like a refusal.
            for (int again = 1; again <= kSlowReplyAttempts && read < 0; ++again) {
                sleepMs(kSlowReplyMs);
                std::memset(buf, 0, sizeof buf);
                buf[0] = kBldrReportId;
                read = hid_get_feature_report(dev_, buf, sizeof buf);
            }
        }
        if (read < 0) {
            setErrorHid("no reply");
            if (!retryOnBadReply) return false;
            sleepMs(kBlockRetryMs);
            continue;
        }

        // Never index into a reply shorter than the fields being read. A short
        // control read would otherwise present the memset zeros as a real
        // answer -- and an all-zero echo looks exactly like a mismatch.
        if (size_t(read) < kBldrReportSize) {
            char b[96];
            std::snprintf(b, sizeof b, "short reply: %d bytes, expected %zu",
                          read, kBldrReportSize);
            setError(b);
            if (!retryOnBadReply) return false;
            sleepMs(kBlockRetryMs);
            continue;
        }
        // The bootloader answers 0x50, not an echo of the request id. Anything
        // else means we are not talking to what we think we are -- or are
        // reading a held response rather than a fresh one (CLAUDE.md
        // invariant 6). Either way, do not act on it.
        if (checkPrefix && buf[0] != kReplyPrefix) {
            char b[96];
            std::snprintf(b, sizeof b,
                          "reply began 0x%02X, expected 0x%02X", buf[0], kReplyPrefix);
            setError(b);
            if (!retryOnBadReply) return false;
            sleepMs(kBlockRetryMs);
            continue;
        }
        if (buf[kReplyStatus] != kStatusOk) {
            char b[96];
            std::snprintf(b, sizeof b, "device answered status 0x%02X to command 0x%02X",
                          buf[kReplyStatus], static_cast<unsigned>(cmd));
            setError(b);
            if (!retryOnBadReply) return false;
            sleepMs(kBlockRetryMs);
            continue;
        }

        // The index and checksum the device echoes are the only per-block
        // confirmation the protocol offers. Checked HERE, inside the retry
        // loop, so a transient costs a resend -- which is safe precisely
        // because the block carries its own destination -- instead of aborting
        // the flash. Only a device that repeatedly echoes the wrong thing is a
        // real failure.
        if (checkEcho) {
            const uint16_t gotIndex = get16(buf + kReplyIndex);
            const uint16_t gotSum   = get16(buf + kReplyChecksum);
            if (gotIndex != expectIndex || gotSum != expectSum) {
                char b[200];
                std::snprintf(b, sizeof b,
                              "device echoed index 0x%04X sum 0x%04X, expected "
                              "0x%04X / 0x%04X", gotIndex, gotSum, expectIndex, expectSum);
                setError(b);
                sleepMs(kBlockRetryMs);
                continue;
            }
        }

        if (reply) std::memcpy(reply, buf, kBldrReportSize);
        return true;
    }
    return false;
}

// The vendor's loopback check: send the first block through the echo command,
// which writes nothing, and require it back byte for byte. It is the only
// read-only proof that report 0xA0 actually reaches this device before Start
// erases the application region. FIRMWARE.md section 3 step (c).
bool Flasher::echoTest(const uint8_t* firstBlock)
{
    uint8_t r[kBldrReportSize] = {};
    r[0] = kBldrReportId;
    r[1] = static_cast<uint8_t>(Cmd::Echo);
    r[2] = 0x01;
    std::memcpy(r + kPayload, firstBlock, kBlockBytes);

    // No echo reply has ever been captured -- the one real update came in
    // through the recovery branch, which skips this step -- so byte 0 of an
    // echo reply is pure inference. Checking it would be STRICTER than the
    // vendor, which looks at the status and the payload only (FIRMWARE.md
    // section 3 step (c)), and a wrong guess here would fail after the mouse
    // is already in DFU. Verify what the vendor verifies, nothing more.
    uint8_t reply[kBldrReportSize];
    if (!transact(Cmd::Echo, r, reply, true, 0, 0, false, /*checkPrefix=*/false)) {
        setError("the echo test did not come back: " + error_);
        return false;
    }
    if (std::memcmp(reply + kPayload, firstBlock, kBlockBytes) != 0) {
        setError("the echo test came back altered — the transport to this "
                 "device is not carrying 1024-byte payloads intact, and "
                 "nothing has been written");
        return false;
    }
    return true;
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
    // The WRITE is checked; only the READ is allowed to fail. The capture
    // shows the SET_REPORT completing with SUCCESS and the following
    // GET_REPORT being cancelled when the device re-enumerates, so a failed
    // read is normal and a failed write is not. Conflating them made a refused
    // command indistinguishable from a rebooting mouse, and the user was told
    // to go and look at their hypervisor. [CAP]
    const int written = hid_send_feature_report(dev_, r, kBldrReportSize);
    if (written < 0) {
        setErrorHid("the mouse refused the enter-bootloader command; nothing "
                    "has been written and it is still running its firmware");
        return false;
    }
    uint8_t buf[kBldrReportSize];
    std::memset(buf, 0, sizeof buf);
    buf[0] = kBldrReportId;
    hid_get_feature_report(dev_, buf, sizeof buf);   // expected to fail
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
    if (opt_.dryRun) return true;
    r[kPayload] = static_cast<uint8_t>(blocks & 0xFF);
    // Payload +1..+4 is an uninitialised stack member in the vendor tool -- the
    // capture shows it sending a stack address, and the device accepted it. We
    // send zero. FIRMWARE.md section 9.
    //
    // No resend: Start erases the application region (the capture shows the
    // device holding the reply for 3.9 s doing exactly that), and re-issuing a
    // state-changing command to a device mid-erase is not established as safe.
    // A block write is different -- it carries its own destination.
    return transact(Cmd::Start, r, nullptr, /*retryOnBadReply=*/false);
}

bool Flasher::sendBlock(size_t i, const uint8_t* data)
{
    const uint16_t index = static_cast<uint16_t>(kFirstBlockIndex + i);
    const uint16_t sum   = checksum(data, kBlockBytes);

    // Belt and braces against a future change to the image-size rule. Never
    // emit an index outside the range the vendor flow uses: below it lies the
    // bootloader, above it flash nobody has mapped, and neither has a fallback.
    if (index < kFirstBlockIndex || index > kLastBlockIndex) {
        char b[128];
        std::snprintf(b, sizeof b,
                      "refusing to write block %zu at index 0x%04X, outside the "
                      "range 0x%04X..0x%04X", i, index, kFirstBlockIndex, kLastBlockIndex);
        setError(b);
        return false;
    }
    if (opt_.dryRun) return true;

    uint8_t r[kBldrReportSize] = {};
    r[0] = kBldrReportId;
    r[1] = static_cast<uint8_t>(Cmd::WriteBlock);
    put16(r + kReqIndex, index);
    put16(r + kReqChecksum, sum);
    std::memcpy(r + kPayload, data, kBlockBytes);

    // Resend IS safe here, and the echo is verified inside the retry loop: the
    // block carries its own destination index, so a repeat lands in the same
    // place and is idempotent. FIRMWARE.md section 3.
    return transact(Cmd::WriteBlock, r, nullptr, /*retryOnBadReply=*/true,
                    index, sum, /*checkEcho=*/true);
}

bool Flasher::sendComplete()
{
    uint8_t r[kBldrReportSize] = {};
    r[0] = kBldrReportId;
    r[1] = static_cast<uint8_t>(Cmd::Complete);
    if (opt_.dryRun) return true;
    return transact(Cmd::Complete, r, nullptr, /*retryOnBadReply=*/false);
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
        char b[240];
        std::snprintf(b, sizeof b,
                      "a %s is plugged in, but this firmware is for the %s. "
                      "The image is encrypted and identifies nothing by itself, "
                      "so egg-fw will not flash it at a mouse it was not built "
                      "for", found->name, opt_.target->name);
        setError(b);
        return false;
    }

    uint16_t before = 0;
    const bool haveBefore = !inBootloader && firmwareOf(opt_.target->appPid, &before);

    if (!inBootloader) {
        if (!openPid(opt_.target->appPid)) return false;

        // Prove report 0xA0 actually reaches this node before relying on it.
        // Sending the reboot command to a node whose descriptor does not
        // declare 0xA0 is exactly the failure this tool shipped with, and it
        // presented as a USB passthrough problem.
        if (!reportRoutable()) {
            setError("report 0xA0 does not reach the mouse on the interface "
                     "egg-fw selected, so the reboot command would go nowhere. "
                     "Nothing has been written");
            return false;
        }
        if (opt_.dryRun) {
            say("rehearsal: report 0xA0 reaches the mouse, so the transport is "
                "sound. Everything past this point needs the bootloader, which "
                "only a real run can enter — re-run with --yes.");
            return true;
        }
        if (!enterBootloader()) return false;
        say("waiting for the bootloader to appear");
        if (!opt_.dryRun && !waitFor(opt_.target->bldrPid, kBootloaderWaitMs)) {
            setError("the mouse accepted the reboot command but did not come "
                     "back as its bootloader. In a virtual machine that is "
                     "usually the passthrough: the bootloader is a different "
                     "USB device and has to be forwarded too");
            return false;
        }
    } else {
        say("mouse is already in its bootloader; flashing directly");
    }

    if (!openPid(opt_.target->bldrPid, kBootloaderWaitMs)) return false;
    if (!reportRoutable()) {
        setError("report 0xA0 does not reach the bootloader on the interface "
                 "egg-fw selected. Nothing further has been written; the mouse "
                 "is in its bootloader and the vendor's updater can still "
                 "recover it");
        return false;
    }

    // Read-only, and the last chance to find out that this transport cannot
    // carry report 0xA0 before Start erases anything. A rehearsal runs exactly
    // this far and no further, so it proves something instead of proving that
    // a loop counts to 205.
    say("checking the transport with the vendor's echo test (writes nothing)");
    if (!echoTest(image.data())) return false;

    if (opt_.dryRun) {
        say("rehearsal stops here: everything past this point erases or writes");
        return true;
    }

    say("erasing and starting — the device holds this reply while it erases");
    if (!sendStart(chk.blocks)) {
        char b[420];
        std::snprintf(b, sizeof b,
                      "the start command was not acknowledged: %s.\n"
                      "  Treat the application region as ALREADY ERASED — that "
                      "is what the device does while holding this reply. The "
                      "mouse should be in its bootloader as %04X:%04X; check "
                      "with `egg-fw info` and run this again, which writes from "
                      "the start. Do not unplug it on the assumption that "
                      "nothing happened.",
                      error_.c_str(), kVendorId, opt_.target->bldrPid);
        setError(b);
        return false;
    }

    for (size_t i = 0; i < chk.blocks; ++i) {
        if (!sendBlock(i, image.data() + i * kBlockBytes)) {
            char b[400];
            std::snprintf(b, sizeof b,
                          "block %zu of %zu failed: %s.\n"
                          "  The mouse should now be sitting in its bootloader as "
                          "%04X:%04X — run `egg-fw info` to confirm. If it is, "
                          "running this again flashes from the start, which is "
                          "the vendor tool's own recovery path and is how this "
                          "project's test mouse was recovered. Whether the "
                          "bootloader always survives a half-written image is "
                          "not proven (FIRMWARE.md section 4), so check before "
                          "assuming.",
                          i, chk.blocks, error_.c_str(),
                          kVendorId, opt_.target->bldrPid);
            setError(b);
            return false;
        }
        if (progress) progress(i + 1, chk.blocks);
        sleepMs(kBlockPaceMs);
    }

    // A lost reply here is not a refusal. The device is either rebooting or it
    // is not; going on to look is strictly better than aborting a flash that
    // has already finished writing every block.
    if (!sendComplete()) {
        say("the complete command was not acknowledged (" + error_ +
            "); the device may already be rebooting — checking");
    }

    say("waiting for the mouse to come back");
    close();
    if (!waitFor(opt_.target->appPid, kReenumerateMs)) {
        char b[400];
        std::snprintf(b, sizeof b,
                      "every block was written and acknowledged, but the mouse "
                      "has not re-appeared as %04X:%04X within %d seconds.\n"
                      "  This is the one state that cannot be told apart from "
                      "the outside (FIRMWARE.md section 4). Check `egg-fw info`: "
                      "if it reports the bootloader, re-run this; if it reports "
                      "nothing, re-seat the cable first. Do NOT assume the "
                      "update failed — it may simply be slow to enumerate.",
                      kVendorId, opt_.target->appPid, kReenumerateMs / 1000);
        setError(b);
        return false;
    }

    // Success has to be observed, not assumed. A device that re-enumerates on
    // the OLD firmware -- bootloader refused the image, or it landed somewhere
    // harmless -- satisfies the wait just as well. Reading the version back is
    // the positive control.
    uint16_t after = 0;
    if (firmwareOf(opt_.target->appPid, &after)) {
        char b[200];
        if (haveBefore && after == before) {
            // NOT a failure. There is no version gating in this protocol --
            // FIRMWARE.md section 4: "no 'already up to date' string in the
            // binary. Clicking the button always flashes" -- so re-flashing
            // the version already installed is legitimate, and is the natural
            // repair action. Reporting it as failure would invite another
            // erase-and-write cycle.
            std::snprintf(b, sizeof b,
                          "the mouse reports firmware %x.%02x, the same version "
                          "as before — expected if you re-flashed the version it "
                          "already had", after >> 8, after & 0xFF);
        } else {
            std::snprintf(b, sizeof b, "the mouse reports firmware %x.%02x",
                          after >> 8, after & 0xFF);
        }
        say(b);
    }

    if (opt_.factoryReset) {
        sleepMs(1080);   // what the vendor waits
        if (openPid(opt_.target->appPid, kBootloaderWaitMs) && sendFactoryReset()) {
            say("sent the factory reset the vendor tool sends");
        } else {
            // Not a failure of the flash: the firmware is written. Say so
            // plainly rather than leaving a stale error behind a success.
            say("NOTE: the firmware is written, but the closing factory reset "
                "did not go through. The old configuration survives and the new "
                "firmware may read it differently — `egg-cli` can reset it.");
        }
        close();
        error_.clear();
    }
    return true;
}

void Flasher::setError(const std::string& s) { error_ = s; }

// Every I/O failure used to collapse to "write failed" or "no reply", which
// cannot distinguish a yanked cable from a device refusing the data -- the
// difference between "plug it back in" and "stop".
void Flasher::setErrorHid(const std::string& s)
{
    error_ = s;
    if (dev_) {
        if (const wchar_t* w = hid_error(dev_)) {
            std::string extra;
            for (const wchar_t* c = w; *c && extra.size() < 160; ++c) {
                extra += (*c < 128) ? char(*c) : '?';
            }
            if (!extra.empty()) error_ += " (" + extra + ")";
        }
    }
}

bool Flasher::firmwareOf(uint16_t pid, uint16_t* out)
{
    hid_device_info* list = hid_enumerate(kVendorId, pid);
    bool found = false;
    if (list) {
        *out = list->release_number;
        found = true;
        hid_free_enumeration(list);
    }
    return found;
}

}  // namespace fw
