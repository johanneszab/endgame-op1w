// vole-cli — command-line configuration for the Endgame Gear wireless mice
// (OP1w 4k and XM2w 4k, both generations).
//
// Writes go out as whole blocks, so changing one setting means supplying every
// other value in that block. Every field can be read back from the device's
// config blob, so each command is a read-modify-write against live state.

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "vole/device.h"
#include "vole/settings.h"

using namespace vole;

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

// Settings whose encoding or valid range depends on which mouse is attached.
// Writing one of these blind is how a v1 device ends up with a v2 lift-off
// index, so refuse rather than guess.
void requireIdentifiedModel(Device& dev, const std::string& setting)
{
    if (dev.modelIdentified()) {
        return;
    }
    die("cannot set " + setting + " until the mouse is identified — cmd 0x0E "
        "did not answer, which usually means it is asleep. Move it and retry. "
        "(Every wireless dongle enumerates as the same USB ID, so the model "
        "cannot be inferred without asking the mouse.)");
}

// The vendor tools clamp CPI differently per generation — v1 50..26000, v2
// 10..30000 — so this needs the model. Before it is known, kUnknownModel holds
// the intersection, which cannot be out of range for either.
void requireCpiInRange(Device& dev, int x, int y)
{
    const ModelInfo& m = dev.model();
    if (x < m.cpiMin || x > m.cpiMax || y < m.cpiMin || y > m.cpiMax) {
        char buf[160];
        std::snprintf(buf, sizeof buf,
                      "CPI out of range: %s allows %u-%u%s",
                      dev.modelIdentified() ? m.name : "an unidentified mouse",
                      m.cpiMin, m.cpiMax,
                      dev.modelIdentified()
                          ? ""
                          : " until cmd 0x0E names it (the v2 allows 10-30000)");
        die(buf);
    }
}

