#include "pe.h"

#include <cstdio>
#include <cstring>
#include <set>

namespace fw {
namespace {

uint16_t u16(const std::vector<uint8_t>& d, size_t o)
{
    return static_cast<uint16_t>(d[o] | (d[o + 1] << 8));
}

uint32_t u32(const std::vector<uint8_t>& d, size_t o)
{
    return static_cast<uint32_t>(d[o]) | (static_cast<uint32_t>(d[o + 1]) << 8) |
           (static_cast<uint32_t>(d[o + 2]) << 16) | (static_cast<uint32_t>(d[o + 3]) << 24);
}

bool have(const std::vector<uint8_t>& d, size_t o, size_t n)
{
    return o <= d.size() && n <= d.size() - o;
}

struct Section {
    uint32_t vaddr, vsize, raddr, rsize;
};

struct Pe {
    const std::vector<uint8_t>* d = nullptr;
    uint32_t imagebase = 0;
    uint32_t resRva = 0, resSize = 0;
    std::vector<Section> secs;

    // Both conversions mirror pelib.py exactly, including its bounds rules.
    bool rva2off(uint32_t rva, size_t* out) const
    {
        for (const Section& s : secs) {
            const uint32_t span = s.vsize > s.rsize ? s.vsize : s.rsize;
            if (rva >= s.vaddr && rva < s.vaddr + span) {
                const size_t o = size_t(s.raddr) + (rva - s.vaddr);
                if (o >= d->size()) return false;
                *out = o;
                return true;
            }
        }
        return false;
    }

