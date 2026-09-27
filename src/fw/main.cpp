// egg-fw — firmware updater for Endgame Gear wireless mice, over USB-C.
//
// Separate from egg-cli on purpose. This writes firmware; it has no business
// sharing a binary, a device-selection strategy or a set of command verbs with
// the tool that changes CPI. See re/firmware/FIRMWARE.md.
//
// It takes the vendor's own updater .exe, downloaded from endgamegear.com, and
// does the rest. There is no separate extraction step: the firmware lives
// inside that executable as a PE resource, and picking the right one is not
// something a user should be asked to do by hand — one updater ships five
// candidate images of identical size and flashes the third.

#if __has_include(<hidapi.h>)
#  include <hidapi.h>
#else
#  include <hidapi/hidapi.h>
#endif

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "flasher.h"
#include "pe.h"

using namespace fw;

namespace {

void usage()
{
    std::cout <<
        "egg-fw — firmware updater for Endgame Gear wireless mice\n"
        "\n"
        "usage:\n"
        "  egg-fw info                       what is attached, and in which mode\n"
        "  egg-fw verify  <updater.exe>      inspect it; touches no hardware\n"
        "  egg-fw flash   <updater.exe>      write it  (add --yes to do it for real)\n"
        "  egg-fw extract <updater.exe> -o <file.bin>\n"
        "                                    save the firmware image by itself\n"
        "\n"
        "what to do:\n"
        "  1. Download the firmware updater for your mouse from endgamegear.com.\n"
        "     Give egg-fw that .exe as it is — do not try to unpack it.\n"
        "  2. Switch the mouse off, unplug the USB-C cable from the dongle and\n"
        "     plug it into the MOUSE, then switch the mouse on. The firmware is\n"
        "     only updatable over the cable; this tool will not touch the dongle.\n"
        "  3. egg-fw flash Endgame_Gear_..._Firmware_Updater_v1.10.exe\n"
        "     That is a rehearsal and writes nothing. If it is happy, run it\n"
        "     again with --yes.\n"
        "\n"
        "options:\n"
        "  --yes            actually write. Without it nothing reaches the mouse\n"
        "  --model <name>   assert which mouse this is for. Normally unnecessary:\n"
        "                   the updater names its own model and egg-fw checks it\n"
        "                   against the mouse that is plugged in\n"
        "  --keep-config    skip the closing factory reset. The vendor's updater\n"
        "                   always resets; skipping leaves settings the new\n"
        "                   firmware may interpret differently\n"
        "\n"
        "models:\n";
    for (const Target& t : kTargets) {
        std::printf("  %-10s %-12s  %s\n", t.key, t.name,
                    t.verified ? "flashing verified on hardware"
                               : "product IDs known, flashing UNTESTED");
    }
    std::cout <<
        "\n"
        "If an update is interrupted the mouse stays in its bootloader, which is\n"
        "recoverable, not broken: run egg-fw again and it flashes from the start.\n";
}

int die(const std::string& msg)
{
    std::cerr << "error: " << msg << "\n";
    return 1;
}

std::vector<uint8_t> readFile(const std::string& path, std::string* err)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) { *err = "cannot open " + path; return {}; }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
}

// What a file turned out to be, and what it says about itself.
struct Loaded {
    std::vector<uint8_t> image;
    bool          fromExe = false;
    uint32_t      resourceId = 0;
    size_t        fileOffset = 0;
    const Target* declared = nullptr;   // the model the .exe names, if any
};

// The updater names its model in its VERSIONINFO product string. "OP1w 4k" is
// a prefix of "OP1w 4k v2", so the longest match wins — matching the short one
// first would call every v2 updater a v1.
const Target* declaredModel(const std::vector<uint8_t>& exe)
{
    const Target* best = nullptr;
    size_t bestLen = 0;
    for (const Target& t : kTargets) {
        const size_t len = std::strlen(t.name);
        if (len > bestLen && containsWideString(exe, t.name)) {
            best = &t; bestLen = len;
        }
    }
    return best;
}

