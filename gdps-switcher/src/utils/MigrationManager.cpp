#include <asp/fs.hpp>
#include "MigrationManager.hpp"
#include "GDPSMain.hpp"

#include <km7dev.server_api/ServerAPIEvents.hpp>

Result<> MigrationManager::setup() {
    auto savePath = dirs::getSaveDir() / "gdpses";

    if (!savePath.exists()) {
        if (auto res = asp::fs::createDir(savePath); !res) {
            return Err(fmt::format("Failed to create directory '{}': {}", savePath, res.unwrapErr()));
        }
    } else if (!savePath.isDir()) {
        return Err(fmt::format("'{}' exists but is not a directory.", savePath));
    }

    if (Mod::get()->getSavedValue<bool>("1.4.0-migration") != true) {
        migrateData();
    }

    return Ok();
}

std::string MigrationManager::urlToFilenameSafe(std::string_view url) {
    std::string ret = "";
    ret.reserve(url.size());
    for (char c : url) {
        if (std::isalnum(c) || c == '.' || c == '-' || c == '_') {
            ret += c;
        } else {
            ret += '_';
        }
    }
    return ret;
}

void MigrationManager::migrateData() {
    auto savePath = dirs::getSaveDir();

    auto main = GDPSMain::get();
    auto oldSel = Mod::get()->getSavedValue<std::string>("server", "https://www.boomlings.com/database/");

    for (auto& old : Mod::get()->getSavedValue<std::vector<GDPSTypes::OldServer>>("saved-servers")) {
        auto serv = fromOldServer(old);
        int id = serv.id;
        main->m_servers[id] = std::make_shared<GDPSTypes::Server>(std::move(serv));
        if (oldSel == main->m_servers[id]->url) {
            main->m_currentServer = id;
            ServerAPIEvents::updateServer(main->m_serverApiId, main->m_servers[id]->url);
            Mod::get()->setSavedValue<int>("current", id);
        }
    }

    
    GDPSMain::get()->save();

    auto gdpsPath = savePath / "gdpses";

    auto dirIter = asp::fs::iterDir(savePath);
    if (!dirIter) {
        log::error("Failed to read save directory '{}': {}", savePath, dirIter.unwrapErr());
        return;
    }
    for (auto entryRes : std::move(dirIter).unwrap()) {
        if (!entryRes) continue;
        auto entry = std::move(entryRes).unwrap();
        if (!entry.isFile()) continue;

        auto fname = std::string(entry.path.filename().value_or(asp::fs::PathView("")).string());
        if ((!fname.starts_with("CCGameManager-") && !fname.starts_with("CCLocalLevels-")) || !fname.ends_with(".dat")) {
            continue;
        }

        auto start = fname.find('-') + 1;
        auto end = fname.rfind(".dat");
        if (start == std::string::npos || end == std::string::npos || start >= end) continue;

        auto name = fname.substr(start, end - start);
        auto isBackup = name.ends_with("2");
        if (isBackup) {
            name.erase(name.length() - 1);
        }

        auto dir = gdpsPath / name;
        if (!dir.exists()) {
            if (auto res = asp::fs::createDir(dir); !res) {
                log::error("Failed to create directory '{}': {}", dir, res.unwrapErr());
                continue;
            }
        } else if (!dir.isDir()) {
            log::error("'{}' exists but is not a directory.", dir);
            continue;
        }

        auto newName = fname.substr(0, start - 1) + (isBackup ? "2" : "") + fname.substr(end);
        auto newPath = dir / newName;

        if (auto res = asp::fs::rename(savePath / fname, newPath); !res) {
            log::error("Failed to move file '{}' to '{}': {}", fname, newPath, res.unwrapErr());
        }
    }
    Mod::get()->setSavedValue<bool>("1.4.0-migration", true);
}

GDPSTypes::Server MigrationManager::fromOldServer(GDPSTypes::OldServer server) {
    GDPSTypes::Server ret = GDPSTypes::Server();
    int i = 0;
    for (auto &[id, server] : GDPSMain::get()->m_servers) {
        if (id == i) {
            i++;
        }
    }
    ret.id = i;
    ret.name = server.name;
    ret.url = server.url;
    ret.saveDir = urlToFilenameSafe(server.url);
    return ret;
}

$on_mod(Loaded) {
    auto sm = std::make_unique<MigrationManager>();
    auto a = sm->setup();
}
