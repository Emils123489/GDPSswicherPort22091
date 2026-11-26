#include <Geode/Geode.hpp>
#include <Geode/modify/MusicDownloadManager.hpp>
#include <utility>
#include <utils/GDPSMain.hpp>

using namespace geode::prelude;

// GD appends file names directly, so the folder needs a trailing separator
static std::string folderString(asp::fs::Path const& path) {
    auto str = path.string();
    if (!str.ends_with('/') && !str.ends_with('\\')) str += '/';
    return str;
}

class $modify(GDPSMDL, MusicDownloadManager) {

    std::string pathForSFXFolder(int id) {
        if (!GDPSMain::get()->isActive()) return MusicDownloadManager::pathForSFXFolder(id);
        auto server = GDPSMain::get()->m_servers[GDPSMain::get()->m_currentServer];
        return folderString(dirs::getSaveDir() / "gdpses" / server->saveDir);
    }

    std::string pathForSongFolder(int id) {
        if (!GDPSMain::get()->isActive()) return MusicDownloadManager::pathForSongFolder(id);
        auto server = GDPSMain::get()->m_servers[GDPSMain::get()->m_currentServer];
        return folderString(dirs::getSaveDir() / "gdpses" / server->saveDir);
    }
};
