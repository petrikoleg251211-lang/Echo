#include "SaveSystem.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace echo {

namespace {
float sanitizeFloat(float val, float minVal, float maxVal, float fallback) noexcept {
    if (!std::isfinite(val)) return fallback;
    return std::clamp(val, minVal, maxVal);
}

void hashMix(std::uint32_t& hash, std::uint32_t val) noexcept {
    hash ^= val;
    hash *= 16777619u;
}
} // namespace

std::uint32_t SaveSystem::computeChecksum(const SaveData& data) noexcept {
    std::uint32_t h = 2166136261u;
    hashMix(h, static_cast<std::uint32_t>(std::clamp(data.checkpointRoomId, 0, 14)));
    hashMix(h, static_cast<std::uint32_t>(std::clamp(data.decoyCount, 0, 99)));
    hashMix(h, static_cast<std::uint32_t>(std::round(sanitizeFloat(data.masterVolume, 0.0f, 100.0f, 80.0f) * 10.0f)));
    hashMix(h, static_cast<std::uint32_t>(std::clamp(data.maxReflections, 1, 6)));
    for (int id : data.activatedSwitches) hashMix(h, static_cast<std::uint32_t>(id));
    for (int id : data.collectedLoreIds) hashMix(h, static_cast<std::uint32_t>(id));
    for (int id : data.visitedRooms) hashMix(h, static_cast<std::uint32_t>(id));
    return h;
}

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

