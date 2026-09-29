#include "device.h"

// Distributions disagree on whether the header sits at <hidapi.h> (Debian's
// pkg-config adds -I/usr/include/hidapi) or <hidapi/hidapi.h>.
#if __has_include(<hidapi.h>)
#  include <hidapi.h>
#else
#  include <hidapi/hidapi.h>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <thread>

namespace vole {
namespace {

void sleepMs(int ms)
{
    if (ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

std::string narrow(const wchar_t* w)
{
    if (!w) {
        return {};
    }
    std::string s;
    for (; *w; ++w) {
        s.push_back(*w < 128 ? static_cast<char>(*w) : '?');
    }
    return s;
}

// How long to wait after a write before the response is readable. The vendor
// tool uses 150-360 ms depending on the command; the blob read is the slow one.
int settleFor(Cmd cmd)
{
    return cmd == Cmd::ReadConfig ? kBulkSettleMs : kDefaultSettleMs;
}

struct HidInit {
    HidInit() { hid_init(); }
    ~HidInit() { hid_exit(); }
};

void ensureHidInit()
{
    static HidInit once;
    (void)once;
}

}  // namespace

const ModelInfo* modelForMousePid(uint16_t pid)
{
    for (const ModelInfo& m : kModels) {
        if (m.mousePid == pid) {
            return &m;
        }
    }
    return nullptr;
}

std::string Version::toString() const
{
    // The device encodes the version as HEX digits, not a decimal pair: 1.10 is
    // 0x0110 and 1.08 is 0x0108. The vendor's updater formats the whole
    // bcdDevice with "%x" and divides by 100, which is the same thing.
    //
    // Printing the minor as decimal was wrong and invisible for a year, because
    // every firmware seen until now had a minor digit of 9 or less, where the
    // two readings coincide. Firmware 1.10 showed up as "1.16".
    // re/firmware/FIRMWARE.md §5. [CAP]
    char buf[16];
    std::snprintf(buf, sizeof buf, "%x.%02x", major, minor);
    return buf;
}

Device::~Device()
{
    close();
}

std::vector<DeviceInfo> Device::enumerate()
{
    ensureHidInit();

    std::vector<DeviceInfo> found;
    // The dongle, plus every model's own PID for when it is plugged in by cable.
    std::vector<uint16_t> pids;
    pids.push_back(kProductDongle);
    for (const ModelInfo& m : kModels) {
        pids.push_back(m.cabledPid);
    }

    for (uint16_t pid : pids) {
        hid_device_info* list = hid_enumerate(kVendorId, pid);
        for (hid_device_info* it = list; it; it = it->next) {
            DeviceInfo d;
            d.path            = it->path ? it->path : "";
            d.vendorId        = it->vendor_id;
            d.productId       = it->product_id;
            d.interfaceNumber = it->interface_number;
            d.usagePage       = it->usage_page;
            d.usage           = it->usage;
            d.product         = narrow(it->product_string);
            d.wired           = (pid != kProductDongle);
            if (!d.path.empty()) {
                found.push_back(std::move(d));
            }
        }
        hid_free_enumeration(list);
    }

    // Best match first. On Windows/macOS each collection is its own node, so
    // the usage fields identify it exactly. On Linux one hidraw node covers a
    // whole USB interface and reports only the *first* collection's usage, so
    // fall back to the interface number.
    std::stable_sort(found.begin(), found.end(),
        [](const DeviceInfo& a, const DeviceInfo& b) {
            auto rank = [](const DeviceInfo& d) {
                if (d.usagePage == kUsagePage && d.usage == kUsage) return 0;
                if (d.interfaceNumber == kInterfaceNumber)          return 1;
                return 2;
            };
            return rank(a) < rank(b);
        });

    return found;
}

bool Device::open()
{
    auto candidates = enumerate();
    if (candidates.empty()) {
        setError("no Endgame Gear mouse or dongle found (is it plugged in?)");
        return false;
    }

    for (const auto& c : candidates) {
        if (!openPath(c.path)) {
            continue;
        }
        if (probe()) {
            // Not fatal if this fails — the mouse may be asleep. model()
            // stays kUnknownModel and model-specific writes stay disabled
            // until a later identifyModel() succeeds.
            identifyModel();
            return true;
        }
        close();
    }

    setError("found the device but no interface answered a probe "
             "(permissions? see udev/70-vole.rules)");
    return false;
}

bool Device::openPath(const std::string& path)
{
    ensureHidInit();
    close();

    hid_device* h = hid_open_path(path.c_str());
    if (!h) {
        setError("cannot open " + path + " (permission denied?)");
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    dev_ = h;
    for (const auto& c : enumerate()) {
        if (c.path == path) {
            info_ = c;
            break;
        }
    }
    info_.path = path;
    return true;
}

void Device::close()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (dev_) {
        hid_close(dev_);
        dev_ = nullptr;
    }
}

std::string Device::lastError() const
{
    std::lock_guard<std::mutex> lock(errorMutex_);
    return error_;
}

void Device::setError(std::string msg)
{
    std::lock_guard<std::mutex> lock(errorMutex_);
    error_ = std::move(msg);
}

bool Device::sendReport(const uint8_t* buf, size_t len)
{
    // Retry on the transient errors the vendor tool retries on. hidapi does not
    // expose the Windows error codes, so on Linux any failure is retried the
    // same number of times.
    for (int attempt = 0; attempt < kWriteRetries; ++attempt) {
        if (hid_send_feature_report(dev_, buf, len) >= 0) {
            return true;
        }
        sleepMs(kRetryDelayMs);
    }
    setError("SET_FEATURE failed: " + narrow(hid_error(dev_)));
    return false;
}

bool Device::getReport(uint8_t* buf, size_t len)
{
    const uint8_t reportId = buf[0];

    for (int attempt = 0; attempt < kReadRetries; ++attempt) {
        if (hid_get_feature_report(dev_, buf, len) >= 0) {
            // The device answers 0x03 while it is still working on the command.
            if (buf[kOffStatus] != kStatusBusy) {
                return true;
            }
            int delay = 0;
            for (int busy = 0; busy < kBusyRetries; ++busy) {
                delay += kBusyDelayStepMs;
                sleepMs(delay);
                std::memset(buf, 0, len);
                buf[0] = reportId;
                if (hid_get_feature_report(dev_, buf, len) >= 0 &&
                    buf[kOffStatus] == kStatusOk) {
                    return true;
                }
            }
            setError("device stayed busy (status 0x03)");
            return false;
        }
        sleepMs(kRetryDelayMs);
        std::memset(buf, 0, len);
        buf[0] = reportId;
    }

    setError("GET_FEATURE failed: " + narrow(hid_error(dev_)));
    return false;
}

bool Device::command(Cmd cmd, Target target,
                     const uint8_t* payload, size_t payloadLen,
                     uint8_t chunkIndex, Response* out,
                     uint8_t declaredLength)
{
    if (payloadLen > kCmdReportSize - kPayloadOffset) {
        setError("payload too large for a 64-byte report");
        return false;
    }
    if (declaredLength == 0) {
        declaredLength = static_cast<uint8_t>(payloadLen);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (!dev_) {
        setError("device not open");
        return false;
    }

    std::array<uint8_t, kCmdReportSize> tx{};
    tx[kOffReportId] = kReportCmd;
    tx[kOffCommand]  = static_cast<uint8_t>(cmd);
    tx[kOffTarget]   = static_cast<uint8_t>(target);
    tx[kOffLength]   = declaredLength;
    tx[kOffChunk]    = chunkIndex;
    if (payload && payloadLen) {
        std::memcpy(tx.data() + kPayloadOffset, payload, payloadLen);
    }

    if (!sendReport(tx.data(), tx.size())) {
        return false;
    }
    sleepMs(settleFor(cmd));

    Response scratch;
    Response& rx = out ? *out : scratch;
    rx.raw.fill(0);
    rx.raw[0] = kReportCmd;
    if (!getReport(rx.raw.data(), rx.raw.size())) {
        return false;
    }
    if (rx.status() != kStatusOk) {
        setError("device returned status 0x" +
                 std::string(1, "0123456789ABCDEF"[rx.status() >> 4]) +
                 std::string(1, "0123456789ABCDEF"[rx.status() & 0xF]));
        return false;
    }
    return true;
}

bool Device::probe()
{
    Response r;
    // The wireless probe, which is what the vendor tool issues before every
    // command. Over a dongle this answers even with the mouse asleep.
    if (command(Cmd::Probe, Target::Dongle, &r) ||
        command(Cmd::Probe, Target::Mouse, &r)) {
        return true;
    }

    // A cabled mouse rejects cmd 0x0F outright with status 0x07, whatever
    // target byte it is given — there is no dongle for the selector to select.
    // It answers cmd 0x0E perfectly well, so use that as the liveness test
    // instead. Tried second so the wireless path keeps its existing behaviour
    // exactly, including answering while the mouse is asleep, which 0x0E
    // cannot do. PROTOCOL.md section 13. [DEV]
    return command(Cmd::MouseInfo, Target::None, &r);
}

std::optional<Device::BatteryStatus> Device::batteryStatus()
{
    Response r;
    if (!command(Cmd::Battery, Target::None, &r)) {
        return std::nullopt;
    }
    BatteryStatus s;
    s.undecoded1 = r.payload()[1];
    uint8_t pct  = r.payload()[0];

    // While the cable is in, this byte is NOT a state of charge, and the
    // vendor's own tool never shows it as one: under 100 it prints "Charging",
    // at 100 or over it clamps and prints "100%" (re/protocol_pass2.c
    // FUN_00417439, and the v1's 0xB4 event arm in re/v1/notify.c, which gates
    // the same rule on its connection-mode byte being 0x11 = cabled). [BIN]
    //
    // Measurement says why rather than merely that: a v2 sitting at 95% over
    // the dongle reported 55 on the cable minutes later. A battery does not
    // lose 40 points by being plugged in, so the number is not what it looks
    // like -- most likely the charge current spoils the estimate. Reporting it
    // as a percentage is worse than reporting nothing, because it looks
    // right. [DEV]
    //
    // The vendor's threshold implies the byte does still reach 100 when the
    // cell is full, which is the one thing worth surfacing: charging vs done.
    s.charging = info_.wired && pct < 100;
    if (pct > 100) {
        // Wireless: a percentage over 100 is a bad read, and there is no
        // charging story to explain it away. Cabled: clamp, as the vendor does.
        if (!info_.wired) {
            setError("implausible battery reading");
            return std::nullopt;
        }
        pct = 100;
    }
    s.percent = pct;
    return s;
}

std::optional<int> Device::batteryPercent()
{
    if (auto s = batteryStatus()) {
        return s->percent;
    }
    return std::nullopt;
}

std::optional<Version> Device::dongleFirmware()
{
    Response r;
    if (!command(Cmd::DongleInfo, Target::None, &r)) {
        return std::nullopt;
    }
    return Version{r.payload()[0], r.payload()[1]};
}

std::optional<Version> Device::mouseFirmware()
{
    Response r;
    if (!command(Cmd::MouseInfo, Target::None, &r)) {
        return std::nullopt;
    }
    return Version{r.payload()[6], r.payload()[7]};
}

bool Device::readConfigBlob(std::array<uint8_t, kBlobSize>& out)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!dev_) {
        setError("device not open");
        return false;
    }

    std::array<uint8_t, kCmdReportSize> tx{};
    tx[kOffReportId] = kReportCmd;
    tx[kOffCommand]  = static_cast<uint8_t>(Cmd::ReadConfig);
    if (!sendReport(tx.data(), tx.size())) {
        return false;
    }
    sleepMs(kBulkSettleMs);

    // The answer comes back on the 1041-byte bulk report, not the 64-byte one.
    std::vector<uint8_t> rx(kBulkReportSize, 0);
    rx[0] = kReportBulk;
    if (!getReport(rx.data(), rx.size())) {
        return false;
    }
    if (rx[kOffStatus] != kStatusOk) {
        setError("config read returned a non-OK status");
        return false;
    }
    std::memcpy(out.data(), rx.data() + kPayloadOffset, out.size());
    return true;
}

std::string Notification::toHex() const
{
    static const char* digits = "0123456789ABCDEF";
    std::string s;
    for (size_t i = 0; i < raw.size(); ++i) {
        if (i) {
            s += ' ';
        }
        s += digits[raw[i] >> 4];
        s += digits[raw[i] & 0x0F];
    }
    return s;
}

std::optional<Notification> Device::pollEvent()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!dev_) {
        return std::nullopt;
    }