bool load(const std::string& path, Loaded* out, std::string* err)
{
    std::vector<uint8_t> raw = readFile(path, err);
    if (!err->empty()) return false;

    if (!looksLikePe(raw)) {
        // A bare image. Nothing in it says which mouse it belongs to.
        out->image = std::move(raw);
        return true;
    }

    const ExtractResult ex = extractFirmware(raw);
    if (!ex.ok) { *err = ex.why; return false; }
    out->image      = ex.image;
    out->fromExe    = true;
    out->resourceId = ex.resourceId;
    out->fileOffset = ex.fileOffset;
    out->declared   = declaredModel(raw);
    return true;
}

void describe(const std::string& path, const Loaded& l, const ImageCheck& c)
{
    std::printf("File     : %s\n", path.c_str());
    if (l.fromExe) {
        std::printf("Source   : firmware resource FWFILE/%u inside the updater, "
                    "at offset 0x%zX\n", l.resourceId, l.fileOffset);
        std::printf("Declares : %s\n", l.declared ? l.declared->name
                                                  : "(the updater does not name a model)");
    } else {
        std::printf("Source   : raw image file (not a vendor updater)\n");
    }
    std::printf("Image    : %zu bytes, %zu blocks, entropy %.4f\n",
                l.image.size(), c.blocks, c.entropy);
    std::printf("sha256   : %s\n", c.sha256.c_str());
}

int cmdInfo()
{
    bool inBootloader = false;
    const Target* t = Flasher::detect(&inBootloader);
    if (!t) {
        std::cout << "No mouse found.\n\n"
                     "egg-fw talks to a cabled mouse or its bootloader, never to "
                     "the dongle.\nSwitch the mouse off, move the USB-C cable "
                     "from the dongle to the mouse,\nand switch it back on.\n";
        return 1;
    }
    std::printf("Model       : %s%s\n", t->name,
                t->verified ? "" : "   (flashing UNTESTED on this model)");
    std::printf("Mode        : %s\n",
                inBootloader ? "bootloader — ready to flash"
                             : "running its firmware");
    std::printf("Product IDs : application %04X, bootloader %04X\n",
                t->appPid, t->bldrPid);
    if (inBootloader) {
        std::cout << "\nThe mouse is in its bootloader, which is where an "
                     "interrupted update leaves\nit. That is recoverable: run "
                     "egg-fw flash and it writes from the start.\n";
    }
    return 0;
}

int cmdVerify(const std::string& path)
{
    Loaded l; std::string err;
    if (!load(path, &l, &err)) return die(err);

    const ImageCheck c = inspectImage(l.image);
    describe(path, l, c);
    if (!c.ok) { std::printf("Verdict  : REJECTED — %s\n", c.why.c_str()); return 1; }
    std::printf("Blocks   : indices 0x%04X..0x%04X\n", kFirstBlockIndex,
                unsigned(kFirstBlockIndex + c.blocks - 1));
    std::printf("Verdict  : plausible firmware image\n");
    return 0;
}

int cmdExtract(const std::vector<std::string>& args)
{
    std::string path, out;
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "-o") {
            if (++i >= args.size()) return die("-o needs a filename");
            out = args[i];
        } else if (path.empty()) path = args[i];
        else return die("unexpected argument " + args[i]);
    }
    if (path.empty()) return die("extract needs an updater .exe");
    if (out.empty())  return die("extract needs -o <file.bin>");

    Loaded l; std::string err;
    if (!load(path, &l, &err)) return die(err);
    if (!l.fromExe) return die("that file is not a vendor updater executable");

    std::ofstream f(out, std::ios::binary);
    if (!f) return die("cannot write " + out);
    f.write(reinterpret_cast<const char*>(l.image.data()),
            static_cast<std::streamsize>(l.image.size()));
    if (!f) return die("write failed");

    const ImageCheck c = inspectImage(l.image);
    describe(path, l, c);
    std::printf("Wrote    : %s\n", out.c_str());
    return 0;
}

