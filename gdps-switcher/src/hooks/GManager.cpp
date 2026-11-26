#include <asp/fs.hpp>
#include "GManager.hpp"
#include "../utils/GDPSMain.hpp"

#ifdef GEODE_IS_IOS
    static constexpr const char* const gdpsesPath = "save/gdpses";
#else
    static constexpr const char* const gdpsesPath = "gdpses";
#endif

std::vector<GSGManager*> GSGManager::Fields::m_managers = {};

void GSGManager::setup() {
    if (getUserObject("DONT_CHANGE_DIR"_spr)) return GManager::setup();
    const auto main = GDPSMain::get();
    if (!main->isActive()) return GManager::setup();
    if (m_fields->m_originalFileName != "") return GManager::setup();
    
    m_fields->m_originalFileName = m_fileName;
    GSGManager::Fields::m_managers.push_back(this);

    auto server = main->m_servers[main->m_currentServer];
    const auto dir = geode::dirs::getSaveDir() / "gdpses" / server->saveDir;

    if (!dir.exists()) {
        if (auto res = asp::fs::createDir(dir); !res) {
            auto err = res.unwrapErr();
            main->registerIssue(fmt::format("Failed to setup save file: {}", err));
            return log::error("Failed to create directory '{}', data will not save: {}", dir, err);
        }
    }
    m_fileName = fmt::format("{}/{}/{}", gdpsesPath, server->saveDir, m_fileName);
    GManager::setup();
}

// GD 2.2091 changed this to `bool save(bool async)`; the result says whether a save happened
bool GSGManager::save(bool async) {
    if (GDPSMain::get()->shouldSaveGameData()) return GManager::save(async);
    return false;
}

void GSGManager::updateFileNames() {
    log::debug("Updating filenames");
    for (auto manager : Fields::m_managers) {
        if (!manager) continue;
        const auto main = GDPSMain::get();
        auto res = main->getCurrentServer();
        if (!res) log::error("{}", res.unwrapErr());
        auto server = res.unwrap(); // We probably should just crash if the result is Err.
        const auto dir = geode::dirs::getSaveDir() / "gdpses" / server->saveDir;
        if (!dir.exists()) {
            if (auto res = asp::fs::createDir(dir); !res) {
                auto err = res.unwrapErr();
                main->registerIssue(fmt::format("Failed to create directory: {}", err));
                log::error("Failed to create directory '{}', data will not save: {}", dir, err);
                continue;
            }
        }
        if (main->isActive()) {
            manager->m_fileName = fmt::format("{}/{}/{}", gdpsesPath, server->saveDir, manager->m_fields->m_originalFileName);
        }
    }
}
