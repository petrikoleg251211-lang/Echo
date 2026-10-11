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
        constexpr float kCostEchoStorm = 6.0f;     // Block 5.1: EchoStorm periodic ambient sweep (+6)
        constexpr float kCostBossDissonance = 15.0f; // Block 5.4: Boss Phase II dissonance wave (+15 if moving)

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
    EchoSwarmAmplify = 12,
    EchoStormPulse = 13,
    BossDissonanceShock = 14
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
        case NoiseAction::EchoStormPulse:
            delta = cfg::noise::kCostEchoStorm * gainMultiplier;
            break;
        case NoiseAction::BossDissonanceShock:
            delta = cfg::noise::kCostBossDissonance * gainMultiplier;
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
    Victory = 6,
    Codex = 7
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

// 12 Collectible Acoustic Relics / Upgrades (including Block 1 SilentStep & Block 5 Acoustic Relics)
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
    PhaseDampener = 9,      // Block 5.2: +50% Silence decay speed & -30% non-footstep acoustic spike noise
    UltrasonicLens = 10,    // Block 5.2: +40% High-Freq [3] / Whistle damage & +2 Echo Trap capacity
    AbyssalTuningFork = 11, // Block 5.2: +25% Echo Shard gain & +30% resonance damage vs Elite/Boss
    Count = 12
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
            return {id, "SILENT STEP", "-35% NOISE GENERATION & +18% STEALTH SPEED", sf::Color(110, 231, 183), 32};
        case RelicId::PhaseDampener:
            return {id, "PHASE DAMPENER", "+50% SILENCE DECAY & -30% PULSE/PING NOISE", sf::Color(96, 165, 250), 27};
        case RelicId::UltrasonicLens:
            return {id, "ULTRASONIC LENS", "+40% HIGH-FREQ DMG & +2 ECHO TRAPS", sf::Color(232, 121, 249), 28};
        case RelicId::AbyssalTuningFork:
        default:
            return {id, "ABYSSAL TUNING FORK", "+25% SHARD GAIN & +30% BOSS/ELITE RESONANCE", sf::Color(250, 204, 21), 34};
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

// Block 5.1: Dynamic Acoustic Room Events / Modifiers
enum class RoomEvent : std::uint8_t {
    None = 0,
    EchoStorm = 1,          // Periodic ambient wave sweeps every 3.5s illuminate room & add +6 ambient noise
    AcousticBlackout = 2,   // Dead-zone chamber: sonar range & echo persistence reduced by 45%, but +50% shard reward
    ResonanceOvercharge = 3 // High-conductance chamber: 2x resonator charge speed, +25% sonar damage, +20% noise gain
};

inline constexpr const char* roomEventName(RoomEvent ev) noexcept {
    switch (ev) {
        case RoomEvent::None:                return "STANDARD ACOUSTICS";
        case RoomEvent::EchoStorm:           return "ECHO STORM (PERIODIC SONAR SWEEPS)";
        case RoomEvent::AcousticBlackout:    return "ACOUSTIC BLACKOUT (-45% RANGE, +50% SHARDS)";
        case RoomEvent::ResonanceOvercharge: return "RESONANCE OVERCHARGE (2X CHARGE, +25% DMG)";
    }
    return "STANDARD ACOUSTICS";
}

// Block 5.4: Multi-Phase Acoustic Boss Arena Mechanics (The Choir Sovereign)
enum class BossArenaAcousticPhase : std::uint8_t {
    HarmonicChoir = 0,   // Phase 1 (100%..67% HP): Standard acoustic reflections & 3-bolt harmonic volleys
    DissonantPulse = 1,  // Phase 2 (66%..34% HP): Periodic arena dissonance shockwave (+15 noise if moving) & 6-bolt ring
    AbyssalSilence = 2   // Phase 3 (33%..0% HP): High-frequency resonance required or reflected shots deal 2x damage; 8-bolt ring
};

inline constexpr const char* bossArenaPhaseName(BossArenaAcousticPhase phase) noexcept {
    switch (phase) {
        case BossArenaAcousticPhase::HarmonicChoir:  return "PHASE I: HARMONIC CHOIR";
        case BossArenaAcousticPhase::DissonantPulse: return "PHASE II: DISSONANT SHOCKWAVE";
        case BossArenaAcousticPhase::AbyssalSilence: return "PHASE III: ABYSSAL SILENCE";
    }
    return "PHASE I: HARMONIC CHOIR";
}

