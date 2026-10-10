#pragma once

#include "AcousticSystem.hpp"
#include <SFML/System/Vector2.hpp>
#include <string>
#include <vector>

namespace echo {

struct RoomPortal {
    sf::Vector2f position;
    float radius{28.0f};
    int targetRoomId{0};
    sf::Vector2f targetSpawnPos;
    int requiredSwitchId{-1}; // -1 = unlocked, >= 0 = requires switch activation
};

struct InteractiveMechanism {
    int id{0};
    sf::Vector2f position;
    bool activated{false};
    std::string label;
    int unlocksDoorId{-1};
};

struct LoreCollectible {
    int id{0};
    sf::Vector2f position;
    bool collected{false};
    std::string title;
    std::string excerpt;
};

struct RoomDefinition {
    int id{0};
    std::string name;
    std::string sectorTitle;
    std::string acousticHint;
    sf::Vector2f playerDefaultSpawn{180.0f, 400.0f};
    std::vector<WallSegment> walls;
    std::vector<RoomPortal> portals;
    std::vector<InteractiveMechanism> mechanisms;
    std::vector<LoreCollectible> collectibles;
    std::vector<sf::Vector2f> enemyPatrolNodes;
    bool isCheckpoint{false};
    bool isExitRoom{false};
};

class Labyrinth {
public:
    Labyrinth();

    void buildAllRooms();

    const RoomDefinition& currentRoom() const noexcept { return m_rooms[m_currentRoomIndex]; }
    RoomDefinition& currentRoomMut() noexcept { return m_rooms[m_currentRoomIndex]; }

    int currentRoomId() const noexcept { return static_cast<int>(m_currentRoomIndex); }
    std::size_t roomCount() const noexcept { return m_rooms.size(); }

    void setCurrentRoom(int roomId) noexcept;
    void activateSwitch(int switchId) noexcept;
    bool isSwitchActivated(int switchId) const noexcept;

    void markLoreCollected(int loreId) noexcept;
    const std::vector<int>& collectedLoreIds() const noexcept { return m_collectedLoreIds; }

private:
    std::vector<RoomDefinition> m_rooms;
    std::size_t m_currentRoomIndex{0};
    std::vector<int> m_activatedSwitches;
    std::vector<int> m_collectedLoreIds;
};

} // namespace echo
