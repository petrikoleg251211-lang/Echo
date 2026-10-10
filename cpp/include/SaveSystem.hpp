#pragma once

#include <string>
#include <vector>

namespace echo {

struct SaveData {
    int checkpointRoomId{0};
    int decoyCount{4};
    float masterVolume{80.0f};
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
};

} // namespace echo
