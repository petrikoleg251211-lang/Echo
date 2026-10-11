#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace echo {

struct SaveData {
    // Procedural Roguelike Run State
    std::uint32_t runSeed{0xEC402026u};
    int targetRoomCount{32};
    int checkpointRoomId{0};
    float playerPosX{640.0f};
    float playerPosY{400.0f};
    int playerHealth{6};
    int playerMaxHealth{6};
    float playerStamina{100.0f};
    int decoyCount{4};
    int echoTrapCount{3};
    int activeFrequency{1};
    int echoShards{0};
    int ghostClearsThisRun{0};

    // Audio & Graphics Settings
    float masterVolume{80.0f};
    float musicVolume{75.0f};
    float sfxVolume{85.0f};
    float ambientVolume{80.0f};
    float rayDensityScale{1.0f};
    int maxReflections{3};
    float ambientVisibility{0.03f};
    float runTimeSec{0.0f};

    // Persistent Meta-Progression Across Runs
    int totalRuns{1};
    int totalVictories{0};
    int bossesDefeated{0};
    int metaTotalGhostClears{0};

    // In-Run World & Build Lists + Persistent Audio-Lore Codex
    std::vector<int> activatedSwitches;
    std::vector<int> collectedLoreIds;
    std::vector<int> visitedRooms;
    std::vector<int> clearedRooms;
    std::vector<int> collectedRelics;
    std::vector<int> discoveredCodexIds;
};

class SaveSystem {
public:
    static std::string resolveSaveFilePath(const std::string& exeDir, const std::string& filename = "echolabyrinth_save.dat");
    static bool hasSaveFile(const std::string& filePath);
    static bool saveToFile(const std::string& filePath, const SaveData& data);
    static bool loadFromFile(const std::string& filePath, SaveData& outData);
    static std::uint32_t computeChecksum(const SaveData& data) noexcept;
    static std::uint32_t computeChecksumV4(const SaveData& data) noexcept;
};

} // namespace echo
