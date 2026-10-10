#include "SaveSystem.hpp"
#include <fstream>
#include <sstream>

namespace echo {

bool SaveSystem::saveToFile(const std::string& filePath, const SaveData& data) {
    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open()) return false;

    out << "ECHOLABYRINTH_SAVE_V1\n";
    out << "room " << data.checkpointRoomId << "\n";
    out << "decoys " << data.decoyCount << "\n";

    out << "switches";
    for (int id : data.activatedSwitches) out << " " << id;
    out << "\n";

    out << "lore";
    for (int id : data.collectedLoreIds) out << " " << id;
    out << "\n";

    return out.good();
}

bool SaveSystem::loadFromFile(const std::string& filePath, SaveData& outData) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    std::string header;
    if (!std::getline(in, header) || header != "ECHOLABYRINTH_SAVE_V1") {
        return false;
    }

    SaveData parsed{};
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key)) continue;

        if (key == "room") {
            iss >> parsed.checkpointRoomId;
        } else if (key == "decoys") {
            iss >> parsed.decoyCount;
        } else if (key == "switches") {
            int val = 0;
            while (iss >> val) parsed.activatedSwitches.push_back(val);
        } else if (key == "lore") {
            int val = 0;
            while (iss >> val) parsed.collectedLoreIds.push_back(val);
        }
    }

    outData = std::move(parsed);
    return true;
}

} // namespace echo
