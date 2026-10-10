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
    std::string label;
    int requiredSwitchId{-1}; // -1 = unlocked, >= 0 = requires switch or resonator activation
    bool isVictoryExit{false};
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

struct AmbientEmitter {
    sf::Vector2f position;
    float intervalSec{3.2f};
    float timer{0.0f};
    bool isSteamVent{false}; // If true, masks player noise nearby when pulsing
};

struct AcousticHazardZone {
    sf::Vector2f position;
    float radius{44.0f};
    std::string label{"BRITTLE SHALE"};
};

struct RoomDefinition {
    int id{0};
    std::string name;
    std::string sectorTitle;
    std::string objectiveText;
    sf::Vector2f playerDefaultSpawn{145.0f, 400.0f};
    std::vector<WallSegment> walls;
    std::vector<RoomPortal> portals;
    std::vector<InteractiveMechanism> mechanisms;
    std::vector<AcousticResonator> resonators;
    std::vector<AcousticHazardZone> hazards;
    std::vector<LoreCollectible> collectibles;
    std::vector<AmbientEmitter> ambientEmitters;
    std::vector<sf::Vector2f> enemyPatrolNodes;
    bool isCheckpoint{false};
    bool isExitRoom{false};
};

class Labyrinth {
public:
    Labyrinth();

    void buildAllRooms();
    void resetAllProgress();

    const RoomDefinition& currentRoom() const noexcept { return m_rooms[m_currentRoomIndex]; }
    RoomDefinition& currentRoomMut() noexcept { return m_rooms[m_currentRoomIndex]; }

    const RoomDefinition& roomAt(std::size_t idx) const noexcept { return m_rooms[idx]; }
    int currentRoomId() const noexcept { return static_cast<int>(m_currentRoomIndex); }
    std::size_t roomCount() const noexcept { return m_rooms.size(); }

    void setCurrentRoom(int roomId) noexcept;
    void activateSwitch(int switchId) noexcept;
    bool isSwitchActivated(int switchId) const noexcept;

    void markLoreCollected(int loreId) noexcept;
    bool isLoreCollected(int loreId) const noexcept;

    void markRoomVisited(int roomId) noexcept;
    bool isRoomVisited(int roomId) const noexcept;

    void restoreState(
        const std::vector<int>& switches,
        const std::vector<int>& loreIds,
        const std::vector<int>& visitedRooms
    );

    // Mathematically verifies via 2D BFS grid pathfinding (with player collision radius)
    // that all levers, lore logs, and portals in roomId are physically reachable and
    // locked gates cannot be bypassed when closed.
    bool verifyRoomTraversability(int roomId, std::string& outError) const;

    const std::vector<int>& activatedSwitches() const noexcept { return m_activatedSwitches; }
    const std::vector<int>& collectedLoreIds() const noexcept { return m_collectedLoreIds; }
    const std::vector<int>& visitedRooms() const noexcept { return m_visitedRooms; }

private:
    std::vector<RoomDefinition> m_rooms;
    std::size_t m_currentRoomIndex{0};
    std::vector<int> m_activatedSwitches;
    std::vector<int> m_collectedLoreIds;
    std::vector<int> m_visitedRooms;
};

} // namespace echo
