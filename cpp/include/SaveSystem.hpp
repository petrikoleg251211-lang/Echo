#pragma once

#include <string>
#include <vector>

namespace echo {

struct SaveData {
    int checkpointRoomId{0};
    int decoyCount{3};
    std::vector<int> activatedSwitches;
    std::vector<int> collectedLoreIds;
};

class SaveSystem {
public:
    static bool saveToFile(const std::string& filePath, const SaveData& data);
    static bool loadFromFile(const std::string& filePath, SaveData& outData);
};

} // namespace echo
