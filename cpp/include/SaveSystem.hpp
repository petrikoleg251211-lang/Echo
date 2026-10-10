#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace echo {

struct SaveData {
    int checkpointRoomId{0};
    float playerPosX{145.0f};
    float playerPosY{400.0f};
    float playerStamina{100.0f};
    int decoyCount{4};
    float masterVolume{80.0f};
    float rayDensityScale{1.0f};
    int maxReflections{3};
    float ambientVisibility{0.03f};
    float runTimeSec{0.0f};
    std::vector<int> activatedSwitches;
    std::vector<int> collectedLoreIds;
    std::vector<int> visitedRooms;
};

class SaveSystem {
public:
    static std::string resolveSaveFilePath(const std::string& exeDir, const std::string& filename = "echolabyrinth_save.dat");
    static bool hasSaveFile(const std::string& filePath);
    static bool saveToFile(const std::string& filePath, const SaveData& data);
    static bool loadFromFile(const std::string& filePath, SaveData& outData);
    static std::uint32_t computeChecksum(const SaveData& data) noexcept;
};

} // namespace echo