// Block 5.3: Audio-Lore Codex (12 Collectible Subterranean Acoustic Logs)
constexpr std::size_t kTotalCodexFragments = 12;

struct CodexEntrySpec {
    int id;
    const char* title;
    const char* category;
    const char* excerpt;
};

inline CodexEntrySpec getCodexEntrySpec(int index) noexcept {
    switch (index % static_cast<int>(kTotalCodexFragments)) {
        case 0:
            return {200, "LOG 01: RESONANCE PROTOCOL", "EXPEDITION DIRECTIVE",
                    "SOUND IS SIGHT, WEAPON, AND RISK. KEEP CHAMBER PEAK NOISE <= 30 TO ACHIEVE GHOST CLEAR."};
        case 1:
            return {201, "LOG 02: THE BLIND ECOSYSTEM", "XENO-BIOLOGY",
                    "SIGHTLESS STALKERS EVOLVED HYPER-ACUTE TYMPANIC MEMBRANES. EVEN A SINGLE SPRINT STEP DRAWS THEM."};
        case 2:
            return {202, "LOG 03: BRYOPHYTE DAMPENING", "MATERIAL SCIENCE",
                    "CAVERN DAMP MOSS ABSORBS 80% OF SONAR REFLECTIONS AND HALVES FOOTSTEP NOISE WHEN CROUCHING NEARBY."};
        case 3:
            return {203, "LOG 04: SPECULAR STEEL MIRRORS", "ACOUSTIC OPTICS",
                    "POLISHED FOUNDRY PLATES REFLECT WAVES WITHOUT ENERGY LOSS AND ADD +2 EXTRA SPECULAR BOUNCES."};
        case 4:
            return {204, "LOG 05: BRITTLE SILICATE PARTITIONS", "STRUCTURAL HAZARD",
                    "CRYSTALLINE GLASS TRANSMITS 72% OF SOUND UNTIL SHATTERED BY RADIAL PULSE OR STOMP (+40 NOISE)."};
        case 5:
            return {205, "LOG 06: HYDRO-ACOUSTIC CONDUCTION", "FLUID DYNAMICS",
                    "FLOODED AQUEDUCT PUDDLES DOUBLE WAVE PROPAGATION RANGE BUT EMIT A +15 NOISE SPLASH WHEN CROSSED."};
        case 6:
            return {206, "LOG 07: HARMONIC PIPE COLUMNS", "RESONANCE LURES",
                    "STRIKING A HOLLOW CONDUIT COLUMN SETS OFF A 4-SECOND HARMONIC HUM THAT ILLUMINATES AND LURES HUNTERS."};
        case 7:
            return {207, "LOG 08: ULTRASONIC WHISTLE & STOMP", "TACTICAL ARSENAL",
                    "NARROW 15-DEG WHISTLE BEAMS ADD ONLY +5 NOISE; SEISMIC STOMPS (+35 NOISE) REPEL HUNTERS AND SHATTER GLASS."};
        case 8:
            return {208, "LOG 09: SPECIMEN ALPHA — SILENT HUNTER", "THREAT DOSSIER",
                    "EMITS ZERO PASSIVE FOOTSTEP TREMORS. LISTEN FOR ITS 0.5-SECOND INHALE HISS BEFORE IT LUNGES."};
        case 9:
            return {209, "LOG 10: SPECIMEN BETA — DEAF BRUTE", "THREAT DOSSIER",
                    "OSSIFIED CRANIUM IGNORES AIRBORNE WHISTLES AND PINGS; REACTS SOLELY TO GROUND VIBRATIONS."};
        case 10:
            return {210, "LOG 11: SPECIMEN GAMMA — RESONANCE MIMIC", "THREAT DOSSIER",
                    "IMITATES HARMONIC CRYSTALS WITH DECEPTIVE LURE PULSES. SCAN WITH HIGH FREQUENCY [3] OR WHISTLE [Q] TO UNMASK."};
        case 11:
        default:
            return {211, "LOG 12: THE CHOIR SOVEREIGN", "ABYSSAL CORE",
                    "THE THREE-PHASE APEX CONDUCTOR OF THE DEEP LABYRINTH. SHATTER ITS HARMONIC SHELL TO UNSEAL THE ASCENT."};
    }
}

} // namespace echo