int cmdFlash(std::vector<std::string> args)
{
    Options opt;
    opt.dryRun = true;
    std::string model, path;

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        if (a == "--yes")              opt.dryRun = false;
        else if (a == "--keep-config") opt.factoryReset = false;
        else if (a == "--verbose")     opt.verbose = true;
        else if (a == "--model") {
            if (++i >= args.size()) return die("--model needs a value");
            if (!model.empty())     return die("--model given twice");
            model = args[i];
        } else if (a.rfind("--", 0) == 0) {
            return die("unknown option " + a);
        } else if (path.empty()) {
            path = a;
        } else {
            return die("unexpected argument " + a);
        }
    }
    if (path.empty()) return die("no updater .exe given — see egg-fw with no arguments");

    Loaded l; std::string err;
    if (!load(path, &l, &err)) return die(err);

    const ImageCheck c = inspectImage(l.image);
    if (!c.ok) return die(c.why);

    // Resolve which mouse this image is for, preferring what the updater says
    // about itself over what the user typed.
    const Target* asked = nullptr;
    if (!model.empty()) {
        for (const Target& t : kTargets) if (model == t.key) asked = &t;
        if (!asked) return die("unknown model " + model);
    }
    if (l.declared && asked && l.declared != asked) {
        return die(std::string("the updater is for the ") + l.declared->name +
                   " but --model says " + asked->name);
    }
    opt.target = l.declared ? l.declared : asked;
    if (!opt.target) {
        return die("cannot tell which mouse this image is for. Pass the vendor's "
                   "updater .exe, which names its model, or say --model "
                   "explicitly — the firmware image itself is encrypted and "
                   "identifies nothing");
    }

    describe(path, l, c);
    std::printf("Target   : %s\n", opt.target->name);
    if (!opt.target->verified) {
        std::cout << "\n*** Flashing has never been tested on this model. Its "
                     "product IDs are known\n*** but the sequence is assumed. "
                     "Think before continuing.\n";
    }
    if (opt.dryRun) {
        std::cout << "\nREHEARSAL — nothing will be written. Add --yes to do it "
                     "for real.\n";
    } else if (opt.factoryReset) {
        std::cout << "\nThe update ends with a factory reset, exactly as the "
                     "vendor's updater does:\nthe mouse loses its settings. To "
                     "keep a copy, plug the dongle back in first\nand run "
                     "`egg-cli blob`.\n";
    }
    std::cout << "\n";

    Flasher f(opt);
    f.log = [](const std::string& s) { std::cout << "  " << s << "\n"; };
    // Throttled: a carriage return is right on a terminal and unreadable in a
    // log, and 205 lines of it buries anything that matters.
    f.progress = [&](size_t done, size_t total) {
        const size_t every = total / 20 ? total / 20 : 1;
        if (done != total && done % every) return;
        std::printf("\r  block %zu / %zu", done, total);
        if (done == total) std::printf("\n");
        std::fflush(stdout);
    };

    if (!f.flash(l.image)) return die(f.lastError());

    std::cout << (opt.dryRun
                     ? "Rehearsal finished. Nothing was written.\n"
                     : "Firmware written. Unplug the cable, put the dongle back "
                       "in, and check the\nversion with `egg-cli info`.\n");
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) { usage(); return 1; }

    if (hid_init() != 0) return die("hid_init failed");
    struct HidGuard { ~HidGuard() { hid_exit(); } } guard;

    const std::string cmd = args[0];
    args.erase(args.begin());

    if (cmd == "info")    return cmdInfo();
    if (cmd == "verify")  { if (args.empty()) return die("verify needs a file"); return cmdVerify(args[0]); }
    if (cmd == "extract") return cmdExtract(args);
    if (cmd == "flash")   return cmdFlash(args);

    usage();
    return 1;
}
