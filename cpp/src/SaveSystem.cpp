#include "SaveSystem.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace echo {

std::string SaveSystem::resolveSaveFilePath(const std::string& exeDir, const std::string& filename) {
    if (!exeDir.empty()) {
        std::error_code ec;
        const std::filesystem::path p = std::filesystem::path(exeDir) / filename;
        if (std::filesystem::exists(exeDir, ec)) {
            return p.string();
        }
    }
    return filename;
}

bool SaveSystem::hasSaveFile(const std::string& filePath) {
    SaveData dummy{};
    return loadFromFile(filePath, dummy);
}

bool SaveSystem::saveToFile(const std::string& filePath, const SaveData& data) {
    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open()) return false;

    out << "ECHOLABYRINTH_SAVE_V2\n";
    out << "room " << std::clamp(data.checkpointRoomId, 0, 14) << "\n";
    out << "decoys " << std::clamp(data.decoyCount, 0, 99) << "\n";
    out << "volume " << std::clamp(data.masterVolume, 0.0f, 100.0f) << "\n";

    out << "switches";
    for (int id : data.activatedSwitches) out << " " << id;
    out << "\n";

    out << "lore";
    for (int id : data.collectedLoreIds) out << " " << id;
    out << "\n";

    out << "visited";
    for (int id : data.visitedRooms) out << " " << id;
    out << "\n";

    return out.good();
}

bool SaveSystem::loadFromFile(const std::string& filePath, SaveData& outData) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    std::string header;
    if (!std::getline(in, header)) return false;
    if (!header.empty() && header.back() == '\r') header.pop_back();

    if (header != "ECHOLABYRINTH_SAVE_V2" && header != "ECHOLABYRINTH_SAVE_V1") {
        return false;
    }

    SaveData parsed{};
    bool hasRoomKey = false;
    std::string line;

    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key)) continue;

        if (key == "room") {
            int r = 0;
            if (iss >> r) {
                parsed.checkpointRoomId = std::clamp(r, 0, 14);
                hasRoomKey = true;
            }
        } else if (key == "decoys") {
            int d = 4;
            if (iss >> d) {
                parsed.decoyCount = std::clamp(d, 0, 99);
            }
        } else if (key == "volume") {
            float v = 80.0f;
            if (iss >> v) {
                parsed.masterVolume = std::clamp(v, 0.0f, 100.0f);
            }
        } else if (key == "switches") {
            int val = 0;
            while (iss >> val && parsed.activatedSwitches.size() < 64) {
                parsed.activatedSwitches.push_back(val);
            }
        } else if (key == "lore") {
            int val = 0;
            while (iss >> val && parsed.collectedLoreIds.size() < 64) {
                parsed.collectedLoreIds.push_back(val);
            }
        } else if (key == "visited") {
            int val = 0;
            while (iss >> val && parsed.visitedRooms.size() < 32) {
                if (val >= 0 && val < 15) {
                    parsed.visitedRooms.push_back(val);
                }
            }
        }
    }

    if (!hasRoomKey) return false;
    if (parsed.visitedRooms.empty()) {
        parsed.visitedRooms.push_back(parsed.checkpointRoomId);
    }

    outData = std::move(parsed);
    return true;
}

} // namespace echo
