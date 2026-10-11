#pragma once

#include <SFML/Graphics/Color.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace echo {

namespace cfg {
    constexpr unsigned int kDefaultWindowWidth = 1280;
    constexpr unsigned int kDefaultWindowHeight = 800;
    constexpr float kWorldWidth = 1280.0f;
    constexpr float kWorldHeight = 800.0f;
    constexpr float kFixedTimeStep = 1.0f / 60.0f;

    // Pre-allocated pool capacities (zero heap allocations during gameplay loop)
    constexpr std::size_t kMaxActiveRays = 2048;
    constexpr std::size_t kMaxEchoTraces = 2400;
    constexpr std::size_t kMaxWaveRings = 48;

    // Player physical & movement parameters
    constexpr float kPlayerRadius = 12.0f;
    constexpr float kSpeedSneak = 98.0f;
    constexpr float kSpeedWalk = 182.0f;
    constexpr float kSpeedSprint = 292.0f;

    // Ability cooldowns (seconds)
    constexpr float kCooldownPing = 0.36f;
    constexpr float kCooldownRadialPulse = 1.45f;
    constexpr float kCooldownDecoy = 0.80f;
    constexpr float kCooldownWhistle = 0.42f;
    constexpr float kCooldownStomp = 1.65f;
    constexpr float kCooldownEchoTrap = 2.20f;

    // Block 3: Extended Acoustic Arsenal parameters
    constexpr float kWhistleSpreadRad = 0.26179939f; // 15 degrees narrow beam
    constexpr float kWhistleRange = 840.0f;
    constexpr float kStompRadius = 250.0f;
    constexpr float kStompKnockback = 68.0f;
    constexpr float kEchoTrapDelaySec = 3.0f;
    constexpr int kDefaultEchoTrapCount = 3;

    // Block 2: Resonator Column hum duration & radius
    constexpr float kResonatorHumDurationSec = 4.0f;
    constexpr float kResonatorHumRadius = 185.0f;

    // Acoustic defaults
    constexpr std::uint8_t kDefaultMaxReflections = 3;
    constexpr float kEnemyRadius = 14.0f;

    // Roguelike procedural run parameters
    constexpr std::size_t kDefaultRunRoomCount = 32;

    // Block 1 & Block 2 & Block 3: Sound as Risk Resource — Noise Meter (0..100) & Silence parameters
    namespace noise {
        constexpr float kMinNoise = 0.0f;
        constexpr float kMaxNoise = 100.0f;

        // Per-second continuous movement noise rates
        constexpr float kRateSneakPerSec = 2.0f;
        constexpr float kRateWalkPerSec = 8.0f;
        constexpr float kRateSprintPerSec = 25.0f;
        constexpr float kDecaySilencePerSec = 10.0f;

        // Instantaneous action noise spikes
        constexpr float kCostWhistle = 5.0f;       // Block 3.1: Narrow 15-deg Whistle (+5)
        constexpr float kCostPing = 12.0f;         // Block 1.1: Directional Ping (+12)
        constexpr float kCostWaterStep = 15.0f;    // Block 2.4: Footstep on Water (+15)
        constexpr float kCostDecoyImpact = 15.0f;  // Block 1.1: Decoy impact (+15)
        constexpr float kCostCombatHit = 10.0f;    // Block 1.1: Combat hit (+10)
        constexpr float kCostRadialPulse = 30.0f;  // Block 1.1: Radial Pulse (+30)
        constexpr float kCostStomp = 35.0f;        // Block 3.2: Wide Stomp (+35)
        constexpr float kCostGlassShatter = 40.0f; // Block 2.3: Shattering Glass wall (+40)
        constexpr float kCostEchoSwarmAmp = 12.0f; // Block 4.4: EchoSwarm sound amplification (+12)

        // Noise Tier thresholds:
        //   Quiet   : [0.0, 30.0]   (Ghost Clear requires peak noise <= 30.0)
        //   Audible : (30.0, 60.0)
        //   Loud    : [60.0, 85.0)
        //   Scream  : [85.0, 100.0]
        constexpr float kThresholdAudible = 30.0f;
        constexpr float kThresholdLoud = 60.0f;
        constexpr float kThresholdScream = 85.0f;