    bool off2rva(size_t off, uint32_t* out) const
    {
        for (const Section& s : secs) {
            if (s.rsize && off >= s.raddr && off < size_t(s.raddr) + s.rsize) {
                *out = static_cast<uint32_t>(s.vaddr + (off - s.raddr));
                return true;
            }
        }
        return false;
    }
};

bool parsePe(const std::vector<uint8_t>& d, Pe* pe, std::string* why)
{
    if (!have(d, 0, 0x40) || d[0] != 'M' || d[1] != 'Z') { *why = "not a PE file (no MZ)"; return false; }
    const uint32_t e = u32(d, 0x3C);
    if (!have(d, e, 24) || std::memcmp(&d[e], "PE\0\0", 4) != 0) { *why = "not a PE file (no PE header)"; return false; }

    const size_t fh = e + 4;
    const uint16_t nsec = u16(d, fh + 2);
    const uint16_t sizeopt = u16(d, fh + 16);
    const size_t opt = fh + 20;
    if (!have(d, opt, sizeopt)) { *why = "truncated optional header"; return false; }

    const uint16_t magic = u16(d, opt);
    const bool pe32p = (magic == 0x20B);
    if (pe32p) { *why = "64-bit PE; every vendor updater so far is 32-bit"; return false; }

    pe->imagebase = u32(d, opt + 28);
    const uint32_t numdd = u32(d, opt + 0x5C);
    const size_t ddoff = opt + 0x60;
    if (numdd < 3 || !have(d, ddoff, numdd * 8)) { *why = "no resource data directory"; return false; }
    pe->resRva  = u32(d, ddoff + 2 * 8);
    pe->resSize = u32(d, ddoff + 2 * 8 + 4);
    if (!pe->resRva) { *why = "executable has no resources at all"; return false; }

    const size_t so = opt + sizeopt;
    for (uint16_t i = 0; i < nsec; ++i) {
        const size_t b = so + size_t(i) * 40;
        if (!have(d, b, 40)) { *why = "truncated section table"; return false; }
        Section s;
        s.vsize = u32(d, b + 8);
        s.vaddr = u32(d, b + 12);
        s.rsize = u32(d, b + 16);
        s.raddr = u32(d, b + 20);
        pe->secs.push_back(s);
    }
    pe->d = &d;
    return true;
}

struct ResEntry {
    uint32_t id, lang, rva, size;
    size_t   off;
};

// Walk type -> name -> language. We only care about the string-named type
// "FWFILE", so the type level is matched by name and the rest by id.
void walkResources(const Pe& pe, size_t base, size_t off, int level,
                   bool inFwfile, uint32_t id, std::vector<ResEntry>* out,
                   std::set<size_t>* seen)
{
    const std::vector<uint8_t>& d = *pe.d;
    if (seen->count(off) || !have(d, off, 16)) return;
    seen->insert(off);

    const uint16_t nn = u16(d, off + 12);
    const uint16_t ni = u16(d, off + 14);
    for (uint32_t i = 0; i < uint32_t(nn) + ni; ++i) {
        const size_t e = off + 16 + size_t(i) * 8;
        if (!have(d, e, 8)) return;
        const uint32_t nv = u32(d, e);
        const uint32_t ov = u32(d, e + 4);

        bool matchedName = inFwfile;
        uint32_t thisId = id;
        if (nv & 0x80000000u) {
            const size_t no = base + (nv & 0x7FFFFFFFu);
            if (!have(d, no, 2)) continue;
            const uint16_t len = u16(d, no);
            if (level == 0) {
                static const char kName[] = "FWFILE";
                const size_t n = sizeof kName - 1;
                matchedName = (len == n) && have(d, no + 2, n * 2);
                for (size_t c = 0; matchedName && c < n; ++c) {
                    matchedName = (d[no + 2 + c * 2] == uint8_t(kName[c])) &&
                                  (d[no + 2 + c * 2 + 1] == 0);
                }
            }
        } else {
            if (level == 0) matchedName = false;   // FWFILE is string-named
            else thisId = nv;
        }
        if (level == 0 && !matchedName) continue;

        if (ov & 0x80000000u) {
            walkResources(pe, base, base + (ov & 0x7FFFFFFFu), level + 1,
                          matchedName, thisId, out, seen);
        } else {
            const size_t doff = base + ov;
            if (!have(d, doff, 16) || !inFwfile) continue;
            ResEntry r;
            r.rva  = u32(d, doff);
            r.size = u32(d, doff + 4);
            r.lang = thisId;
            r.id   = id;
            size_t fo = 0;
            if (!pe.rva2off(r.rva, &fo)) continue;
            r.off = fo;
            out->push_back(r);
        }
    }
}

// The load-bearing part. Find every place the binary does
//     PUSH <VA of L"FWFILE"> ; PUSH imm32 ; PUSH 0 ; CALL [FindResourceW]
// and read the id out of the second PUSH.
std::vector<uint32_t> idsFromCode(const Pe& pe)
{
    const std::vector<uint8_t>& d = *pe.d;
    static const char kName[] = "FWFILE";
    std::vector<uint8_t> lit;
    for (const char* c = kName; *c; ++c) { lit.push_back(uint8_t(*c)); lit.push_back(0); }
    lit.push_back(0); lit.push_back(0);          // the terminating L'\0'

    // Every VA at which the wide literal appears.
    std::vector<uint32_t> vas;
    for (size_t i = 0; i + lit.size() <= d.size(); ++i) {
        if (std::memcmp(&d[i], lit.data(), lit.size()) != 0) continue;
        uint32_t rva = 0;
        if (pe.off2rva(i, &rva)) vas.push_back(pe.imagebase + rva);
    }

    std::vector<uint32_t> ids;
    for (uint32_t va : vas) {
        uint8_t pat[5] = { 0x68, uint8_t(va), uint8_t(va >> 8), uint8_t(va >> 16), uint8_t(va >> 24) };
        for (size_t j = 0; j + 18 <= d.size(); ++j) {
            if (std::memcmp(&d[j], pat, sizeof pat) != 0) continue;
            if (d[j + 5] == 0x68 && d[j + 10] == 0x6A && d[j + 11] == 0x00 &&
                d[j + 12] == 0xFF && d[j + 13] == 0x15) {
                ids.push_back(u32(d, j + 6));
            }
        }
    }
    return ids;
}

}  // namespace

bool looksLikePe(const std::vector<uint8_t>& data)
{
    return data.size() >= 2 && data[0] == 'M' && data[1] == 'Z';
}

bool containsWideString(const std::vector<uint8_t>& exe, const char* text)
{
    std::vector<uint8_t> w;
    for (const char* c = text; *c; ++c) { w.push_back(uint8_t(*c)); w.push_back(0); }
    if (w.empty() || exe.size() < w.size()) return false;
    for (size_t i = 0; i + w.size() <= exe.size(); ++i) {
        if (std::memcmp(&exe[i], w.data(), w.size()) == 0) return true;
    }
    return false;
}

ExtractResult extractFirmware(const std::vector<uint8_t>& exe)
{
    ExtractResult r;
    Pe pe;
    if (!parsePe(exe, &pe, &r.why)) return r;

    size_t resBase = 0;
    if (!pe.rva2off(pe.resRva, &resBase)) { r.why = "resource directory is not in any section"; return r; }

    std::vector<ResEntry> entries;
    std::set<size_t> seen;
    walkResources(pe, resBase, resBase, 0, false, 0, &entries, &seen);

    for (const ResEntry& e : entries) r.candidateIds.push_back(e.id);
    if (entries.empty()) {
        r.why = "no FWFILE resource in this executable — is it really a firmware "
                "updater, and not the configuration tool?";
        return r;
    }

    const std::vector<uint32_t> used = idsFromCode(pe);
    std::set<uint32_t> distinct(used.begin(), used.end());
    r.idsUsedInCode.assign(distinct.begin(), distinct.end());

    // Refuse rather than guess. This is the whole reason the tool reads the id
    // out of the code: one updater carries five identical-sized candidates.
    if (distinct.size() != 1) {
        char b[320];
        std::snprintf(b, sizeof b,
                      "cannot tell which of the %zu FWFILE resources this updater "
                      "flashes: the code scan found %zu distinct ids. Refusing to "
                      "guess — flashing the wrong image is not recoverable by "
                      "guessing again",
                      entries.size(), distinct.size());
        r.why = b;
        return r;
    }

    const uint32_t want = *distinct.begin();
    const ResEntry* pick = nullptr;
    size_t matches = 0;
    for (const ResEntry& e : entries) {
        if (e.id == want) { pick = &e; ++matches; }
    }
    if (matches != 1) {
        char b[200];
        std::snprintf(b, sizeof b,
                      "the code flashes FWFILE id %u but that id appears %zu times "
                      "in the resource directory", want, matches);
        r.why = b;
        return r;
    }
    if (!have(exe, pick->off, pick->size)) {
        r.why = "the selected resource runs past the end of the file";
        return r;
    }

    r.image.assign(exe.begin() + pick->off, exe.begin() + pick->off + pick->size);
    r.resourceId = pick->id;
    r.language   = pick->lang;
    r.rva        = pick->rva;
    r.fileOffset = pick->off;
    r.ok         = true;
    return r;
}

}  // namespace fw
