#pragma once

// Platform-independent string matching used by GameUrls.cpp.
// Kept header-only and free of Geode includes so it can be unit-tested on any host.

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace GameUrls::scan {
    inline constexpr std::string_view VANILLA_BASE = "https://www.boomlings.com/database/";
    inline constexpr std::string_view VANILLA_SECONDARY = "http://www.boomlings.com/database/";

    // Endpoints GD appends to the two URLs we look for. Patched GDPS executables
    // replace the host part but keep these.
    inline constexpr std::string_view BASE_ENDPOINT = "getGJLevelLists.php";
    inline constexpr std::string_view SECONDARY_ENDPOINT = "getGJLevels21.php";

    // base64("http") always starts with this
    inline constexpr std::string_view BASE64_HTTP_PREFIX = "aHR0c";

    inline constexpr std::size_t MAX_URL_LEN = 512;

    inline bool isUrlChar(char c) {
        return c > 0x20 && c < 0x7f;
    }

    inline bool looksLikeServerUrl(std::string_view url) {
        if (!(url.starts_with("http://") || url.starts_with("https://"))) return false;
        if (url.size() <= 8 || url.size() > MAX_URL_LEN) return false;
        for (char c : url) {
            if (!isUrlChar(c)) return false;
        }
        return url.ends_with('/');
    }

    // Finds a NUL-terminated "http(s)://.../getGJLevelLists.php" string and
    // returns everything before the endpoint (with trailing slash).
    // An https URL is preferred, since that is what vanilla GD uses for its main requests.
    inline std::optional<std::string> findBaseUrl(std::string_view data) {
        std::optional<std::string> httpFallback;
        std::size_t pos = 0;
        while ((pos = data.find(BASE_ENDPOINT, pos)) != std::string_view::npos) {
            auto end = pos + BASE_ENDPOINT.size();
            auto const hit = pos;
            pos = end;
            if (end < data.size() && data[end] != '\0') continue;

            auto start = hit;
            while (start > 0 && data[start - 1] != '\0' && hit - start < MAX_URL_LEN) {
                --start;
            }
            auto url = data.substr(start, hit - start);
            if (!looksLikeServerUrl(url)) continue;
            if (url.starts_with("https://")) return std::string(url);
            if (!httpFallback) httpFallback = std::string(url);
        }
        return httpFallback;
    }

    inline int base64Value(char c) {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+' || c == '-') return 62;
        if (c == '/' || c == '_') return 63;
        return -1;
    }

    // Accepts both the standard and URL-safe alphabets, padding optional.
    inline std::optional<std::string> base64Decode(std::string_view in) {
        while (!in.empty() && in.back() == '=') in.remove_suffix(1);
        std::string out;
        out.reserve(in.size() * 3 / 4);
        unsigned int buffer = 0;
        int bits = 0;
        for (char c : in) {
            int v = base64Value(c);
            if (v < 0) return std::nullopt;
            buffer = (buffer << 6) | static_cast<unsigned int>(v);
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                out.push_back(static_cast<char>((buffer >> bits) & 0xff));
            }
        }
        return out;
    }

    // Finds the NUL-terminated base64 string that decodes to
    // "http(s)://.../getGJLevels21.php" and returns the part before the endpoint.
    inline std::optional<std::string> findSecondaryUrl(std::string_view data) {
        std::size_t pos = 0;
        while ((pos = data.find(BASE64_HTTP_PREFIX, pos)) != std::string_view::npos) {
            auto const start = pos;
            pos += BASE64_HTTP_PREFIX.size();
            if (start > 0 && data[start - 1] != '\0') continue;

            auto end = start;
            while (end < data.size() && data[end] != '\0' && end - start < MAX_URL_LEN * 2) {
                ++end;
            }
            if (end < data.size() && data[end] != '\0') continue;

            auto decoded = base64Decode(data.substr(start, end - start));
            if (!decoded || !decoded->ends_with(SECONDARY_ENDPOINT)) continue;
            decoded->resize(decoded->size() - SECONDARY_ENDPOINT.size());
            if (looksLikeServerUrl(*decoded)) {
                return decoded;
            }
        }
        return std::nullopt;
    }
}
