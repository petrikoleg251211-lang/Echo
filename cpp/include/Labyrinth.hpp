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
    Stalker = 0,             // Standard Acoustic Hunter (balanced speed, investigates sounds)
    Warden = 1,              // Heavy Armored Warden (slow, high HP, emits periodic floor tremors)
    Shrieker = 2,            // Fast Sonic Shrieker (high sensitivity, fires ranged SonicBolts)
    Phantom = 3,             // Phase Phantom (fast, takes bonus damage from reflected sonar rays)
    AbyssalBoss = 4,         // The Choir Sovereign (3-phase boss with radial shockwaves & bolt volleys)
    SilentStalker = 5,       // Block 4.1: Emits zero passive tremors while moving; only visible via sonar; inhales 0.5s before attack
    DeafBrute = 6,           // Block 4.2: Deaf to airborne sounds; only reacts to ground vibrations (Sprint, Stomp, RadialPulse, ShaleCrack)
    Mimic = 7,               // Block 4.3: Disguised as resonator crystal emitting lure waves; unmasks on High-freq/Whistle scan or close approach
    EchoSwarm = 8,           // Block 4.4: Acoustic parasite swarm; repeats & amplifies nearby player sounds (+12 noise) to alert other hunters
    Leech = 9,               // Stage 2: Acoustic Parasite; latches onto player draining stamina & pulsing +8 noise until shaken off by Sprint/Pulse/Stomp
    Banshee = 10,            // Stage 2: Wailing Siren; unleashes periodic BansheeWail screams (+22 noise) that alert hunters across the chamber
    TuningForkConstruct = 11,// Stage 2: Resonance Automaton; dampens Low/Mid direct hits, shatters (2.4x dmg) on High-Freq [3] or reflected rays
    TheConductor = 12        // Stage 2: Chamber 16 Mini-Boss; orchestrates 4-beam harmonic cross volleys & frequency-shielded phases
};

enum class PropType : std::uint8_t {
    PipeConduit = 0,
    WaterPuddle = 1,
    VentTurbine = 2,
    EmergencyBeacon = 3
};

struct RoomPortal {
    sf::Vector2f position;
    float radius{28.0f};
    int targetRoomId{0};
    sf::Vector2f targetSpawnPos;
    std::string label;
    int requiredSwitchId{-1}; // -1 = open; 800+roomId = combat lock; 900+roomId = secret wall
    CardinalDir direction{CardinalDir::North};
    bool isSecretDoor{false};
    bool isVictoryExit{false};
};

struct InteractiveMechanism {
    int id{0};
    sf::Vector2f position;
    bool activated{false};
    float animProgress{0.0f};
    std::string label;
    int targetDoorId{-1};
};

struct LoreCollectible {
    int id{0};
    sf::Vector2f position;
    bool collected{false};
    std::string title;
    std::string excerpt;
};

struct RelicPedestal {
    RelicId relicId{RelicId::HarmonicAmplifier};
    sf::Vector2f position;
    int costShards{0};
    bool collected{false};
};

struct ShopOffer {
    int id{0};
    sf::Vector2f position;
    std::string label;
    int costShards{20};
    int healAmount{0};
    int decoyAmount{0};
    bool purchased{false};
};

struct DecoProp {
    PropType type{PropType::PipeConduit};
    sf::Vector2f position;
    sf::Vector2f endPoint;
    float scale{14.0f};
    float rotationDeg{0.0f};
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

// Block 1: Cross-room sound propagation entry (attenuates 50% per open doorway hop)
struct RoomNoiseLeak {
    int roomId{0};
    int gridDistance{1};
    float leakedNoise{0.0f};
    CardinalDir entryDirInTarget{CardinalDir::West};
};

struct RoomDefinition {
    int id{0};
    int gridX{0};
    int gridY{0};
    int distFromStart{0};
    RoomType type{RoomType::Combat};
    BiomeId biome{BiomeId::DrownedGalleries};
    std::array<int, 4> neighbors{{-1, -1, -1, -1}}; // North, East, South, West room IDs

    std::string name;
    std::string sectorTitle;
    std::string objectiveText;
    SurfaceMaterial floorMaterial{SurfaceMaterial::Stone};
    EnemyVariant enemyVariant{EnemyVariant::Stalker};
    RoomEvent roomEvent{RoomEvent::None};
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
    bool ghostCleared{false};
    bool noiseIntrusionActive{false};
    bool noiseIntrusionConsumed{false};
    CardinalDir pendingNoiseAlertDir{CardinalDir::West};
    float pendingNoiseAlertLevel{0.0f};
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
    int conductorRoomId() const noexcept { return m_conductorRoomId; }
    int exitRoomId() const noexcept { return m_exitRoomId; }

    int findRoomAtGrid(int gx, int gy) const noexcept;
    static sf::Vector2f spawnPositionForEntry(CardinalDir enteredFromDir) noexcept;
    static const char* roomTypeName(RoomType type) noexcept;
    static char roomTypeIcon(RoomType type) noexcept;

    void setCurrentRoom(int roomId) noexcept;
    void setCombatLock(int roomId, bool locked) noexcept;
    void markRoomCleared(int roomId) noexcept;
    bool isRoomCleared(int roomId) const noexcept;

    // Block 1 & Block 5: Cross-room sound propagation, closed-door blocking, & Ghost Clear / RoomEvent rewards
    bool isDoorOpenTowards(int roomId, CardinalDir dir) const noexcept;
    std::vector<RoomNoiseLeak> propagateNoiseFromRoom(
        int sourceRoomId,
        float sourceNoise,
        int maxGridSteps = 2
    ) const;
    bool reopenRoomForNoiseIntrusion(int roomId) noexcept;
    static int computeClearShardReward(
        int baseShards,
        float roomPeakNoise,
        bool& outIsGhostClear,
        RoomEvent roomEvent = RoomEvent::None
    ) noexcept;

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
    int m_conductorRoomId{15};
    int m_exitRoomId{0};

    std::size_t m_currentRoomIndex{0};
    float m_animTime{0.0f};
    std::vector<RoomDefinition> m_rooms;
    std::vector<int> m_activatedSwitches;
    std::vector<int> m_collectedLoreIds;
    std::vector<int> m_visitedRooms;
    std::vector<int> m_clearedRooms;
};

} // namespace echo