    // On Linux one hidraw node covers the whole interface, so the keyboard and
    // consumer reports produced by remapped buttons arrive here too. Drain the
    // queue and keep only the notification report.
    uint8_t buf[kCmdReportSize];
    for (;;) {
        std::memset(buf, 0, sizeof buf);
        const int n = hid_read_timeout(dev_, buf, sizeof buf, 0);   // 0 = non-blocking
        if (n <= 0) {
            return std::nullopt;
        }
        if (buf[0] == kNotifyReportId) {
            Notification ev;
            const size_t copy = std::min(static_cast<size_t>(n), kNotifyReportSize);
            std::memcpy(ev.raw.data(), buf, copy);
            return ev;
        }
    }
}

bool Device::identifyModel()
{
    // Cabled: the USB PID is the mouse's own, so no query is needed.
    if (info_.wired) {
        for (const ModelInfo& m : kModels) {
            if (m.cabledPid == info_.productId) {
                model_ = &m;
                return true;
            }
        }
    }

    // Wireless: every dongle enumerates as 0x1970, so ask the mouse. Cmd 0x0E
    // answers with its own VID at payload +0..+1 and PID at +2..+3.
    Response r;
    if (!command(Cmd::MouseInfo, Target::None, &r)) {
        return false;   // mouse asleep or out of range
    }

    const uint16_t vid = static_cast<uint16_t>(r.payload()[0] | (r.payload()[1] << 8));
    const uint16_t pid = static_cast<uint16_t>(r.payload()[2] | (r.payload()[3] << 8));
    if (vid != kVendorId) {
        setError("cmd 0x0E reported an unexpected vendor ID");
        return false;
    }

    const ModelInfo* m = modelForMousePid(pid);
    if (!m) {
        char buf[64];
        std::snprintf(buf, sizeof buf,
                      "unrecognised mouse product ID 0x%04X", pid);
        setError(buf);
        return false;
    }
    model_ = m;
    return true;
}

