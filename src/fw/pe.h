// Pulling the firmware image out of a vendor updater .exe, on Linux, with no
// Windows and no Python.
//
// A port of re/tools/extract_fw.py. The two are kept deliberately equivalent
// and are tested against each other on every updater we have — see
// re/firmware/FIRMWARE.md section 2. If you change the selection rule here,
// change it there, and re-run the comparison.
//
// The reason this is not simply "take the FWFILE resource": one updater ships
// FIVE candidate images of identical size and flashes the third. Picking by
// lowest id, highest id, or size all choose wrong. The id has to come from the
// instruction that passes it to FindResourceW.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fw {

struct ExtractResult {
    bool        ok = false;
    std::string why;               // why not, when !ok

    std::vector<uint8_t> image;
    uint32_t    resourceId = 0;
    uint32_t    language   = 0;
    uint32_t    rva        = 0;
    size_t      fileOffset = 0;

    // Everything the selection saw, so a refusal can explain itself.
    std::vector<uint32_t> candidateIds;
    std::vector<uint32_t> idsUsedInCode;
};

// True when the buffer looks like a PE executable rather than a raw image.
bool looksLikePe(const std::vector<uint8_t>& data);

// True when the executable contains `text` as a UTF-16LE string. Used to read
// the model out of the updater's own VERSIONINFO product name, which is the
// only place either file says which mouse it is for -- the firmware image
// itself is encrypted and self-describes nothing. Callers must prefer the
// LONGEST matching model name: "OP1w 4k" is a prefix of "OP1w 4k v2".
bool containsWideString(const std::vector<uint8_t>& exe, const char* text);

// Extract the firmware image a vendor updater actually flashes. Refuses, rather
// than guessing, whenever the code scan cannot pin exactly one resource id.
ExtractResult extractFirmware(const std::vector<uint8_t>& exe);

}  // namespace fw
