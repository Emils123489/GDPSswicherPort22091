#include "GameUrls.hpp"

#include <Geode/platform/platform.hpp>
#include <Geode/loader/Log.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#if defined(GEODE_IS_WINDOWS)
    #include <Windows.h>
#elif defined(GEODE_IS_ANDROID)
    #include <link.h>
#elif defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
    #include <mach-o/getsect.h>
    #include <mach-o/loader.h>
#endif

#include "GameUrlsScan.hpp"

namespace {
    using Region = std::string_view;

    // Collects the readable data regions of the main GD module.
    std::vector<Region> gameDataRegions() {
        std::vector<Region> regions;
        auto const base = geode::base::get();
        if (!base) return regions;

#if defined(GEODE_IS_WINDOWS)
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER const*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return regions;
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS const*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return regions;
        auto section = IMAGE_FIRST_SECTION(nt);
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
            auto const flags = section->Characteristics;
            if (!(flags & IMAGE_SCN_MEM_READ)) continue;
            if (flags & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_DISCARDABLE)) continue;
            auto size = section->Misc.VirtualSize ? section->Misc.VirtualSize : section->SizeOfRawData;
            regions.emplace_back(reinterpret_cast<char const*>(base + section->VirtualAddress), size);
        }
#elif defined(GEODE_IS_ANDROID)
        struct Ctx {
            std::uintptr_t base;
            std::vector<Region>* out;
        } ctx { base, &regions };
        dl_iterate_phdr([](dl_phdr_info* info, size_t, void* data) -> int {
            auto ctx = static_cast<Ctx*>(data);
            bool isGame = false;
            for (int i = 0; i < info->dlpi_phnum; ++i) {
                auto const& ph = info->dlpi_phdr[i];
                if (ph.p_type != PT_LOAD) continue;
                auto start = info->dlpi_addr + ph.p_vaddr;
                if (ctx->base >= start && ctx->base < start + ph.p_memsz) {
                    isGame = true;
                    break;
                }
            }
            if (!isGame) return 0;
            for (int i = 0; i < info->dlpi_phnum; ++i) {
                auto const& ph = info->dlpi_phdr[i];
                if (ph.p_type != PT_LOAD || !(ph.p_flags & PF_R)) continue;
                // Only file-backed bytes; string literals never live in .bss
                ctx->out->emplace_back(
                    reinterpret_cast<char const*>(info->dlpi_addr + ph.p_vaddr), ph.p_filesz
                );
            }
            return 1;
        }, &ctx);
#elif defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
        auto header = reinterpret_cast<mach_header_64 const*>(base);
        if (header->magic != MH_MAGIC_64) return regions;
        for (auto [seg, sect] : std::array<std::pair<char const*, char const*>, 2> {{
            {"__TEXT", "__cstring"},
            {"__TEXT", "__const"},
        }}) {
            unsigned long size = 0;
            auto data = getsectiondata(header, seg, sect, &size);
            if (data && size) regions.emplace_back(reinterpret_cast<char const*>(data), size);
        }
#endif
        return regions;
    }
}

GameUrls::Result GameUrls::find() {
    Result result;
    auto regions = gameDataRegions();

    for (auto const& region : regions) {
        if (result.base.empty()) {
            if (auto found = scan::findBaseUrl(region)) {
                result.base = std::move(*found);
                result.baseFromBinary = true;
            }
        }
        if (result.secondary.empty()) {
            if (auto found = scan::findSecondaryUrl(region)) {
                result.secondary = std::move(*found);
                result.secondaryFromBinary = true;
            }
        }
        if (!result.base.empty() && !result.secondary.empty()) break;
    }

    if (result.base.empty()) {
        geode::log::warn("Could not find the built-in server URL in the game binary, using the vanilla URL");
        result.base = scan::VANILLA_BASE;
    }
    if (result.secondary.empty()) {
        // A patched executable usually only changes the host, so derive it from the base URL
        if (result.baseFromBinary && result.base.starts_with("https://")) {
            result.secondary = "http://" + result.base.substr(8);
        } else {
            result.secondary = scan::VANILLA_SECONDARY;
        }
    }

    geode::log::info(
        "Built-in server URLs: {} ({}), {} ({})",
        result.base, result.baseFromBinary ? "from binary" : "fallback",
        result.secondary, result.secondaryFromBinary ? "from binary" : "fallback"
    );
    return result;
}