bool SaveSystem::saveToFile(const std::string& filePath, const SaveData& rawData) {
    SaveData data = rawData;
    data.checkpointRoomId = std::clamp(data.checkpointRoomId, 0, 14);
    data.playerPosX = sanitizeFloat(data.playerPosX, 55.0f, 1225.0f, 145.0f);
    data.playerPosY = sanitizeFloat(data.playerPosY, 90.0f, 710.0f, 400.0f);
    data.playerStamina = sanitizeFloat(data.playerStamina, 0.0f, 100.0f, 100.0f);
    data.decoyCount = std::clamp(data.decoyCount, 0, 99);
    data.masterVolume = sanitizeFloat(data.masterVolume, 0.0f, 100.0f, 80.0f);
    data.rayDensityScale = sanitizeFloat(data.rayDensityScale, 0.5f, 2.0f, 1.0f);
    data.maxReflections = std::clamp(data.maxReflections, 1, 6);
    data.ambientVisibility = sanitizeFloat(data.ambientVisibility, 0.0f, 0.25f, 0.03f);
    data.runTimeSec = sanitizeFloat(data.runTimeSec, 0.0f, 999999.0f, 0.0f);

    const std::string tmpPath = filePath + ".tmp";
    {
        std::ofstream out(tmpPath, std::ios::trunc);
        if (!out.is_open()) return false;

        out << "ECHOLABYRINTH_SAVE_V3\n";
        out << "room " << data.checkpointRoomId << "\n";
        out << "pos " << data.playerPosX << " " << data.playerPosY << "\n";
        out << "stamina " << data.playerStamina << "\n";
        out << "decoys " << data.decoyCount << "\n";
        out << "volume " << data.masterVolume << "\n";
        out << "ray_scale " << data.rayDensityScale << "\n";
        out << "bounces " << data.maxReflections << "\n";
        out << "darkness " << data.ambientVisibility << "\n";
        out << "runtime " << data.runTimeSec << "\n";

        out << "switches";
        for (int id : data.activatedSwitches) out << " " << id;
        out << "\n";

        out << "lore";
        for (int id : data.collectedLoreIds) out << " " << id;
        out << "\n";

        out << "visited";
        for (int id : data.visitedRooms) out << " " << id;
        out << "\n";

        out << "checksum " << computeChecksum(data) << "\n";
        out.flush();
        if (!out.good()) {
            out.close();
            std::error_code ec;
            std::filesystem::remove(tmpPath, ec);
            return false;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmpPath, filePath, ec);
    if (ec) {
        std::filesystem::remove(filePath, ec);
        ec.clear();
        std::filesystem::rename(tmpPath, filePath, ec);
        if (ec) {
            return false;
        }
    }
    return true;
}

bool SaveSystem::loadFromFile(const std::string& filePath, SaveData& outData) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    std::string header;
    if (!std::getline(in, header)) return false;
    if (!header.empty() && header.back() == '\r') header.pop_back();

    const bool isV3 = (header == "ECHOLABYRINTH_SAVE_V3");
    const bool isV2 = (header == "ECHOLABYRINTH_SAVE_V2");
    const bool isV1 = (header == "ECHOLABYRINTH_SAVE_V1");
    if (!isV3 && !isV2 && !isV1) {
        return false;
    }

    SaveData parsed{};
    bool hasRoomKey = false;
    bool hasChecksum = false;
    std::uint32_t fileChecksum = 0;
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
        } else if (key == "pos") {
            float px = 145.0f;
            float py = 400.0f;
            if (iss >> px >> py) {
                parsed.playerPosX = sanitizeFloat(px, 55.0f, 1225.0f, 145.0f);
                parsed.playerPosY = sanitizeFloat(py, 90.0f, 710.0f, 400.0f);
            }
        } else if (key == "stamina") {
            float st = 100.0f;
            if (iss >> st) {
                parsed.playerStamina = sanitizeFloat(st, 0.0f, 100.0f, 100.0f);
            }
        } else if (key == "decoys") {
            int d = 4;
            if (iss >> d) {
                parsed.decoyCount = std::clamp(d, 0, 99);
            }
        } else if (key == "volume") {
            float v = 80.0f;
            if (iss >> v) {
                parsed.masterVolume = sanitizeFloat(v, 0.0f, 100.0f, 80.0f);
            }
        } else if (key == "ray_scale") {
            float rs = 1.0f;
            if (iss >> rs) {
                parsed.rayDensityScale = sanitizeFloat(rs, 0.5f, 2.0f, 1.0f);
            }
        } else if (key == "bounces") {
            int b = 3;
            if (iss >> b) {
                parsed.maxReflections = std::clamp(b, 1, 6);
            }
        } else if (key == "darkness") {
            float dk = 0.03f;
            if (iss >> dk) {
                parsed.ambientVisibility = sanitizeFloat(dk, 0.0f, 0.25f, 0.03f);
            }
        } else if (key == "runtime") {
            float rt = 0.0f;
            if (iss >> rt) {
                parsed.runTimeSec = sanitizeFloat(rt, 0.0f, 999999.0f, 0.0f);
            }
        } else if (key == "switches") {
            int val = 0;
            while (iss >> val && parsed.activatedSwitches.size() < 64) {
                if (std::find(parsed.activatedSwitches.begin(), parsed.activatedSwitches.end(), val) == parsed.activatedSwitches.end()) {
                    parsed.activatedSwitches.push_back(val);
                }
            }
        } else if (key == "lore") {
            int val = 0;
            while (iss >> val && parsed.collectedLoreIds.size() < 64) {
                if (std::find(parsed.collectedLoreIds.begin(), parsed.collectedLoreIds.end(), val) == parsed.collectedLoreIds.end()) {
                    parsed.collectedLoreIds.push_back(val);
                }
            }
        } else if (key == "visited") {
            int val = 0;
            while (iss >> val && parsed.visitedRooms.size() < 32) {
                if (val >= 0 && val < 15) {
                    if (std::find(parsed.visitedRooms.begin(), parsed.visitedRooms.end(), val) == parsed.visitedRooms.end()) {
                        parsed.visitedRooms.push_back(val);
                    }
                }
            }
        } else if (key == "checksum") {
            if (iss >> fileChecksum) {
                hasChecksum = true;
            }
        }
    }

    if (!hasRoomKey) return false;
    if (isV3) {
        if (!hasChecksum || computeChecksum(parsed) != fileChecksum) {
            return false;
        }
    }

    if (parsed.visitedRooms.empty()) {
        parsed.visitedRooms.push_back(parsed.checkpointRoomId);
    }

    outData = std::move(parsed);
    return true;
}

} // namespace echo
