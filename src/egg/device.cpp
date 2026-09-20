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
#include <cstring>
#include <thread>

namespace egg {
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

const ModelInfo& modelFor(uint16_t pid)
{
    for (const ModelInfo& m : kModels) {
        if (m.pid == pid) {
            return m;
        }
    }
    // Unknown PID: assume the dongle's capabilities, which is the only set
    // that has been verified against hardware.
    return kModels[0];
}

std::string Version::toString() const
{
    return std::to_string(major) + "." + (minor < 10 ? "0" : "") + std::to_string(minor);
}

Device::~Device()
{
    close();
}

std::vector<DeviceInfo> Device::enumerate()
{
    ensureHidInit();

    std::vector<DeviceInfo> found;
    for (const ModelInfo& m : kModels) {
        const uint16_t pid = m.pid;
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
            d.wired           = !m.isDongle;
            d.model           = &m;
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
        setError("no Endgame Gear OP1w 4k v2 found (is the dongle plugged in?)");
        return false;
    }

    for (const auto& c : candidates) {
        if (!openPath(c.path)) {
            continue;
        }
        if (probe()) {
            return true;  // it answered; this is the config collection
        }
        close();
    }

    setError("found the device but no interface answered a probe "
             "(permissions? see udev/70-endgamegear.rules)");
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
    return command(Cmd::Probe, Target::Dongle, &r) ||
           command(Cmd::Probe, Target::Mouse, &r);
}

std::optional<Device::BatteryStatus> Device::batteryStatus()
{
    Response r;
    if (!command(Cmd::Battery, Target::None, &r)) {
        return std::nullopt;
    }
    BatteryStatus s;
    s.percent = r.payload()[0];
    s.signal  = r.payload()[1];
    if (s.percent > 100) {
        setError("implausible battery reading");
        return std::nullopt;
    }
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

bool Device::mouseAwake()
{
    // Cmd 0x0E targets the mouse; the dongle answers 0x0D either way.
    Response r;
    return command(Cmd::MouseInfo, Target::None, &r);
}

bool Device::writeSensorBlock(const uint8_t* payload)
{
    Response r;
    return command(Cmd::WriteSensor, Target::Mouse, payload, kSensorPayload, 0, &r);
}

bool Device::writePowerBlock(const uint8_t* payload)
{
    // Eleven bytes on the wire, length field says ten — exactly what the
    // vendor tool does. The eleventh byte carries sensor glass mode, and the
    // device does act on it.
    //
    // The v1 models have no glass mode in their UI, so for those the eleventh
    // byte is not sent at all and the declared length becomes the truth. That
    // also explains why the field says ten: it is the v1 payload size, and v2
    // appended a byte without updating it. **[?]** — not testable here.
    const size_t len = model().hasGlassMode ? kPowerPayload : kPowerDeclaredLength;

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

uint8_t lodMillimetresToIndex(double mm)
{
    const int idx = static_cast<int>(std::lround(mm * 10.0)) - 7;
    return static_cast<uint8_t>(std::clamp(idx, 0, 13));
}

double lodIndexToMillimetres(uint8_t index)
{
    return (static_cast<double>(index) + 7.0) / 10.0;
}

}  // namespace egg
