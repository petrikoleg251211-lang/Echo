#pragma once

#include <SFML/Graphics/Color.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace echo {

enum class SurfaceMaterial : std::uint8_t {
    Stone = 0,         // Cold slate-blue stone masonry (82% reflectivity, 42% transmission)
    Concrete = 1,      // Dense structural concrete (74% reflectivity, 50% transmission)
    Metal = 2,         // High-resonance steel plating (94% reflectivity, 72% transmission)
    Water = 3,         // Subterranean flooded basin (64% reflectivity, 60% transmission)
    DampMoss = 4,      // Sound-absorbing organic moss (36% reflectivity, 30% transmission)
    ResonanceDoor = 5  // Harmonic lock gate (90% reflectivity, 45% transmission when sealed)
};

enum class WaveSignature : std::uint8_t {
    FootstepSneak = 0,
    FootstepWalk = 1,
    FootstepSprint = 2,
    DirectionalPing = 3,
    RadialPulse = 4,
    DecoyImpact = 5,
    SwitchClunk = 6,
    MechanismClunk = 6,
    EnemyTremor = 7,
    AmbientDrip = 8,
    ResonatorHum = 9,
    ShaleCrack = 10
};

// EchoLabyrinth 4.0/5.0: Room-Based Roguelike Chamber Categories
enum class RoomType : std::uint8_t {
    Start = 0,      // Entry Sanctuary (Safe, Checkpoint, Tutorial Lore)
    Combat = 1,     // Standard Acoustic Hunter encounter (locks doors until cleared)
    Treasure = 2,   // Harmonic Relic Vault (contains a free Relic Pedestal)
    Puzzle = 3,     // Acoustic Resonator / Mirror alignment puzzle chamber
    Shop = 4,       // Acoustic Quartermaster (spend Echo Shards on Relics, Repairs, Decoys)
    Secret = 5,     // Hidden fracture chamber (unsealed by pinging hollow wall with sonar)
    Challenge = 6,  // High-risk multi-wave arena (locks doors, 2 waves + high Shard/Relic reward)
    Elite = 6,      // Alias for Challenge/Elite chamber
    Sanctuary = 7,  // Acoustic Calibration Shrine (Checkpoint + free +2 Hull Integrity restore)
    Boss = 8,       // The Choir Sovereign Lair (3-phase acoustic boss encounter)
    FinalExit = 9   // Resonance Ascent Conduit (unlocked after defeating the Boss)
};

// 4-Directional Room Connections (Binding of Isaac grid style)
enum class CardinalDir : std::uint8_t {
    North = 0,
    East = 1,
    South = 2,
    West = 3
};

inline constexpr CardinalDir oppositeDir(CardinalDir dir) noexcept {
    switch (dir) {
        case CardinalDir::North: return CardinalDir::South;
        case CardinalDir::East:  return CardinalDir::West;
        case CardinalDir::South: return CardinalDir::North;
        case CardinalDir::West:  return CardinalDir::East;
    }
    return CardinalDir::South;
}

inline constexpr int dirDeltaX(CardinalDir dir) noexcept {
    switch (dir) {
        case CardinalDir::East:  return 1;
        case CardinalDir::West:  return -1;
        default:                 return 0;
    }
}

inline constexpr int dirDeltaY(CardinalDir dir) noexcept {
    switch (dir) {
        case CardinalDir::North: return -1;
        case CardinalDir::South: return 1;
        default:                 return 0;
    }
}