bool Device::mouseAwake()
{
    // Cmd 0x0E targets the mouse; the dongle answers 0x0D either way.
    Response r;
    return command(Cmd::MouseInfo, Target::None, &r);
}

// Refusing here costs little in practice: cmd 0x0E only fails while the mouse
// is unreachable, and a write aimed at Target::Mouse would not have arrived
// either. Once it wakes, reload() and the link-up handler both re-identify it.
bool Device::requireModel(const char* what)
{
    if (model_) {
        return true;
    }
    setError(std::string(what) + " cannot be written until the mouse is "
             "identified: cmd 0x0E has not answered, which normally means it "
             "is asleep. Wake it and retry. Both generations share one dongle "
             "USB ID, and they encode lift-off distance on incompatible "
             "scales, so writing this blind would corrupt it.");
    return false;
}

bool Device::writeSensorBlock(const uint8_t* payload)
{
    // Whole-block write, and the block carries lift-off distance, whose scale
    // differs between generations. There is no way to rewrite the CPI stages
    // without also restating that byte, so this needs a known model.
    if (!requireModel("sensor settings")) {
        return false;
    }

    // Payload +5 is sensor angle tuning, which the v1 does not have. Its vendor
    // tool's serializer has no store for that byte at all, so the v1 always
    // sends 0 there (re/v1/serializers.c, FUN_00404d00). We must match: the
    // blob byte we decode it from means something unestablished on a v1, and
    // whole-block writes leave no way to skip the field. PROTOCOL.md section 12.
    uint8_t buf[kSensorPayload];
    std::memcpy(buf, payload, kSensorPayload);
    if (!model().hasAngleTuning) {
        buf[5] = 0;
    }

    Response r;
    return command(Cmd::WriteSensor, Target::Mouse, buf, kSensorPayload, 0, &r);
}