        // Cross-room attenuation through open doors (50% per doorway hop)
        constexpr float kCrossRoomAttenuation = 0.50f;

        // Ghost Clear reward multiplier when room is cleared without exceeding 30 noise
        constexpr float kGhostClearShardMultiplier = 1.50f;
    } // namespace noise
} // namespace cfg

// Block 1: Noise Tier classification
// Exact intervals:
//   Quiet   : [0.0, 30.0]
//   Audible : (30.0, 60.0)
//   Loud    : [60.0, 85.0)
//   Scream  : [85.0, 100.0]
enum class NoiseTier : std::uint8_t {
    Quiet = 0,   // [0.0, 30.0]   Patrol only (Ghost Clear eligible)
    Audible = 1, // (30.0, 60.0)  Adjacent enemies transition to Investigate
    Loud = 2,    // [60.0, 85.0)  Enemies converge on sound source
    Scream = 3   // [85.0, 100.0] All enemies within 2 grid rooms get priority hunt lock on player
};

enum class NoiseAction : std::uint8_t {
    StepSneak = 0,
    StepWalk = 1,
    StepSprint = 2,
    DirectionalPing = 3,
    RadialPulse = 4,
    DecoyImpact = 5,
    CombatHit = 6,
    SilenceDecay = 7,
    WhistlePing = 8,
    StompImpact = 9,
    GlassShatter = 10,
    WaterSplash = 11,
    EchoSwarmAmplify = 12
};