// EchoLabyrinth 4.0/5.0: Passive & Active Acoustic Relics / Upgrades
enum class RelicId : std::uint8_t {
    HarmonicAmplifier = 0, // +35% Sonar Range, +20% Ray Count, +25% Harmonic Damage
    MossTreadBoots = 1,    // -45% Footstep Loudness, +15% Walk/Sneak Speed, Brittle Shale Immunity
    PrismaticMirror = 2,   // +2 Max Ray Reflections, reflected rays deal +50% bonus damage
    KineticCapacitor = 3,  // +35% Stamina Regen, -25% Ability Cooldowns
    ResonanceCapacitor = 3,// Alias for KineticCapacitor
    FocusedTransducer = 4, // Focused Ping [LMB] fires twin harmonic beams with +45% lethality
    SubharmonicVisor = 5,  // Reveals Secret Rooms on map & highlights enemies in dark at close range
    PiezoArmorCore = 6,    // +2 Max Hull Integrity (HP) & emits retaliatory shockwave when hit
    EchoSiphonCoil = 7,    // Sonar hits on enemies restore +6 Stamina & charge crystals 2x faster
    PhantomDecoyRig = 8,   // +3 Decoy Stones & Decoys emit 3x pulsing waves that damage nearby hunters
    SilentStep = 9,        // Block 1.4 Ghost Clear Relic: +18% Movement Speed & -35% Noise Meter gain
    Count = 10
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
        case RelicId::HarmonicAmplifier:
            return {id, "HARMONIC AMPLIFIER", "+35% SONAR RANGE, +20% RAYS, +25% SONAR DMG", RelicRarity::Uncommon, 30, sf::Color(56, 189, 248)};
        case RelicId::MossTreadBoots:
            return {id, "MOSS-TREAD BOOTS", "-45% STEP SOUND, +15% SPEED, SHALE IMMUNITY", RelicRarity::Common, 22, sf::Color(52, 211, 153)};
        case RelicId::PrismaticMirror:
            return {id, "PRISMATIC MIRROR", "+2 RAY BOUNCES & +50% REFLECTED RAY DAMAGE", RelicRarity::Rare, 40, sf::Color(167, 139, 250)};
        case RelicId::KineticCapacitor:
            return {id, "KINETIC CAPACITOR", "+35% STAMINA REGEN & -25% SONAR COOLDOWNS", RelicRarity::Common, 25, sf::Color(250, 204, 21)};
        case RelicId::FocusedTransducer:
            return {id, "FOCUSED TRANSDUCER", "TWIN HARMONIC PING BEAMS & +45% PING DAMAGE", RelicRarity::Relic, 50, sf::Color(244, 63, 94)};
        case RelicId::SubharmonicVisor:
            return {id, "SUBHARMONIC VISOR", "REVEALS SECRET ROOMS ON MAP & +DARK VISION", RelicRarity::Rare, 42, sf::Color(45, 212, 191)};
        case RelicId::PiezoArmorCore:
            return {id, "PIEZO ARMOR CORE", "+2 MAX HULL INTEGRITY & RETALIATORY PULSE", RelicRarity::Rare, 45, sf::Color(248, 113, 113)};
        case RelicId::EchoSiphonCoil:
            return {id, "ECHO SIPHON COIL", "SONAR HITS RESTORE STAMINA & 2X CRYSTAL CHARGE", RelicRarity::Uncommon, 30, sf::Color(125, 211, 252)};
        case RelicId::PhantomDecoyRig:
            return {id, "PHANTOM DECOY RIG", "+3 DECOYS & LETHAL RESONANCE DECOY BLAST", RelicRarity::Rare, 38, sf::Color(216, 180, 254)};
        case RelicId::SilentStep:
        default:
            return {RelicId::SilentStep, "SILENT STEP ACCELERATOR", "GHOST CLEAR RELIC: +18% SPEED & -35% NOISE METER GAIN", RelicRarity::Relic, 40, sf::Color(110, 231, 183)};
    }
}

// ============================================================================
// BLOCK 1: Sound as Risk Resource — Noise Meter & Silence System (0..100)
// ============================================================================
enum class NoiseTier : std::uint8_t {
    Quiet = 0,    // [0.0, 30.0]:  Enemies remain in Patrol; eligible for Ghost Clear (x1.5 Shards)
    Audible = 1,  // (30.0, 60.0): Enemies in current/adjacent open rooms transition to InvestigateSound
    Loud = 2,     // [60.0, 85.0): Enemies converge directly on the sound origin
    Scream = 3    // [85.0, 100.0]: All enemies within 2 grid rooms receive priority hunt lock on player
};