bool Device::writePowerBlock(const uint8_t* payload)
{
    // Eleven bytes on the wire, length field says ten — exactly what the
    // vendor tool does. The eleventh byte carries sensor glass mode, and the
    // device does act on it.
    //
    // v1 writes ten bytes and declares ten; v2 writes eleven and still declares
    // ten, the eleventh being sensor glass mode. That is why the length field
    // reads low on v2: ten was the truth on v1, and v2 appended a byte without
    // updating it. Confirmed in both vendor binaries.
    //
    // Guessing the length is not safe either way round: sending ten to a v2
    // does not mean "omit glass mode", it means "glass mode off", because the
    // report is zero-filled and the v2 acts on payload +10 regardless of the
    // declared length.
    if (!requireModel("polling and power settings")) {
        return false;
    }
    const size_t len = model().powerPayloadLen;

    Response r;
    return command(Cmd::WritePower, Target::Mouse, payload, len, 0, &r,
                   kPowerDeclaredLength);
}

bool Device::writeButtonTable(const uint8_t* payload)
{
    // Two chunks of 28 bytes, 1-based index in header byte 6.
    for (uint8_t chunk = 1; chunk <= 2; ++chunk) {
        Response r;
        if (!command(Cmd::WriteButtons, Target::Mouse,
                     payload + (chunk - 1) * kButtonChunkPayload,
                     kButtonChunkPayload, chunk, &r)) {
            return false;
        }
    }
    return true;
}

