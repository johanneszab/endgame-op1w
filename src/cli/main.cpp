// egg-cli — command-line configuration for the Endgame Gear OP1w 4k v2.
//
// Writes go out as whole blocks, so changing one setting means supplying every
// other value in that block. Every field can be read back from the device's
// config blob, so each command is a read-modify-write against live state.

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "egg/device.h"
#include "egg/settings.h"

using namespace egg;

namespace {

[[noreturn]] void die(const std::string& msg)
{
    std::cerr << "error: " << msg << "\n";
    std::exit(1);
}

bool parseBool(const std::string& s)
{
    if (s == "1" || s == "on" || s == "true" || s == "yes")  return true;
    if (s == "0" || s == "off" || s == "false" || s == "no") return false;
    die("expected on/off, got " + s);
}

int parseInt(const std::string& s)
{
    try {
        return std::stoi(s, nullptr, 0);
    } catch (...) {
        die("expected a number, got " + s);
    }
}

double parseDouble(const std::string& s)
{
    try {
        return std::stod(s);
    } catch (...) {
        die("expected a number, got " + s);
    }
}

DecodedConfig currentConfig(Device& dev)
{
    std::array<uint8_t, kBlobSize> blob{};
    if (!dev.readConfigBlob(blob)) {
        die("cannot read configuration: " + dev.lastError());
    }
    return decodeBlob(blob);
}

void writeSensor(Device& dev, const SensorBlock& s)
{
    uint8_t buf[kSensorPayload];
    s.encode(buf);
    if (!dev.writeSensorBlock(buf)) {
        die("write failed: " + dev.lastError());
    }
}

void writePower(Device& dev, const PowerBlock& p)
{
    uint8_t buf[kPowerPayload];
    p.encode(buf);
    if (!dev.writePowerBlock(buf)) {
        die("write failed: " + dev.lastError());
    }
}

void writeButtons(Device& dev, const PowerBlock& p, ButtonTable t)
{
    // The per-button filter lives in the button record too; keep it in step so
    // writing the table cannot silently revert it.
    syncFilters(p, t);

    uint8_t buf[kButtonTableBytes];
    t.encode(buf);
    if (!dev.writeButtonTable(buf)) {
        die("write failed: " + dev.lastError());
    }
}

size_t buttonIndex(const std::string& name)
{
    static const char* keys[] = {"left", "right", "middle", "back", "forward",
                                 "special", "wheel-up", "wheel-down"};
    for (size_t i = 0; i < kButtonCount; ++i) {
        if (name == keys[i]) {
            return i;
        }
    }
    die("unknown button " + name + " (left right middle back forward "
        "special wheel-up wheel-down)");
}

std::string timeoutText(bool enabled, uint8_t minutes)
{
    return enabled ? std::to_string(minutes) + " min" : std::string("disabled");
}

void cmdInfo(Device& dev)
{
    const auto& m = dev.model();
    std::cout << "Device      : " << dev.info().product
              << " (" << (dev.info().wired ? "wired" : "wireless") << ")\n"
              << "Model       : " << m.name;
    if (m.isDongle) {
        std::cout << "  (the dongle is shared across all four wireless models,"
                     " so the mouse cannot be identified from USB alone)";
    }
    std::cout << "\nPath        : " << dev.info().path << "\n";

    // The dongle answers over USB whether or not the mouse is awake.
    if (auto v = dev.dongleFirmware()) {
        std::cout << "Dongle FW   : " << v->toString() << "\n";
    }

    if (auto v = dev.mouseFirmware()) {
        std::cout << "Mouse FW    : " << v->toString() << "\n";
        if (auto s = dev.batteryStatus()) {
            std::cout << "Battery     : " << static_cast<int>(s->percent) << " %\n"
                      << "Signal      : " << static_cast<int>(s->signal) << "\n";
        } else {
            std::cout << "Battery     : unavailable\n";
        }
    } else {
        std::cout << "Mouse FW    : -\n"
                  << "Battery     : -\n"
                  << "Link        : mouse asleep or out of range\n";
    }
}

// Prints device notifications as they arrive. Generates no USB traffic — the
// dongle pushes these on its own.
void cmdListen(Device& dev)
{
    std::cout << "Listening for device notifications (Ctrl-C to stop).\n";
    for (;;) {
        while (auto ev = dev.pollEvent()) {
            std::cout << ev->toHex() << "   ";
            if (ev->isBattery()) {
                std::cout << "battery " << static_cast<int>(ev->batteryPercent())
                          << " %, signal " << static_cast<int>(ev->signalLevel());
            } else if (ev->isLinkState()) {
                std::cout << (ev->linkUp() ? "link up (mouse awake)"
                                           : "link down (mouse asleep)");
            } else {
                std::cout << "unknown event";
            }
            std::cout << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void cmdShow(Device& dev)
{
    const DecodedConfig c = currentConfig(dev);

    std::cout << "CPI stages  : ";
    for (size_t i = 0; i < c.sensor.stages.size(); ++i) {
        const auto& s = c.sensor.stages[i];
        if (i == c.sensor.activeStage) std::cout << "[";
        std::cout << s.x;
        if (s.x != s.y) std::cout << "/" << s.y;
        if (i == c.sensor.activeStage) std::cout << "]";
        std::cout << (i + 1 < c.sensor.stages.size() ? ", " : "\n");
    }

    std::cout << std::fixed << std::setprecision(1)
              << "CPI levels  : " << static_cast<int>(c.sensor.cpiLevels) << "\n"
              << "LOD         : " << lodIndexToMillimetres(c.sensor.lodIndex) << " mm\n"
              << "Angle snap  : " << (c.sensor.angleSnapping ? "on" : "off") << "\n"
              << "Ripple ctrl : " << (c.sensor.rippleControl ? "on" : "off") << "\n"
              << "Angle tuning: " << static_cast<int>(c.sensor.angleTuning) << " deg\n"
              << "LED liftoff : " << (c.sensor.ledOnLiftOff ? "on" : "off") << "\n"
              << "Polling     : " << pollingLabel(c.power.pollingMode) << "\n"
              << "Motion sync : " << (c.power.motionSync ? "on" : "off") << "\n"
              << "Glass mode  : " << (c.power.glassMode ? "on" : "off") << "\n"
              << "Max sensor  : " << (c.power.forceMaxFps() ? "on" : "off") << "\n"
              << "Slamclick   : " << (c.power.slamclick() ? "on" : "off") << "\n"
              << "Multiclick  : " << (c.power.multiclick() ? "on" : "off") << "\n"
              << "Power saving: " << timeoutText(c.power.powerSavingEnabled,
                                                 c.power.powerSavingMinutes) << "\n"
              << "Deep sleep  : " << timeoutText(c.power.deepSleepEnabled,
                                                 c.power.deepSleepMinutes) << "\n"
              << "Left-handed : " << (c.buttons.isLeftHanded() ? "on" : "off") << "\n";

    std::cout << "Click filter:\n";
    for (size_t i = 0; i < kFilterButtonCount; ++i) {
        std::cout << "  " << std::left << std::setw(12) << buttonName(i);
        switch (filterModeOf(c.power.buttonFilter[i])) {
        case ButtonFilterMode::GxSafe:  std::cout << "SPDT GX Safe Mode\n";  break;
        case ButtonFilterMode::GxSpeed: std::cout << "SPDT GX Speed Mode\n"; break;
        case ButtonFilterMode::Count:
            // A count, not a debounce time - the vendor UI shows a bare number
            // and warns that lower values do not reduce click latency.
            std::cout << static_cast<int>(c.power.buttonFilter[i]) << "\n";
            break;
        }
    }

    std::cout << "Buttons     :\n";
    for (size_t i = 0; i < c.buttons.entries.size(); ++i) {
        std::cout << "  " << std::left << std::setw(12) << buttonName(i)
                  << c.buttons.entries[i].describe() << "\n";
    }
}

void cmdBlob(Device& dev)
{
    std::array<uint8_t, kBlobSize> blob{};
    if (!dev.readConfigBlob(blob)) {
        die("cannot read configuration: " + dev.lastError());
    }
    for (size_t i = 0; i < blob.size(); i += 16) {
        std::cout << std::hex << std::setw(4) << std::setfill('0') << i << "  ";
        for (size_t j = 0; j < 16; ++j) {
            std::cout << std::setw(2) << static_cast<int>(blob[i + j]) << ' ';
        }
        std::cout << "\n";
    }
    std::cout << std::dec << std::setfill(' ');
}

void usage()
{
    std::cout <<
        "usage: egg-cli <command> [args]\n"
        "\n"
        "  info                          device, firmware and battery\n"
        "  show                          current configuration\n"
        "  blob                          hexdump the raw 1024-byte config blob\n"
        "  listen                        print device notifications as they arrive\n"
        "\n"
        "  set cpi <1-4> <x> [y]         CPI for one stage, in CPI units\n"
        "  set cpi-levels <1-4>          number of active stages\n"
        "  set active-stage <1-4>        which stage is selected\n"
        "  set lod <mm>                  lift-off distance, 0.7 - 2.0\n"
        "  set angle-snap <on|off>\n"
        "  set ripple <on|off>\n"
        "  set angle-tuning <-30..30>    degrees\n"
        "  set led-liftoff <on|off>\n"
        "  set polling <4000|2000|1000|1000ps|125>\n"
        "                                1000ps = 1000 Hz with wireless power\n"
        "                                saving; 125 = office mode\n"
        "  set motion-sync <on|off>\n"
        "  set glass-mode <on|off>\n"
        "  set max-sensor-fps <on|off>\n"
        "  set slamclick <on|off>\n"
        "  set multiclick <on|off>\n"
        "  set power-saving <1-120|off>\n"
        "  set deep-sleep <1-120|off>\n"
        "  set click-filter <button> <1-15|gx-safe|gx-speed>\n"
        "  set left-handed <on|off>\n"
        "\n"
        "  map <button> mouse <left|right|middle|back|forward>\n"
        "  map <button> key <mods> <usage>   mods: bitmask, usage: HID code\n"
        "  map <button> wheel <up|down>\n"
        "  map <button> consumer <usage>     e.g. 0xEA = volume down\n"
        "  map <button> cpi-cycle            cycle through the CPI stages\n"
        "  map <button> cpi <x> [y]          fixed CPI, in CPI units\n"
        "  map <button> none\n"
        "\n"
        "  pair                          re-pair the mouse to the dongle\n"
        "  factory-reset                 restore the mouse to defaults\n"
        "\n"
        "buttons: left right middle back forward special wheel-up wheel-down\n"
        "SPDT modes apply to left and right only.\n";
}

int doSet(Device& dev, const std::vector<std::string>& args)
{
    if (args.size() < 3) {
        usage();
        return 1;
    }
    const std::string& key = args[1];
    DecodedConfig cfg = currentConfig(dev);

    // ---- sensor block ----
    static const char* sensorKeys[] = {"cpi", "cpi-levels", "active-stage", "lod",
                                       "angle-snap", "ripple", "angle-tuning",
                                       "led-liftoff"};
    for (const char* k : sensorKeys) {
        if (key != k) {
            continue;
        }
        if (key == "cpi") {
            if (args.size() < 4) die("set cpi <1-4> <x> [y]");
            const int stage = parseInt(args[2]);
            if (stage < 1 || stage > static_cast<int>(kCpiStageCount)) die("stage must be 1-4");
            const int x = parseInt(args[3]);
            const int y = args.size() > 4 ? parseInt(args[4]) : x;
            if (x < 50 || x > 26000 || y < 50 || y > 26000) die("CPI out of range");
            cfg.sensor.stages[stage - 1].x = static_cast<uint16_t>(x);
            cfg.sensor.stages[stage - 1].y = static_cast<uint16_t>(y);
            // Set only when the caller actually passed a Y value.
            if (args.size() > 4) {
                cfg.sensor.stages[stage - 1].xySplit = (x == y) ? 0 : 1;
            }
        } else if (key == "cpi-levels") {
            const int n = parseInt(args[2]);
            if (n < 1 || n > static_cast<int>(kCpiStageCount)) die("cpi-levels must be 1-4");
            cfg.sensor.cpiLevels = static_cast<uint8_t>(n);
        } else if (key == "active-stage") {
            const int n = parseInt(args[2]);
            if (n < 1 || n > static_cast<int>(kCpiStageCount)) die("active-stage must be 1-4");
            cfg.sensor.activeStage = static_cast<uint8_t>(n - 1);
        } else if (key == "lod") {
            const double mm = parseDouble(args[2]);
            if (mm < 0.7 || mm > 2.0) die("lod must be 0.7 - 2.0 mm");
            cfg.sensor.lodIndex = lodMillimetresToIndex(mm);
        } else if (key == "angle-snap") {
            cfg.sensor.angleSnapping = parseBool(args[2]);
        } else if (key == "ripple") {
            cfg.sensor.rippleControl = parseBool(args[2]);
        } else if (key == "angle-tuning") {
            if (!dev.model().hasAngleTuning) {
                die(std::string("sensor angle tuning is not available on ")
                    + dev.model().name);
            }
            const int d = parseInt(args[2]);
            if (d < -30 || d > 30) die("angle-tuning must be -30..30");
            cfg.sensor.angleTuning = static_cast<int8_t>(d);
        } else {
            cfg.sensor.ledOnLiftOff = parseBool(args[2]);
        }
        writeSensor(dev, cfg.sensor);
        std::cout << key << " updated\n";
        return 0;
    }

    // ---- button table ----
    if (key == "left-handed") {
        cfg.buttons.setLeftHanded(parseBool(args[2]));
        writeButtons(dev, cfg.power, cfg.buttons);
        std::cout << "left-handed updated\n";
        return 0;
    }

    // ---- power block ----
    if (key == "polling") {
        const std::string& v = args[2];
        uint8_t mode = 0;
        if (v == "4000")        mode = static_cast<uint8_t>(PollingMode::Hz4000);
        else if (v == "2000")   mode = static_cast<uint8_t>(PollingMode::Hz2000);
        else if (v == "1000")   mode = static_cast<uint8_t>(PollingMode::Hz1000);
        else if (v == "1000ps") mode = static_cast<uint8_t>(PollingMode::Hz1000PowerSave);
        else if (v == "125")    mode = static_cast<uint8_t>(PollingMode::Hz125Office);
        else die("polling must be 4000, 2000, 1000, 1000ps or 125");
        cfg.power.pollingMode = mode;
    } else if (key == "motion-sync") {
        cfg.power.motionSync = parseBool(args[2]);
    } else if (key == "glass-mode") {
        if (!dev.model().hasGlassMode) {
            die(std::string("glass mode is not available on ") + dev.model().name);
        }
        cfg.power.glassMode = parseBool(args[2]);
    } else if (key == "max-sensor-fps") {
        if (!dev.model().hasForceMaxFps) {
            die(std::string("force max sensor FPS is not available on ")
                + dev.model().name);
        }
        cfg.power.setFlag(kForceMaxSensorFps, parseBool(args[2]));
    } else if (key == "slamclick") {
        cfg.power.setFlag(kSlamclickFilter, parseBool(args[2]));
    } else if (key == "multiclick") {
        cfg.power.setFlag(kMulticlickFilter, parseBool(args[2]));
    } else if (key == "power-saving" || key == "deep-sleep") {
        const bool off = (args[2] == "off");
        const int  min = off ? 1 : parseInt(args[2]);
        if (!off && (min < kTimeoutMinMinutes || min > kTimeoutMaxMinutes)) {
            die("timeout must be 1-120 minutes, or off");
        }
        if (key == "power-saving") {
            cfg.power.powerSavingEnabled = !off;
            cfg.power.powerSavingMinutes = static_cast<uint8_t>(min);
        } else {
            cfg.power.deepSleepEnabled = !off;
            cfg.power.deepSleepMinutes = static_cast<uint8_t>(min);
        }
    } else if (key == "click-filter") {
        if (args.size() < 4) die("set click-filter <button> <1-15|gx-safe|gx-speed>");
        const size_t idx = buttonIndex(args[2]);
        if (idx >= kFilterButtonCount) {
            die("click-filter applies to left, right, middle, back and forward only");
        }
        const std::string& v = args[3];
        if (v == "gx-safe" || v == "gx-speed") {
            if (!buttonHasSpdt(idx)) {
                die("SPDT modes are available on the left and right buttons only");
            }
            cfg.power.buttonFilter[idx] = filterRawFor(
                v == "gx-safe" ? ButtonFilterMode::GxSafe : ButtonFilterMode::GxSpeed, 0);
        } else {
            const int n = parseInt(v);
            if (n < 1 || n > 15) die("filter count must be 1-15");
            cfg.power.buttonFilter[idx] =
                filterRawFor(ButtonFilterMode::Count, static_cast<uint8_t>(n));
        }
    } else {
        die("unknown setting " + key + " (try --help)");
    }

    writePower(dev, cfg.power);
    std::cout << key << " updated\n";
    return 0;
}

int doMap(Device& dev, const std::vector<std::string>& args)
{
    if (args.size() < 3) {
        usage();
        return 1;
    }
    const size_t idx = buttonIndex(args[1]);
    const std::string& kind = args[2];

    DecodedConfig cfg = currentConfig(dev);
    ButtonEntry& e = cfg.buttons.entries[idx];
    // Switching away from a Fixed CPI binding must not leave its CPI bytes.
    e.clearPayload();

    if (kind == "none") {
        e.type = static_cast<uint8_t>(ActionType::Disabled);
        e.code = 0;
    } else if (kind == "cpi-cycle") {
        e.type = static_cast<uint8_t>(ActionType::Special);
        e.code = 0xF1;
    } else if (kind == "cpi") {
        if (args.size() < 4) die("map <button> cpi <x> [y]");
        const int x = parseInt(args[3]);
        const int y = args.size() > 4 ? parseInt(args[4]) : x;
        if (x < 50 || x > 26000 || y < 50 || y > 26000) die("CPI out of range");
        e.setFixedCpi(static_cast<uint16_t>(x), static_cast<uint16_t>(y));
    } else if (kind == "mouse") {
        if (args.size() < 4) die("map <button> mouse <left|right|middle|back|forward>");
        static const char* names[] = {"left", "right", "middle", "back", "forward"};
        int which = -1;
        for (int i = 0; i < 5; ++i) {
            if (args[3] == names[i]) { which = i; break; }
        }
        if (which < 0) die("unknown mouse button " + args[3]);
        e.type = static_cast<uint8_t>(ActionType::MouseButton);
        e.code = static_cast<uint8_t>(1u << which);
    } else if (kind == "wheel") {
        if (args.size() < 4) die("map <button> wheel <up|down>");
        e.type = static_cast<uint8_t>(ActionType::Wheel);
        e.code = (args[3] == "up") ? 0x01 : 0xFF;
    } else if (kind == "key") {
        if (args.size() < 5) die("map <button> key <mods> <usage>");
        e.type = static_cast<uint8_t>(ActionType::Keyboard);
        e.code = static_cast<uint8_t>(parseInt(args[3]));
        e.key  = static_cast<uint8_t>(parseInt(args[4]));
    } else if (kind == "consumer") {
        if (args.size() < 4) die("map <button> consumer <usage>");
        e.type = static_cast<uint8_t>(ActionType::Consumer);
        e.code = static_cast<uint8_t>(parseInt(args[3]));
    } else {
        die("unknown action " + kind);
    }

    const std::string described = e.describe();
    writeButtons(dev, cfg.power, cfg.buttons);
    std::cout << buttonName(idx) << " -> " << described << "\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty() || args[0] == "-h" || args[0] == "--help") {
        usage();
        return 0;
    }

    Device dev;
    if (!dev.open()) {
        die(dev.lastError());
    }

    const std::string& cmd = args[0];

    if (cmd == "info") { cmdInfo(dev); return 0; }
    if (cmd == "show") { cmdShow(dev); return 0; }
    if (cmd == "blob")   { cmdBlob(dev);   return 0; }
    if (cmd == "listen") { cmdListen(dev); return 0; }
    if (cmd == "set")  { return doSet(dev, args); }
    if (cmd == "map")  { return doMap(dev, args); }

    if (cmd == "pair") {
        if (!dev.pair()) {
            die("pairing failed: " + dev.lastError());
        }
        std::cout << "Pairing started. The mouse will drop and reconnect.\n";
        return 0;
    }

    if (cmd == "factory-reset") {
        if (!dev.factoryReset()) {
            die("factory reset failed: " + dev.lastError());
        }
        std::cout << "Factory reset sent.\n";
        return 0;
    }

    usage();
    return 1;
}