enum class NoiseAction : std::uint8_t {
    StepSneak = 0,       // +2 / sec
    StepWalk = 1,        // +8 / sec
    StepSprint = 2,      // +25 / sec
    DirectionalPing = 3, // +12 instant
    RadialPulse = 4,     // +30 instant
    DecoyImpact = 5,     // +15 instant at impact point
    CombatHit = 6,       // +10 instant per hit
    SilenceDecay = 7     // -10 / sec in silence
};

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

    // Block 1: Noise & Silence tuning parameters
    namespace noise {
        constexpr float kMinNoise = 0.0f;
        constexpr float kMaxNoise = 100.0f;

        constexpr float kStepSneakPerSec = 2.0f;
        constexpr float kStepWalkPerSec = 8.0f;
        constexpr float kStepSprintPerSec = 25.0f;
        constexpr float kPingNoise = 12.0f;
        constexpr float kRadialPulseNoise = 30.0f;
        constexpr float kDecoyImpactNoise = 15.0f;
        constexpr float kCombatHitNoise = 10.0f;
        constexpr float kSilenceDecayPerSec = 10.0f;

        // Exact half-open / closed interval boundaries:
        // Quiet:   [0.0, 30.0]  (<= kThresholdAudible; eligible for Ghost Clear)
        // Audible: (30.0, 60.0) (>  kThresholdAudible && < kThresholdLoud)
        // Loud:    [60.0, 85.0) (>= kThresholdLoud    && < kThresholdScream)
        // Scream:  [85.0, 100.0](>= kThresholdScream)
        constexpr float kThresholdAudible = 30.0f;
        constexpr float kThresholdLoud = 60.0f;
        constexpr float kThresholdScream = 85.0f;

        constexpr float kCrossRoomAttenuation = 0.50f; // 50% attenuation per open doorway hop
        constexpr float kGhostClearShardMultiplier = 1.50f;
    } // namespace noise
} // namespace cfg

inline constexpr NoiseTier classifyNoiseTier(float noiseLevel) noexcept {
    if (noiseLevel >= cfg::noise::kThresholdScream) return NoiseTier::Scream;
    if (noiseLevel >= cfg::noise::kThresholdLoud)   return NoiseTier::Loud;
    if (noiseLevel >  cfg::noise::kThresholdAudible) return NoiseTier::Audible;
    return NoiseTier::Quiet;
}

inline constexpr const char* noiseTierName(NoiseTier tier) noexcept {
    switch (tier) {
        case NoiseTier::Quiet:   return "QUIET";
        case NoiseTier::Audible: return "AUDIBLE";
        case NoiseTier::Loud:    return "LOUD";
        case NoiseTier::Scream:  return "SCREAM";
    }
    return "QUIET";
}

inline float computeNoiseActionDelta(NoiseAction action, float dt = 1.0f, float gainMultiplier = 1.0f) noexcept {
    switch (action) {
        case NoiseAction::StepSneak:       return cfg::noise::kStepSneakPerSec * dt * gainMultiplier;
        case NoiseAction::StepWalk:        return cfg::noise::kStepWalkPerSec * dt * gainMultiplier;
        case NoiseAction::StepSprint:      return cfg::noise::kStepSprintPerSec * dt * gainMultiplier;
        case NoiseAction::DirectionalPing: return cfg::noise::kPingNoise * gainMultiplier;
        case NoiseAction::RadialPulse:     return cfg::noise::kRadialPulseNoise * gainMultiplier;
        case NoiseAction::DecoyImpact:     return cfg::noise::kDecoyImpactNoise * gainMultiplier;
        case NoiseAction::CombatHit:       return cfg::noise::kCombatHitNoise * gainMultiplier;
        case NoiseAction::SilenceDecay:    return -cfg::noise::kSilenceDecayPerSec * dt;
    }
    return 0.0f;
}

inline float applyNoiseAction(
    float currentNoise,
    NoiseAction action,
    float dt = 1.0f,
    float gainMultiplier = 1.0f
) noexcept {
    const float next = currentNoise + computeNoiseActionDelta(action, dt, gainMultiplier);
    return std::clamp(next, cfg::noise::kMinNoise, cfg::noise::kMaxNoise);
}

} // namespace echo