inline constexpr NoiseTier classifyNoiseTier(float noiseLevel) noexcept {
    if (noiseLevel >= cfg::noise::kThresholdScream) {
        return NoiseTier::Scream;
    }
    if (noiseLevel >= cfg::noise::kThresholdLoud) {
        return NoiseTier::Loud;
    }
    if (noiseLevel > cfg::noise::kThresholdAudible) {
        return NoiseTier::Audible;
    }
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

// Deterministic noise transition function (used by both runtime Game and --self-test)
inline float applyNoiseAction(
    float currentNoise,
    NoiseAction action,
    float dt = 1.0f,
    float gainMultiplier = 1.0f
) noexcept {
    float delta = 0.0f;
    switch (action) {
        case NoiseAction::StepSneak:
            delta = cfg::noise::kRateSneakPerSec * dt * gainMultiplier;
            break;
        case NoiseAction::StepWalk:
            delta = cfg::noise::kRateWalkPerSec * dt * gainMultiplier;
            break;
        case NoiseAction::StepSprint:
            delta = cfg::noise::kRateSprintPerSec * dt * gainMultiplier;
            break;
        case NoiseAction::DirectionalPing:
            delta = cfg::noise::kCostPing * gainMultiplier;
            break;
        case NoiseAction::RadialPulse:
            delta = cfg::noise::kCostRadialPulse * gainMultiplier;
            break;
        case NoiseAction::DecoyImpact:
            delta = cfg::noise::kCostDecoyImpact * gainMultiplier;
            break;
        case NoiseAction::CombatHit:
            delta = cfg::noise::kCostCombatHit * gainMultiplier;
            break;
        case NoiseAction::SilenceDecay:
            delta = -cfg::noise::kDecaySilencePerSec * dt * gainMultiplier;
            break;
        case NoiseAction::WhistlePing:
            delta = cfg::noise::kCostWhistle * gainMultiplier;
            break;
        case NoiseAction::StompImpact:
            delta = cfg::noise::kCostStomp * gainMultiplier;
            break;
        case NoiseAction::GlassShatter:
            delta = cfg::noise::kCostGlassShatter * gainMultiplier;
            break;
        case NoiseAction::WaterSplash:
            delta = cfg::noise::kCostWaterStep * gainMultiplier;
            break;
        case NoiseAction::EchoSwarmAmplify:
            delta = cfg::noise::kCostEchoSwarmAmp * gainMultiplier;
            break;
    }
    return std::clamp(currentNoise + delta, cfg::noise::kMinNoise, cfg::noise::kMaxNoise);
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

enum class MovementStance : std::uint8_t {
    Sneak = 0,
    Walk = 1,
    Sprint = 2
};

// Block 3.4: Player Sonar Frequency Tuning (1 = Low, 2 = Mid, 3 = High)
enum class AcousticFrequency : std::uint8_t {
    Low = 0,  // Penetrates thin walls, longer persistence, heavy resonance vs armored targets
    Mid = 1,  // Balanced standard frequency
    High = 2  // Sharp detail, +1 reflection on metal, unmasks Mimics & disrupts EchoSwarms
};

inline constexpr const char* frequencyName(AcousticFrequency freq) noexcept {
    switch (freq) {
        case AcousticFrequency::Low:  return "LOW [1] (SUB-BASS)";
        case AcousticFrequency::Mid:  return "MID [2] (STANDARD)";
        case AcousticFrequency::High: return "HIGH [3] (ULTRASONIC)";
    }
    return "MID [2]";
}

enum class SurfaceMaterial : std::uint8_t {
    Stone = 0,
    Concrete = 1,
    Metal = 2,
    Water = 3,
    DampMoss = 4,
    ResonanceDoor = 5,
    Glass = 6,
    Count = 7
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
    ResonatorHum = 10,
    WhistleBeam = 11,
    StompWave = 12,
    EchoTrapPulse = 13,
    GlassShatterWave = 14
};

// 4-Directional Cardinal Topology for Room-Based Roguelike Progression
enum class CardinalDir : std::uint8_t {
    North = 0,
    East = 1,
    South = 2,
    West = 3
};

inline constexpr CardinalDir oppositeDir(CardinalDir d) noexcept {
    switch (d) {
        case CardinalDir::North: return CardinalDir::South;
        case CardinalDir::East:  return CardinalDir::West;
        case CardinalDir::South: return CardinalDir::North;
        case CardinalDir::West:  return CardinalDir::East;
    }
    return CardinalDir::South;
}

inline constexpr int dirDeltaX(CardinalDir d) noexcept {
    switch (d) {
        case CardinalDir::East:  return 1;
        case CardinalDir::West:  return -1;
        default:                 return 0;
    }
}

inline constexpr int dirDeltaY(CardinalDir d) noexcept {
    switch (d) {
        case CardinalDir::North: return -1;
        case CardinalDir::South: return 1;
        default:                 return 0;
    }
}

// 10 Distinct Roguelike Chamber Archetypes
enum class RoomType : std::uint8_t {
    Start = 0,
    Combat = 1,
    Treasure = 2,
    Secret = 3,
    Puzzle = 4,
    Sanctuary = 5,
    Shop = 6,
    Elite = 7,
    Boss = 8,
    FinalExit = 9
};

// 9 Collectible Acoustic Relics / Upgrades (including Block 1 SilentStep accelerator)
enum class RelicId : std::uint8_t {
    HarmonicAmplifier = 0,  // +35% Sonar Ping & Pulse range and brightness
    MossTreadBoots = 1,     // -45% Footstep noise radius & immunity to Brittle Shale
    PrismaticMirror = 2,    // +2 Wall reflections & +45% reflected ray damage
    PiezoArmorCore = 3,     // +2 Max Hull Integrity & retaliatory shockwave when hit
    PhantomDecoyRig = 4,    // +3 Decoy capacity & Decoy impact emits damaging sonar shrapnel
    ResonanceCapacitor = 5, // -25% Ping/Pulse cooldowns & +35% Stamina regeneration
    SubharmonicVisor = 6,   // Reveals Secret Rooms on map & boosts passive wall aura
    EchoSiphonCoil = 7,     // Sonar hits on enemies restore Stamina & charge resonators 2x faster
    SilentStep = 8,         // Block 1.4 Ghost Clear reward: -35% global noise generation & +18% movement speed
    Count = 9
};

struct RelicSpec {
    RelicId id;
    const char* name;
    const char* effectSummary;
    sf::Color tint;
    int shopCostShards;
};

inline RelicSpec getRelicSpec(RelicId id) noexcept {
    switch (id) {
        case RelicId::HarmonicAmplifier:
            return {id, "HARMONIC AMPLIFIER", "+35% SONAR RANGE & WAVE INTENSITY", sf::Color(56, 189, 248), 25};
        case RelicId::MossTreadBoots:
            return {id, "MOSS-TREAD BOOTS", "-45% STEP NOISE & SHALE IMMUNITY", sf::Color(52, 211, 153), 22};
        case RelicId::PrismaticMirror:
            return {id, "PRISMATIC MIRROR", "+2 WALL REFLECTIONS & +45% BOUNCE DMG", sf::Color(165, 243, 252), 28};
        case RelicId::PiezoArmorCore:
            return {id, "PIEZO ARMOR CORE", "+2 MAX HULL INTEGRITY & RETALIATION WAVE", sf::Color(251, 191, 36), 30};
        case RelicId::PhantomDecoyRig:
            return {id, "PHANTOM DECOY RIG", "+3 DECOYS & HIGH-ENERGY DECOY SHRAPNEL", sf::Color(192, 132, 252), 24};
        case RelicId::ResonanceCapacitor:
            return {id, "RESONANCE CAPACITOR", "-25% SONAR COOLDOWN & +35% STAMINA REGEN", sf::Color(45, 212, 191), 26};
        case RelicId::SubharmonicVisor:
            return {id, "SUBHARMONIC VISOR", "REVEALS SECRET ROOMS & PASSIVE AURA", sf::Color(244, 114, 182), 25};
        case RelicId::EchoSiphonCoil:
            return {id, "ECHO SIPHON COIL", "SONAR HITS RESTORE STAMINA & 2X RESONATOR CHARGE", sf::Color(129, 140, 248), 26};
        case RelicId::SilentStep:
        default:
            return {id, "SILENT STEP", "-35% NOISE GENERATION & +18% STEALTH SPEED", sf::Color(110, 231, 183), 32};
    }
}

struct MaterialProperties {
    float reflectivity;         // Fraction of wave energy retained after bounce
    float transmission;         // Fraction of sound energy that passes through wall to AI listeners
    float persistenceSec;       // How long wall echo trace remains visible
    float wallThickness;        // Visual thickness of 2.5D wall extrusion in pixels
    int bonusReflections;       // Block 2.2: Additional reflections granted by polished surfaces (Metal = +2)
    float distanceMultiplier;   // Block 2.4: Propagation distance multiplier on contact (Water = 2.0x)
    sf::Color echoColor;        // Primary sonar highlight color
    sf::Color fillTint;         // Extruded wall body tint
    const char* displayName;
};

inline MaterialProperties getMaterialProperties(SurfaceMaterial mat) noexcept {
    switch (mat) {
        case SurfaceMaterial::Stone:
            return {0.82f, 0.24f, 3.0f, 13.0f, 0, 1.0f, sf::Color(56, 189, 248), sf::Color(24, 58, 88), "SLATE STONE"};
        case SurfaceMaterial::Concrete:
            return {0.76f, 0.30f, 2.7f, 14.0f, 0, 1.0f, sf::Color(148, 163, 184), sf::Color(42, 54, 72), "REINFORCED CONCRETE"};
        case SurfaceMaterial::Metal:
            // Block 2.2: Polished Metal mirrors — lossless reflection (1.0f) & up to +2 bonus reflections
            return {1.00f, 0.48f, 3.9f, 11.0f, 2, 1.0f, sf::Color(165, 243, 252), sf::Color(45, 88, 115), "POLISHED METAL"};
        case SurfaceMaterial::Water:
            // Block 2.4: Water — conducts sound to 2x distance
            return {0.72f, 0.56f, 2.4f, 8.0f,  0, 2.0f, sf::Color(45, 212, 191), sf::Color(16, 68, 78), "STAGNANT WATER"};
        case SurfaceMaterial::DampMoss:
            // Block 2.1: Absorbing DampMoss / Foam — reflection attenuated by 80% (reflectivity = 0.20f)
            return {0.20f, 0.14f, 1.5f, 15.0f, 0, 1.0f, sf::Color(52, 211, 153), sf::Color(16, 58, 44), "DAMP MOSS"};
        case SurfaceMaterial::ResonanceDoor:
            return {0.92f, 0.35f, 3.4f, 12.0f, 0, 1.0f, sf::Color(192, 132, 252), sf::Color(68, 38, 102), "RESONANCE GATE"};
        case SurfaceMaterial::Glass:
        default:
            // Block 2.3: Brittle Glass — semi-transparent to sound until shattered by high-energy pulse (+40 noise)
            return {0.62f, 0.72f, 2.6f, 7.0f,  1, 1.0f, sf::Color(224, 242, 254), sf::Color(80, 135, 170), "BRITTLE GLASS"};
    }
}

} // namespace echo
