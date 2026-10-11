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
    hashMix(h, data.runSeed);
    hashMix(h, static_cast<std::uint32_t>(std::clamp(data.targetRoomCount, 15, 60)));
    hashMix(h, static_cast<std::uint32_t>(std::clamp(data.checkpointRoomId, 0, 59)));
    hashMix(h, static_cast<std::uint32_t>(std::clamp(data.playerHealth, 1, 20)));
    hashMix(h, static_cast<std::uint32_t>(std::clamp(data.playerMaxHealth, 2, 20)));
    hashMix(h, static_cast<std::uint32_t>(std::clamp(data.decoyCount, 0, 99)));
    hashMix(h, static_cast<std::uint32_t>(std::clamp(data.echoShards, 0, 9999)));
    hashMix(h, static_cast<std::uint32_t>(std::round(sanitizeFloat(data.masterVolume, 0.0f, 100.0f, 80.0f) * 10.0f)));
    hashMix(h, static_cast<std::uint32_t>(std::clamp(data.maxReflections, 1, 6)));
    hashMix(h, static_cast<std::uint32_t>(std::clamp(data.totalVictories, 0, 9999)));
    for (int id : data.activatedSwitches) hashMix(h, static_cast<std::uint32_t>(id));
    for (int id : data.collectedLoreIds) hashMix(h, static_cast<std::uint32_t>(id));
    for (int id : data.visitedRooms) hashMix(h, static_cast<std::uint32_t>(id));
    for (int id : data.clearedRooms) hashMix(h, static_cast<std::uint32_t>(id));
    for (int id : data.collectedRelics) hashMix(h, static_cast<std::uint32_t>(id));
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
    if (data.runSeed == 0) data.runSeed = 0xEC402026u;
    data.targetRoomCount = std::clamp(data.targetRoomCount, 15, 60);
    data.checkpointRoomId = std::clamp(data.checkpointRoomId, 0, 59);
    data.playerPosX = sanitizeFloat(data.playerPosX, 55.0f, 1225.0f, 640.0f);
    data.playerPosY = sanitizeFloat(data.playerPosY, 55.0f, 745.0f, 400.0f);
    data.playerMaxHealth = std::clamp(data.playerMaxHealth, 2, 20);
    data.playerHealth = std::clamp(data.playerHealth, 1, data.playerMaxHealth);
    data.playerStamina = sanitizeFloat(data.playerStamina, 0.0f, 100.0f, 100.0f);
    data.decoyCount = std::clamp(data.decoyCount, 0, 99);
    data.echoShards = std::clamp(data.echoShards, 0, 9999);
    data.masterVolume = sanitizeFloat(data.masterVolume, 0.0f, 100.0f, 80.0f);
    data.musicVolume = sanitizeFloat(data.musicVolume, 0.0f, 100.0f, 75.0f);
    data.sfxVolume = sanitizeFloat(data.sfxVolume, 0.0f, 100.0f, 85.0f);
    data.ambientVolume = sanitizeFloat(data.ambientVolume, 0.0f, 100.0f, 80.0f);
    data.rayDensityScale = sanitizeFloat(data.rayDensityScale, 0.5f, 2.0f, 1.0f);
    data.maxReflections = std::clamp(data.maxReflections, 1, 6);
    data.ambientVisibility = sanitizeFloat(data.ambientVisibility, 0.0f, 0.25f, 0.03f);
    data.runTimeSec = sanitizeFloat(data.runTimeSec, 0.0f, 999999.0f, 0.0f);
    data.totalRuns = std::clamp(data.totalRuns, 1, 99999);
    data.totalVictories = std::clamp(data.totalVictories, 0, 9999);
    data.bossesDefeated = std::clamp(data.bossesDefeated, 0, 9999);

    const std::string tmpPath = filePath + ".tmp";
    {
        std::ofstream out(tmpPath, std::ios::trunc);
        if (!out.is_open()) return false;

        out << "ECHOLABYRINTH_SAVE_V4\n";
        out << "seed " << data.runSeed << "\n";
        out << "room_count " << data.targetRoomCount << "\n";
        out << "room " << data.checkpointRoomId << "\n";
        out << "pos " << data.playerPosX << " " << data.playerPosY << "\n";
        out << "health " << data.playerHealth << " " << data.playerMaxHealth << "\n";
        out << "stamina " << data.playerStamina << "\n";
        out << "decoys " << data.decoyCount << "\n";
        out << "shards " << data.echoShards << "\n";
        out << "volume " << data.masterVolume << "\n";
        out << "channels " << data.musicVolume << " " << data.sfxVolume << " " << data.ambientVolume << "\n";
        out << "ray_scale " << data.rayDensityScale << "\n";
        out << "bounces " << data.maxReflections << "\n";
        out << "darkness " << data.ambientVisibility << "\n";
        out << "runtime " << data.runTimeSec << "\n";
        out << "meta " << data.totalRuns << " " << data.totalVictories << " " << data.bossesDefeated << "\n";

        out << "switches";
        for (int id : data.activatedSwitches) out << " " << id;
        out << "\n";

        out << "lore";
        for (int id : data.collectedLoreIds) out << " " << id;
        out << "\n";

        out << "visited";
        for (int id : data.visitedRooms) out << " " << id;
        out << "\n";

        out << "cleared";
        for (int id : data.clearedRooms) out << " " << id;
        out << "\n";

        out << "relics";
        for (int id : data.collectedRelics) out << " " << id;
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

    const bool isV4 = (header == "ECHOLABYRINTH_SAVE_V4");
    const bool isV3 = (header == "ECHOLABYRINTH_SAVE_V3");
    const bool isV2 = (header == "ECHOLABYRINTH_SAVE_V2");
    const bool isV1 = (header == "ECHOLABYRINTH_SAVE_V1");
    if (!isV4 && !isV3 && !isV2 && !isV1) {
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

        if (key == "seed") {
            std::uint32_t s = 0;
            if (iss >> s) parsed.runSeed = (s == 0) ? 0xEC402026u : s;
        } else if (key == "room_count") {
            int rc = 32;
            if (iss >> rc) parsed.targetRoomCount = std::clamp(rc, 15, 60);
        } else if (key == "room") {
            int r = 0;
            if (iss >> r) {
                parsed.checkpointRoomId = std::clamp(r, 0, 59);
                hasRoomKey = true;
            }
        } else if (key == "pos") {
            float px = 640.0f;
            float py = 400.0f;
            if (iss >> px >> py) {
                parsed.playerPosX = sanitizeFloat(px, 55.0f, 1225.0f, 640.0f);
                parsed.playerPosY = sanitizeFloat(py, 55.0f, 745.0f, 400.0f);
            }
        } else if (key == "health") {
            int hp = 6;
            int maxHp = 6;
            if (iss >> hp >> maxHp) {
                parsed.playerMaxHealth = std::clamp(maxHp, 2, 20);
                parsed.playerHealth = std::clamp(hp, 1, parsed.playerMaxHealth);
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
        } else if (key == "shards") {
            int sh = 0;
            if (iss >> sh) {
                parsed.echoShards = std::clamp(sh, 0, 9999);
            }
        } else if (key == "volume") {
            float v = 80.0f;
            if (iss >> v) {
                parsed.masterVolume = sanitizeFloat(v, 0.0f, 100.0f, 80.0f);
            }
        } else if (key == "channels") {
            float mv = 75.0f, sv = 85.0f, av = 80.0f;
            if (iss >> mv >> sv >> av) {
                parsed.musicVolume = sanitizeFloat(mv, 0.0f, 100.0f, 75.0f);
                parsed.sfxVolume = sanitizeFloat(sv, 0.0f, 100.0f, 85.0f);
                parsed.ambientVolume = sanitizeFloat(av, 0.0f, 100.0f, 80.0f);
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
        } else if (key == "meta") {
            int tr = 1, tv = 0, bd = 0;
            if (iss >> tr >> tv >> bd) {
                parsed.totalRuns = std::clamp(tr, 1, 99999);
                parsed.totalVictories = std::clamp(tv, 0, 9999);
                parsed.bossesDefeated = std::clamp(bd, 0, 9999);
            }
        } else if (key == "switches") {
            int val = 0;
            while (iss >> val && parsed.activatedSwitches.size() < 128) {
                if (std::find(parsed.activatedSwitches.begin(), parsed.activatedSwitches.end(), val) == parsed.activatedSwitches.end()) {
                    parsed.activatedSwitches.push_back(val);
                }
            }
        } else if (key == "lore") {
            int val = 0;
            while (iss >> val && parsed.collectedLoreIds.size() < 128) {
                if (std::find(parsed.collectedLoreIds.begin(), parsed.collectedLoreIds.end(), val) == parsed.collectedLoreIds.end()) {
                    parsed.collectedLoreIds.push_back(val);
                }
            }
        } else if (key == "visited") {
            int val = 0;
            while (iss >> val && parsed.visitedRooms.size() < 64) {
                if (val >= 0 && val < 60) {
                    if (std::find(parsed.visitedRooms.begin(), parsed.visitedRooms.end(), val) == parsed.visitedRooms.end()) {
                        parsed.visitedRooms.push_back(val);
                    }
                }
            }
        } else if (key == "cleared") {
            int val = 0;
            while (iss >> val && parsed.clearedRooms.size() < 64) {
                if (val >= 0 && val < 60) {
                    if (std::find(parsed.clearedRooms.begin(), parsed.clearedRooms.end(), val) == parsed.clearedRooms.end()) {
                        parsed.clearedRooms.push_back(val);
                    }
                }
            }
        } else if (key == "relics") {
            int val = 0;
            while (iss >> val && parsed.collectedRelics.size() < 16) {
                if (val >= 0 && val < 10) {
                    if (std::find(parsed.collectedRelics.begin(), parsed.collectedRelics.end(), val) == parsed.collectedRelics.end()) {
                        parsed.collectedRelics.push_back(val);
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
    if (isV4 || isV3) {
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
