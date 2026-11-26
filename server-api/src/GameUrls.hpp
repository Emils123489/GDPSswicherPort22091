#pragma once

#include <string>

// Locates the built-in server URLs inside the loaded Geometry Dash binary.
//
// Previous versions of Server API read these strings from hard-coded offsets,
// which had to be re-found by hand for every GD update and every platform.
// This scans the game's read-only data at runtime instead, so it keeps working
// across GD versions and still picks up custom URLs in pre-patched GDPS
// executables (which keep the endpoint names and only replace the host part).
namespace GameUrls {
    struct Result {
        // e.g. "https://www.boomlings.com/database/"
        std::string base;
        // e.g. "http://www.boomlings.com/database/"
        std::string secondary;
        bool baseFromBinary = false;
        bool secondaryFromBinary = false;
    };

    Result find();
}
