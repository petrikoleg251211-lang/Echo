#pragma once

#include <SFML/Graphics/Color.hpp>
#include <cstdint>

namespace echo {

enum class SurfaceMaterial : std::uint8_t {
    Stone = 0,         // Cold slate-blue stone masonry (82% reflectivity, 42% transmission)
    Concrete = 1,      // Industrial reinforced concrete (74% reflectivity, 50% transmission)
    Metal = 2,         // Forged iron & steel bulkheads (94% reflectivity, 72% transmission)
    Water = 3,         // Subterranean flooded canal (64% reflectivity, 60% transmission)
    DampMoss = 4,      // Overgrown acoustic-damping moss (36% reflectivity, 30% transmission)
    ResonanceDoor = 5  // Harmonic lock barrier (90% reflectivity, 45% transmission)
};

enum class WaveSignature : std::uint8_t {
    FootstepSneak = 0,
    FootstepWalk = 1,
    FootstepSprint = 2,
    DirectionalPing = 3,
    RadialPulse = 4,
    DecoyImpact = 5,
    MechanismClunk = 6,
    EnemyTremor = 7,
    AmbientDrip = 8,
    ShaleCrack = 9,
    ResonatorHum = 10
};

enum class CardinalDir : std::uint8_t {
    North = 0,
    East = 1,
    South = 2,
    West = 3,
    Count = 4
};

inline constexpr CardinalDir oppositeDir(CardinalDir dir) noexcept {
    switch (dir) {
        case CardinalDir::North: return CardinalDir::South;
        case CardinalDir::East:  return CardinalDir::West;
        case CardinalDir::South: return CardinalDir::North;
        case CardinalDir::West:  return CardinalDir::East;
        default:                 return CardinalDir::South;
    }
}

inline constexpr int dirDeltaX(CardinalDir dir) noexcept {
    switch (dir) {
        case CardinalDir::East: return 1;
        case CardinalDir::West: return -1;
        default:                return 0;
    }
}

inline constexpr int dirDeltaY(CardinalDir dir) noexcept {
    switch (dir) {
        case CardinalDir::North: return -1;
        case CardinalDir::South: return 1;
        default:                 return 0;
    }
}

enum class RoomType : std::uint8_t {
    Start = 0,      // Safe starting chamber at (0,0)
    Combat = 1,     // Acoustic enemy encounter; locks doors until cleared
    Treasure = 2,   // Contains an Acoustic Relic pedestal
    Secret = 3,     // Concealed behind an illusory wall opened by sonar pulses
    Puzzle = 4,     // Harmonic Resonator & mirror chamber
    Sanctuary = 5,  // Safe room: restores hull integrity & saves checkpoint
    Shop = 6,       // Quartermaster terminal: exchange Echo Shards for upgrades
    Elite = 7,      // High-threat multi-wave / Warden encounter with Relic reward
    Boss = 8,       // The Choir Sovereign multi-phase acoustic boss arena
    FinalExit = 9   // Surface Ascent Shaft unlocked after defeating the Boss
};

enum class RelicId : std::uint8_t {
    FocusedTransducer = 0,  // +35% Ping range, narrower cone, +40% sonar damage
    PrismaticMirror = 1,    // +2 Wall reflections, reflected rays deal +35% damage
    MossTreadBoots = 2,     // -45% Footstep noise radius & immunity to Brittle Shale
    ResonanceCapacitor = 3, // +35% Stamina regen & -25% Ping/Pulse cooldowns
    SubharmonicVisor = 4,   // Reveals Secret rooms on map & boosts passive dark vision
    PiezoArmorCore = 5,     // +2 Max Hull Integrity & defensive shockwave on hit
    EchoSiphonCoil = 6,     // Sonar hits restore Stamina & charge Resonators 2x faster
    PhantomDecoyRig = 7,    // +3 Decoys & Decoy impacts trigger damaging sonic burst
    Count = 8
};

enum class RelicRarity : std::uint8_t {
    Common = 0,
    Uncommon = 1,
    Rare = 2,
    Relic = 3
};

struct RelicSpec {
    RelicId id;
    const char* name;
    const char* effectSummary;
    RelicRarity rarity;
    int shopCostShards;
    sf::Color tint;
};

inline RelicSpec getRelicSpec(RelicId id) noexcept {
    switch (id) {
        case RelicId::FocusedTransducer:
            return {id, "FOCUSED TRANSDUCER", "+35% SONAR RANGE & +40% PING DISRUPTION DAMAGE", RelicRarity::Common, 25, sf::Color(56, 189, 248)};
        case RelicId::PrismaticMirror:
            return {id, "PRISMATIC LENS", "+2 WALL REFLECTIONS & +35% BOUNCE DAMAGE", RelicRarity::Uncommon, 35, sf::Color(167, 139, 250)};
        case RelicId::MossTreadBoots:
            return {id, "BRYOPHYTE TREADS", "-45% FOOTSTEP NOISE & SHALE CRACK IMMUNITY", RelicRarity::Common, 22, sf::Color(52, 211, 153)};
        case RelicId::ResonanceCapacitor:
            return {id, "HARMONIC CAPACITOR", "+35% STAMINA REGEN & -25% SONAR COOLDOWNS", RelicRarity::Uncommon, 32, sf::Color(250, 204, 21)};
        case RelicId::SubharmonicVisor:
            return {id, "SUBHARMONIC VISOR", "REVEALS SECRET ROOMS ON MAP & +DARK VISION", RelicRarity::Rare, 42, sf::Color(45, 212, 191)};
        case RelicId::PiezoArmorCore:
            return {id, "PIEZO ARMOR CORE", "+2 MAX HULL INTEGRITY & RETALIATORY PULSE", RelicRarity::Rare, 45, sf::Color(248, 113, 113)};
        case RelicId::EchoSiphonCoil:
            return {id, "ECHO SIPHON COIL", "SONAR HITS RESTORE STAMINA & 2X CRYSTAL CHARGE", RelicRarity::Uncommon, 30, sf::Color(125, 211, 252)};
        case RelicId::PhantomDecoyRig:
        default:
            return {RelicId::PhantomDecoyRig, "PHANTOM DECOY RIG", "+3 DECOYS & LETHAL RESONANCE DECOY BLAST", RelicRarity::Rare, 38, sf::Color(216, 180, 254)};
    }
}

enum class AppState : std::uint8_t {
    MainMenu = 0,
    Playing = 1,
    Paused = 2,
    Settings = 3,
    Controls = 4,
    GameOver = 5,
    Victory = 6
};

struct MaterialProperties {
    float reflectivity;      // [0.0, 1.0] fraction of energy retained after reflection
    float transmission;      // [0.0, 1.0] fraction of sound loudness transmitted through wall to AI
    float persistenceSec;    // How long the illuminated wall trace stays visible
    float wallThickness;     // Visual bevel thickness in world units
    sf::Color echoColor;     // Primary acoustic illumination color
    sf::Color fillTint;      // Dark structural fill color when illuminated
    const char* label;
};

inline MaterialProperties getMaterialProperties(SurfaceMaterial mat) noexcept {
    switch (mat) {
        case SurfaceMaterial::Stone:
            return {0.82f, 0.42f, 4.0f, 10.0f, sf::Color(56, 189, 248), sf::Color(15, 32, 52), "Stone Masonry"};
        case SurfaceMaterial::Concrete:
            return {0.74f, 0.50f, 3.2f, 11.0f, sf::Color(125, 211, 252), sf::Color(22, 34, 48), "Reinforced Concrete"};
        case SurfaceMaterial::Metal:
            return {0.94f, 0.72f, 5.4f, 8.0f, sf::Color(34, 211, 238), sf::Color(18, 42, 56), "Forged Steel"};
        case SurfaceMaterial::Water:
            return {0.64f, 0.60f, 2.8f, 6.0f, sf::Color(45, 212, 191), sf::Color(10, 38, 44), "Subterranean Water"};
        case SurfaceMaterial::DampMoss:
            return {0.36f, 0.30f, 1.7f, 12.0f, sf::Color(52, 211, 153), sf::Color(12, 34, 28), "Damp Acoustic Moss"};
        case SurfaceMaterial::ResonanceDoor:
            return {0.90f, 0.45f, 4.8f, 9.0f, sf::Color(167, 139, 250), sf::Color(36, 22, 62), "Resonance Gate"};
    }
    return {0.82f, 0.42f, 4.0f, 10.0f, sf::Color(56, 189, 248), sf::Color(15, 32, 52), "Stone Masonry"};
}

namespace cfg {
    constexpr unsigned int kDefaultWindowWidth = 1280;
    constexpr unsigned int kDefaultWindowHeight = 800;
    constexpr float kWorldWidth = 1280.0f;
    constexpr float kWorldHeight = 800.0f;
    constexpr float kFixedTimeStep = 1.0f / 60.0f;

    // Pre-allocated simulation pool sizes (zero per-frame heap allocations)
    constexpr std::size_t kMaxActiveRays = 2560;
    constexpr std::size_t kMaxEchoTraces = 2048;
    constexpr std::size_t kMaxWaveRings = 48;
    constexpr std::uint8_t kDefaultMaxReflections = 3;

    // Default procedural roguelike run size (configurable 15..60 rooms)
    constexpr std::size_t kDefaultRunRoomCount = 32;

    // Player physical & movement parameters
    constexpr float kPlayerRadius = 12.0f;
    constexpr float kEnemyRadius = 14.0f;
    constexpr float kSpeedSneak = 86.0f;
    constexpr float kSpeedWalk = 154.0f;
    constexpr float kSpeedSprint = 238.0f;

    // Ability cooldowns (seconds)
    constexpr float kCooldownPing = 0.48f;
    constexpr float kCooldownRadialPulse = 1.95f;
    constexpr float kCooldownDecoy = 2.20f;
} // namespace cfg

} // namespace echo