bool Device::pair()
{
    Response r;
    return command(Cmd::Pair, Target::None, &r);
}

bool Device::factoryReset()
{
    Response r;
    return command(Cmd::FactoryReset, Target::None, &r);
}

// ----------------------------------------------------------- conversions ---

uint8_t lodMillimetresToIndex(double mm, LodEncoding enc)
{
    if (enc == LodEncoding::Millimetres) {
        // v1: the value is the millimetre count, and only 1 and 2 exist.
        return static_cast<uint8_t>(std::clamp(
            static_cast<int>(std::lround(mm)), 1, 2));
    }
    const int idx = static_cast<int>(std::lround(mm * 10.0)) - 7;
    return static_cast<uint8_t>(std::clamp(idx, 0, kLodMaxIndexV2));
}

double lodIndexToMillimetres(uint8_t index, LodEncoding enc)
{
    if (enc == LodEncoding::Millimetres) {
        // The v1 tool displays anything unexpected as 1 mm, so match it.
        return (index == 2) ? 2.0 : 1.0;
    }
    return (static_cast<double>(index) + 7.0) / 10.0;
}

LodEncoding effectiveLodEncoding(const ModelInfo& m, bool glassMode)
{
    // The v2 tool's lift-off combo is built by FUN_00411280, which branches on
    // a byte written only by the glass-mode control: glass on gives the
    // two-entry millimetre list, glass off the eleven-entry tenths list. The v1
    // has no glass mode, so this collapses to ModelInfo::lod there. [BIN]
    if (m.hasGlassMode && glassMode) {
        return LodEncoding::Millimetres;
    }
    return m.lod;
}

uint8_t lodConvertForGlassMode(uint8_t index, bool glassNowOn)
{
    // Exactly the vendor's mapping, from the two arms of FUN_00411280. [BIN]
    if (glassNowOn) {
        // tenths -> millimetres
        if (index < 8)  return 1;   // 0.7 .. 1.4 mm  ->  1 mm
        if (index <= 10) return 2;  // 1.5 .. 1.7 mm  ->  2 mm
        return index;               // not a tenths value; leave it alone
    }
    // millimetres -> tenths
    if (index == 1) return 3;       // 1 mm  ->  1.0 mm
    if (index == 2) return 10;      // 2 mm  ->  1.7 mm, the tenths maximum
    return index;
}

std::vector<double> lodOptions(LodEncoding enc)
{
    if (enc == LodEncoding::Millimetres) {
        return {1.0, 2.0};
    }
    // 0.7 .. 1.7 mm, eleven values. NOT 0.7 .. 2.0: the v2 vendor tool adds
    // exactly eleven strings to this combo (FUN_00411280, eleven CB_ADDSTRING
    // of the literals at 0x0055E070 .. 0x0055E0E8, "0.7mm" .. "1.7mm"), so
    // indices 11-13 are bytes no vendor tool has ever written and this project
    // does not write values whose encoding was only guessed at. [BIN]
    //
    // The twelfth literal, "2.0mm" at 0x0055E0F4, belongs to that function's
    // other branch, which the vendor uses when sensor glass mode is on -- see
    // kLodMaxIndexV2, effectiveLodEncoding() and PROTOCOL.md section 4.
    std::vector<double> v;
    for (int i = 0; i <= kLodMaxIndexV2; ++i) {
        v.push_back((static_cast<double>(i) + 7.0) / 10.0);
    }
    return v;
}

}  // namespace vole