// The two generations put lift-off distance on incompatible scales, so an
// unidentified mouse gets the raw byte rather than a number that would be
// wrong on one of them.
std::string lodText(Device& dev, uint8_t lodIndex, bool glassMode)
{
    char buf[96];
    if (!dev.modelIdentified()) {
        std::snprintf(buf, sizeof buf,
                      "raw 0x%02X (scale unknown until the mouse is identified)",
                      lodIndex);
        return buf;
    }
    // Not ModelInfo::lod: on a v2, glass mode switches the scale.
    const LodEncoding enc = effectiveLodEncoding(dev.model(), glassMode);
    // Whole millimetres read "1 mm" / "2 mm"; a decimal would imply a
    // precision that scale does not have.
    const bool whole = enc == LodEncoding::Millimetres;
    std::snprintf(buf, sizeof buf, whole ? "%.0f mm%s" : "%.1f mm%s",
                  lodIndexToMillimetres(lodIndex, enc),
                  (whole && dev.model().hasGlassMode) ? " (glass-mode scale)" : "");
    return buf;
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
    if (!dev.modelIdentified()) {
        std::cout << "  (cmd 0x0E did not answer — wake the mouse and retry;"
                     " every dongle enumerates as the same USB ID, so the model"
                     " cannot be known until it does)";
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
            } else if (ev->isPollingChanged()) {
                std::cout << "polling rate changed on the mouse: "
                          << pollingLabel(ev->pollingModeByte());
            } else if (ev->isLinkState() && (ev->linkUp() || ev->linkDown())) {
                std::cout << (ev->linkUp() ? "link up (mouse awake)"
                                           : "link down (mouse asleep)");
            } else {
                // A code no vendor tool acts on; theirs drop these too. Both
                // 0x30 and 0x31 are known to occur. See PROTOCOL.md 4a.
                std::cout << "event 0x" << std::hex << std::setw(2)
                          << std::setfill('0') << static_cast<int>(ev->code())
                          << std::dec << std::setfill(' ') << ", not decoded";
            }
            std::cout << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void cmdShow(Device& dev)
{
    const DecodedConfig c = currentConfig(dev);

    // The colour is the one the mouse's underside LED shows for that stage; on
    // the device it is the only way to tell the stages apart. It belongs to the
    // stage, not to the CPI value. Brackets mark the active stage.
    std::cout << "CPI stages  : ";
    for (size_t i = 0; i < c.sensor.stages.size(); ++i) {
        const auto& s = c.sensor.stages[i];
        if (i == c.sensor.activeStage) std::cout << "[";
        std::cout << s.x;
        if (s.x != s.y) std::cout << "/" << s.y;
        std::cout << " " << kStageColours[i].name;
        if (i == c.sensor.activeStage) std::cout << "]";
        std::cout << (i + 1 < c.sensor.stages.size() ? ", " : "\n");
    }

    std::cout << std::fixed << std::setprecision(1)
              << "CPI levels  : " << static_cast<int>(c.sensor.cpiLevels) << "\n"
              << "LOD         : " << lodText(dev, c.sensor.lodIndex, c.power.glassMode) << "\n"
              << "Angle snap  : " << (c.sensor.angleSnapping ? "on" : "off") << "\n"
              << "Ripple ctrl : " << (c.sensor.rippleControl ? "on" : "off") << "\n"
              << "LED liftoff : " << (c.sensor.ledOnLiftOff ? "on" : "off") << "\n"
              << "Polling     : " << pollingLabel(c.power.pollingMode) << "\n"
              << "Motion sync : " << (c.power.motionSync ? "on" : "off") << "\n"
              << "Slamclick   : " << (c.power.slamclick() ? "on" : "off") << "\n";

    // Model-specific fields. On a generation that does not implement one, the
    // bit or blob byte we would be reading means something unestablished, so
    // printing a value would be stating something about the device that has
    // not been established. PROTOCOL.md section 12.
    const ModelInfo& m = dev.model();
    const auto showOptional = [&](const char* label, bool supported, const char* value) {
        std::cout << label << ": "
                  << (!dev.modelIdentified() ? "? (mouse not identified)"
                      : !supported           ? "n/a on this model"
                                             : value)
                  << "\n";
    };
    char tuning[24];
    std::snprintf(tuning, sizeof tuning, "%d deg", static_cast<int>(c.sensor.angleTuning));
    showOptional("Angle tuning", m.hasAngleTuning,   tuning);
    showOptional("Glass mode  ", m.hasGlassMode,     c.power.glassMode     ? "on" : "off");
    showOptional("Max sensor  ", m.hasForceMaxFps,   c.power.forceMaxFps() ? "on" : "off");
    showOptional("MotionJitter", m.hasMotionJitter,  c.power.motionJitter()? "on" : "off");
    showOptional("Multiclick  ", m.hasMulticlickAck, c.power.multiclick()  ? "on" : "off");

    std::cout << std::fixed << std::setprecision(1)
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
        "usage: vole-cli <command> [args]\n"
        "\n"
        "  info                          device, firmware and battery\n"
        "  show                          current configuration\n"
        "  blob                          hexdump the raw 1024-byte config blob\n"
        "  listen                        print device notifications as they arrive\n"
        "\n"
        "  set cpi <1-4> <x> [y]         CPI for one stage, in CPI units\n"
        "  set cpi-levels <1-4>          number of active stages\n"
        "  set active-stage <1-4>        which stage is selected; v2 models\n"
        "                                only, the v1 switches it with the\n"
        "                                button under the mouse\n"
        "  set lod <mm>                  lift-off distance; v1 offers 1 and 2,\n"
        "                                v2 offers 0.7 - 1.7 in 0.1 steps\n"
        "  set angle-snap <on|off>\n"
        "  set ripple <on|off>\n"
        "  set angle-tuning <-30..30>    degrees; v2 models only\n"
        "  set led-liftoff <on|off>\n"
        "  set polling <4000|2000|1000|1000ps|125>\n"
        "                                1000ps = 1000 Hz with wireless power\n"
        "                                saving; 125 = office mode\n"
        "  set motion-sync <on|off>\n"
        "  set glass-mode <on|off>       v2 models only\n"
        "  set max-sensor-fps <on|off>   v2 models only\n"
        "  set motion-jitter <on|off>    v1 models only\n"
        "  set slamclick <on|off>\n"
        "  set multiclick <on|off>       v2 models only\n"
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
            requireCpiInRange(dev, x, y);
            cfg.sensor.stages[stage - 1].x = static_cast<uint16_t>(x);
            cfg.sensor.stages[stage - 1].y = static_cast<uint16_t>(y);
            // No xySplit to set: SensorBlock::encode derives it from x != y.
        } else if (key == "cpi-levels") {
            const int n = parseInt(args[2]);
            if (n < 1 || n > static_cast<int>(kCpiStageCount)) die("cpi-levels must be 1-4");
            cfg.sensor.cpiLevels = static_cast<uint8_t>(n);
        } else if (key == "active-stage") {
            requireIdentifiedModel(dev, "active-stage");
            if (!dev.model().hasCpiStageSelect) {
                die(std::string("the ") + dev.model().name +
                    " ignores the active-stage field — switch stages with the "
                    "button underneath the mouse. Its own vendor tool has no "
                    "control for this either. (vole-cli show prints the stage the "
                    "mouse is on, and the LED colour that goes with it.)");
            }
            const int n = parseInt(args[2]);
            if (n < 1 || n > static_cast<int>(kCpiStageCount)) die("active-stage must be 1-4");
            cfg.sensor.activeStage = static_cast<uint8_t>(n - 1);
        } else if (key == "lod") {
            requireIdentifiedModel(dev, "lod");
            const double mm  = parseDouble(args[2]);
            const LodEncoding enc =
                effectiveLodEncoding(dev.model(), cfg.power.glassMode);
            const auto   opts = lodOptions(enc);

            // Snap to the nearest distance this model offers rather than
            // demanding an exact hit, but refuse anything outside its range —
            // v1 offers only 1 and 2 mm, so silently clamping 5 mm to 2 mm
            // would be worse than saying so.
            if (mm < opts.front() - 0.001 || mm > opts.back() + 0.001) {
                char lo[16], hi[16];
                std::snprintf(lo, sizeof lo, "%.1f", opts.front());
                std::snprintf(hi, sizeof hi, "%.1f", opts.back());
                die(std::string("lod must be between ") + lo + " and " + hi
                    + " mm on " + dev.model().name
                    + (cfg.power.glassMode
                           ? " with sensor glass mode on (it selects the"
                             " whole-millimetre scale)"
                           : ""));
            }
            double nearest = opts.front();
            for (double o : opts) {
                if (std::fabs(o - mm) < std::fabs(nearest - mm)) {
                    nearest = o;
                }
            }
            if (std::fabs(nearest - mm) > 0.001) {
                std::printf("note: %s offers %.1f mm, not %.1f — using %.1f\n",
                            dev.model().name, nearest, mm, nearest);
            }
            cfg.sensor.lodIndex = lodMillimetresToIndex(nearest, enc);
        } else if (key == "angle-snap") {
            cfg.sensor.angleSnapping = parseBool(args[2]);
        } else if (key == "ripple") {
            cfg.sensor.rippleControl = parseBool(args[2]);
        } else if (key == "angle-tuning") {
            requireIdentifiedModel(dev, key);
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
    //
    // Glass mode is the one key here that also moves a cmd 0x14 field: it
    // selects the lift-off scale, so the stored byte has to be translated and
    // written with it. Everything else below touches the 0x15 block alone.
    bool alsoWriteSensor = false;

    if (key == "polling") {
        requireIdentifiedModel(dev, "polling");
        const std::string& v = args[2];
        uint8_t mode = 0;
        if (v == "4000")        mode = static_cast<uint8_t>(PollingMode::Hz4000);
        else if (v == "2000")   mode = static_cast<uint8_t>(PollingMode::Hz2000);
        else if (v == "1000")   mode = static_cast<uint8_t>(PollingMode::Hz1000);
        else if (v == "1000ps") mode = static_cast<uint8_t>(PollingMode::Hz1000PowerSave);
        else if (v == "125")    mode = static_cast<uint8_t>(PollingMode::Hz125Office);
        else die("polling must be 4000, 2000, 1000, 1000ps or 125");

        // The power-saving and office-mode values exist only on the v2
        // generation; a v1 device has no UI option that produces them.
        if (!dev.model().hasPowerSavePolling &&
            (mode == static_cast<uint8_t>(PollingMode::Hz1000PowerSave) ||
             mode == static_cast<uint8_t>(PollingMode::Hz125Office))) {
            die(std::string(v) + " is not available on " + dev.model().name
                + " (it offers 1000, 2000 and 4000 only)");
        }
        cfg.power.pollingMode = mode;
    } else if (key == "motion-sync") {
        cfg.power.motionSync = parseBool(args[2]);
    } else if (key == "glass-mode") {
        requireIdentifiedModel(dev, key);
        if (!dev.model().hasGlassMode) {
            die(std::string("glass mode is not available on ") + dev.model().name);
        }
        const bool on = parseBool(args[2]);
        if (on != cfg.power.glassMode) {
            // Glass mode selects the lift-off scale, so the stored byte has to
            // be translated with it or it would mean something else afterwards.
            // PROTOCOL.md section 4. The 0x14 block carries that byte, so this
            // key has to write it too -- see alsoWriteSensor.
            const uint8_t before = cfg.sensor.lodIndex;
            cfg.sensor.lodIndex  = lodConvertForGlassMode(before, on);
            if (cfg.sensor.lodIndex != before) {
                std::printf("note: glass mode %s changes the lift-off scale; "
                            "0x%02X -> 0x%02X (%s)\n",
                            on ? "on" : "off", before, cfg.sensor.lodIndex,
                            lodText(dev, cfg.sensor.lodIndex, on).c_str());
            }
            // Write it even when the byte did not move: the scale changed, so
            // restating it keeps the pair consistent on the device.
            alsoWriteSensor = true;
        }
        cfg.power.glassMode = on;
    } else if (key == "max-sensor-fps") {
        requireIdentifiedModel(dev, key);
        if (!dev.model().hasForceMaxFps) {
            die(std::string("force max sensor FPS is not available on ")
                + dev.model().name);
        }
        cfg.power.setFlag(kForceMaxSensorFps, parseBool(args[2]));
    } else if (key == "motion-jitter") {
        requireIdentifiedModel(dev, key);
        if (!dev.model().hasMotionJitter) {
            die(std::string("motion jitter filter is not available on ")
                + dev.model().name);
        }
        cfg.power.setFlag(kMotionJitterFilter, parseBool(args[2]));
    } else if (key == "slamclick") {
        cfg.power.setFlag(kSlamclickFilter, parseBool(args[2]));
    } else if (key == "multiclick") {
        requireIdentifiedModel(dev, key);
        if (!dev.model().hasMulticlickAck) {
            die(std::string("the multiclick filter toggle is not available on ")
                + dev.model().name);
        }
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

    // Same order the GUI uses, so both front-ends leave the device in the same
    // state if one of the two writes fails.
    writePower(dev, cfg.power);
    if (alsoWriteSensor) {
        writeSensor(dev, cfg.sensor);
    }
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
        requireCpiInRange(dev, x, y);
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
