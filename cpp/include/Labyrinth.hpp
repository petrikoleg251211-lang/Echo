#pragma once

#include "AcousticSystem.hpp"
#include "Config.hpp"
#include <SFML/System/Vector2.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace echo {

enum class EnemyVariant : std::uint8_t {
    Stalker = 0,     // Agile arachnid acoustic hunter
    Warden = 1,      // Heavy armored juggernaut (frontal resistance, wide tremor)
    Shrieker = 2,    // Crystalline sonic turret that fires ranged dissonant bolts at sounds
    Phantom = 3,     // Cloaked phase hunter; vulnerable when illuminated by reflected echoes
    AbyssalBoss = 4  // The Choir Sovereign: multi-phase acoustic apex boss
};

enum class PropType : std::uint8_t {
    VentTurbine = 0,
    PipeConduit = 1,
    EmergencyBeacon = 2,
    WaterPuddle = 3
};

struct DecoProp {
    PropType type{PropType::VentTurbine};
    sf::Vector2f position;
    sf::Vector2f endPoint; // Used for PipeConduit segments
    float scale{1.0f};
    float rotationDeg{0.0f};
};

struct RoomPortal {
    sf::Vector2f position;
    float radius{28.0f};
    int targetRoomId{0};
    sf::Vector2f targetSpawnPos;
    std::string label;
    int requiredSwitchId{-1}; // -1 = unlocked, >= 0 = requires switch, resonator, combat clear, or secret sonar ping
    CardinalDir direction{CardinalDir::East};
    bool isSecretDoor{false};
    bool isVictoryExit{false};
};

struct InteractiveMechanism {
    int id{0};
    sf::Vector2f position;
    bool activated{false};
    float animProgress{0.0f};
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

struct RelicPedestal {
    RelicId relicId{RelicId::FocusedTransducer};
    sf::Vector2f position;
    int costShards{0}; // 0 = free pedestal reward, > 0 = Shop price in Echo Shards
    bool collected{false};
};

struct ShopOffer {
    int id{0};
    sf::Vector2f position;
    std::string label;
    int costShards{15};
    int healAmount{0};
    int decoyAmount{0};
    bool purchased{false};
};

struct EnemySpawnSpec {
    EnemyVariant variant{EnemyVariant::Stalker};
    std::vector<sf::Vector2f> waypoints;
    int waveIndex{0}; // 0 = initial wave, 1 = reinforcement wave
};

struct AmbientEmitter {
    sf::Vector2f position;
    float intervalSec{3.2f};
    float timer{0.0f};
    bool isSteamVent{false};
};

struct AcousticHazardZone {
    sf::Vector2f position;
    float radius{44.0f};
    std::string label{"BRITTLE SHALE"};
};

struct RoomDefinition {
    int id{0};
    int gridX{0};
    int gridY{0};
    int distFromStart{0};
    RoomType type{RoomType::Combat};
    std::array<int, 4> neighbors{{-1, -1, -1, -1}}; // North, East, South, West room IDs

    std::string name;
    std::string sectorTitle;
    std::string objectiveText;
    SurfaceMaterial floorMaterial{SurfaceMaterial::Stone};
    EnemyVariant enemyVariant{EnemyVariant::Stalker};
    sf::Vector2f playerDefaultSpawn{640.0f, 400.0f};

    std::vector<WallSegment> walls;
    std::vector<RoomPortal> portals;
    std::vector<InteractiveMechanism> mechanisms;
    std::vector<AcousticResonator> resonators;
    std::vector<AcousticHazardZone> hazards;
    std::vector<LoreCollectible> collectibles;
    std::vector<RelicPedestal> relicPedestals;
    std::vector<ShopOffer> shopOffers;
    std::vector<AmbientEmitter> ambientEmitters;
    std::vector<DecoProp> props;
    std::vector<sf::Vector2f> enemyPatrolNodes;
    std::vector<EnemySpawnSpec> enemySpawns;

    int totalWaves{1};
    int shardReward{0};
    bool visited{false};
    bool discovered{false};
    bool cleared{true};
    bool combatLocked{false};
    bool sanctuaryUsed{false};
    bool isCheckpoint{false};
    bool isExitRoom{false};
};

class Labyrinth {
public:
    Labyrinth();

    // Seed-deterministic procedural roguelike run generator (configurable 15..60 rooms)
    void generateRun(
        std::uint32_t seed,
        std::size_t targetRoomCount = cfg::kDefaultRunRoomCount
    );

    void buildAllRooms();
    void resetAllProgress();
    void updateAnimations(float dt) noexcept;

    std::uint32_t runSeed() const noexcept { return m_runSeed; }
    std::size_t targetRoomCount() const noexcept { return m_targetRoomCount; }

    const RoomDefinition& currentRoom() const noexcept { return m_rooms[m_currentRoomIndex]; }
    RoomDefinition& currentRoomMut() noexcept { return m_rooms[m_currentRoomIndex]; }

    const RoomDefinition& roomAt(std::size_t idx) const noexcept { return m_rooms[idx]; }
    RoomDefinition& roomAtMut(std::size_t idx) noexcept { return m_rooms[idx]; }

    int currentRoomId() const noexcept { return static_cast<int>(m_currentRoomIndex); }
    std::size_t roomCount() const noexcept { return m_rooms.size(); }

    int bossRoomId() const noexcept { return m_bossRoomId; }
    int exitRoomId() const noexcept { return m_exitRoomId; }

    int findRoomAtGrid(int gx, int gy) const noexcept;
    static sf::Vector2f spawnPositionForEntry(CardinalDir enteredFromDir) noexcept;
    static const char* roomTypeName(RoomType type) noexcept;
    static char roomTypeIcon(RoomType type) noexcept;

    void setCurrentRoom(int roomId) noexcept;
    void setCombatLock(int roomId, bool locked) noexcept;
    void markRoomCleared(int roomId) noexcept;
    bool isRoomCleared(int roomId) const noexcept;

    void activateSwitch(int switchId) noexcept;
    bool isSwitchActivated(int switchId) const noexcept;

    void markLoreCollected(int loreId) noexcept;
    bool isLoreCollected(int loreId) const noexcept;

    void markRoomVisited(int roomId) noexcept;
    bool isRoomVisited(int roomId) const noexcept;

    void revealAllRoomsOnMap() noexcept;
    void revealSecretRoomsOnMap() noexcept;

    void restoreState(
        const std::vector<int>& switches,
        const std::vector<int>& loreIds,
        const std::vector<int>& visitedRooms,
        const std::vector<int>& clearedRooms = {}
    );

    bool verifyRoomTraversability(int roomId, std::string& outError) const;
    bool verifyRunGraph(std::string& outError) const;

    const std::vector<int>& activatedSwitches() const noexcept { return m_activatedSwitches; }
    const std::vector<int>& collectedLoreIds() const noexcept { return m_collectedLoreIds; }
    const std::vector<int>& visitedRooms() const noexcept { return m_visitedRooms; }
    const std::vector<int>& clearedRooms() const noexcept { return m_clearedRooms; }

private:
    std::uint32_t m_runSeed{0xEC402026u};
    std::size_t m_targetRoomCount{cfg::kDefaultRunRoomCount};
    int m_bossRoomId{0};
    int m_exitRoomId{0};

    std::vector<RoomDefinition> m_rooms;
    std::size_t m_currentRoomIndex{0};
    std::vector<int> m_activatedSwitches;
    std::vector<int> m_collectedLoreIds;
    std::vector<int> m_visitedRooms;
    std::vector<int> m_clearedRooms;
};

} // namespace echo
