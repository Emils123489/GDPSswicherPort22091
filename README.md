# GDPS Switcher: port to Geometry Dash 2.2091

This is an unofficial port of GDPS Switcher 2.0.0 (`km7dev.gdps-switcher`, by km7dev and Alphii) and its required dependency Server API 4.0.1 (`km7dev.server_api`) from GD 2.2081 / Geode v5.3.0 to **GD 2.2091 / Geode v6.0.0**. Both mods have to be updated, because the switcher won't load without a Server API built for 2.2091.

| Folder | Mod | Old version | New version |
|---|---|---|---|
| `server-api/` | Server API | v4.0.1 (Geode 5.3.0, GD 2.2081) | v4.1.0 (Geode 6.0.0, GD 2.2091) |
| `gdps-switcher/` | GDPS Switcher | 2.0.0 (Geode 5.3.0, GD 2.2081) | 2.1.0 (Geode 6.0.0, GD 2.2091) |

`patches/` contains the same changes as git patches against upstream `AlphiiGD/gdps-switcher@15f4ed9` and `Kingminer7/server-api@7338bba`.

## What changed

### Server API
- **Built-in server URLs are found at runtime.** Server API used to read `https://www.boomlings.com/database/` and its base64 `http://` counterpart from hard-coded per-platform memory offsets in the 2.2081 binary, and it refused to compile for any other GD version. It now scans the game's read-only data for those strings (`src/GameUrls.cpp`, `src/GameUrlsScan.hpp`). This works across GD versions, still picks up custom URLs in pre-patched GDPS executables, and falls back to the vanilla URLs if nothing is found. `PlatformOffsets.hpp` is gone.
- **Event exports** moved from the removed `GEODE_EVENT_EXPORT_NORES` macro to the v6 `GEODE_EVENT_EXPORT(ret, name, params, args)`. Event IDs and call sites are unchanged.
- The `http://` URL rewrite in the `CCHttpClient::send` hook now uses the detected URL's length instead of a hard-coded 34.

### GDPS Switcher
- **`GManager::save`**: GD 2.2091 changed it from `void save()` to `bool save(bool async)`. The hook follows the new signature and returns `false` when it skips a save.
- **File handling**: Geode v6 replaced `std::filesystem` with `asp::fs` (`dirs::getSaveDir()` now returns `asp::fs::Path`, and `utils::string::pathToString` was removed). Save-folder creation, migration, renaming and deletion were moved to `asp::fs`. The deletion path keeps all its safety checks. It now canonicalizes both paths before comparing, which also stops it from refusing every deletion when the save path goes through a symlink (e.g. `/data/user/0` on Android). It also rejects sibling folders that merely share the `gdpses` prefix.
- **UI**: `Popup`'s close button is now a `geode::Button`, so the custom close sprite is set with `setCloseButtonSpr`. `TextInput::getInputNode()->setLabelNormalColor` became `TextInput::setTextColor`.

## Building

### GitHub Actions (easiest)
Push this folder as a new GitHub repository. `.github/workflows/build.yml` builds Server API for all five platforms, makes it available to the switcher build (it isn't on the Geode Index yet), then builds GDPS Switcher. Download the **build-output** artifact from the run. It contains `km7dev.server_api.geode` and `km7dev.gdps-switcher.geode`.

### Locally
You need Geode SDK v6.0.0, Geode CLI 4.x, and Clang 19+ (or MSVC 19.44+).
1. Build Server API: `cd server-api && geode build`
2. Copy the resulting `km7dev.server_api.geode` into your Geode `mods` folder (the CLI checks installed mods when resolving dependencies).
3. Build the switcher: `cd gdps-switcher && geode build`

## Installing
Put **both** `.geode` files in `Geometry Dash/geode/mods/` on a GD 2.2091 install running Geode v6.0.0.

## How it was checked, and what wasn't
- Both mods compile cleanly (syntax and semantic checks) against the Geode v6.0.0 headers and 2.2091 bindings, targeting Android64.
- The URL scanner and the deletion guard have unit tests covering vanilla strings, patched-GDPS strings, http/https ordering and malformed data.
- **Not verified here:** the Windows, macOS and iOS code paths of the URL scanner (they need those platforms' SDKs, so the CI build is the first real compile for them), and running either mod in-game. Before relying on it, check that switching servers and saving work. The first launch logs `Built-in server URLs: … (from binary)`; if it says `fallback`, the scan didn't find the strings in that GD build.

## Publishing
This is an unofficial port for personal use. The Geode Index is the authors' to update. If you want these changes upstream, offer the files in `patches/` to the original authors through their GitHub repositories.
