#pragma once

#include <SFML/Graphics/Color.hpp>
#include <algorithm>
#include <array>
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

    // Stage 1: Game Feel, Hit-Stop, Exponential Screen Shake, & Enemy Attack Telegraphing
    namespace feel {
        constexpr float kHitStopDurationSec = 0.045f;       // 45 ms hit-stop on sonar damage
        constexpr float kHitStopKillDurationSec = 0.060f;   // 60 ms hit-stop when enemy shatters
        constexpr float kHitStopSonarHitSec = kHitStopDurationSec;
        constexpr float kHitStopEnemyKillSec = kHitStopKillDurationSec;

        constexpr float kCameraShakeDecayRate = 7.5f;       // Exponential decay rate for screen shake
        constexpr float kShakeDecayRate = kCameraShakeDecayRate;
        constexpr float kShakeAmpSonarHit = 4.2f;           // Subtle camera kick on sonar enemy impact
        constexpr float kShakeAmpEnemyAlert = 6.8f;         // Camera shake when hunter enters pursuit
        constexpr float kShakeAmpStomp = 8.5f;              // Seismic stomp shake amplitude
        constexpr float kShakeAmpGlassShatter = 7.8f;       // Glass barrier shatter amplitude
        constexpr float kShakeAmpPlayerDamage = 13.5f;      // Heavy impact shake when player takes hull damage
        constexpr float kShakeAmplitudeSonarHit = kShakeAmpSonarHit;
        constexpr float kShakeAmplitudeKill = 7.5f;
        constexpr float kShakeAmplitudeAlert = kShakeAmpEnemyAlert;
        constexpr float kShakeAmplitudeDamage = kShakeAmpPlayerDamage;

        constexpr float kEchoPulseFreqRad = 11.5f;          // Pulsing frequency (rad/s) for wall echo traces
        constexpr float kEchoPulseAmplitude = 0.24f;        // Amplitude of harmonic wall echo trace pulsation
        constexpr float kEnemyMeleeTelegraphSec = 0.45f;    // 0.45s (in 0.3..0.6s) pre-strike telegraph windup
        constexpr float kEnemyRangedTelegraphSec = 0.42f;   // 0.42s windup before Shrieker/Boss fires SonicBolts
        constexpr float kMeleeTelegraphRangeMargin = 22.0f; // Extra distance at which melee windup starts
        constexpr float kThreatIndicatorNoiseMin = 28.0f;   // Noise threshold to show edge-of-screen threat arrows
        constexpr float kThreatArrowScreenMargin = 54.0f;   // Margin from viewport edge for threat chevrons
        constexpr int kTutorialTotalRooms = 3;              // 3 interactive tutorial chambers on first run
    } // namespace feel

    // Block 1 & Block 2 & Block 3: Sound as Risk Resource — Noise Meter (0..100) & Silence parameters
    namespace noise {
        constexpr float kMinNoise = 0.0f;
        constexpr float kMaxNoise = 100.0f;

        // Per-second continuous movement noise rates
        constexpr float kRateSneakPerSec = 2.0f;
        constexpr float kRateWalkPerSec = 8.0f;
        constexpr float kRateSprintPerSec = 25.0f;
        constexpr float kDecaySilencePerSec = 10.0f;
        constexpr float kSneakNoisePerSec = kRateSneakPerSec;
        constexpr float kWalkNoisePerSec = kRateWalkPerSec;
        constexpr float kSprintNoisePerSec = kRateSprintPerSec;

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
        constexpr float kCostBansheeWail = 22.0f;  // Stage 2: Banshee acoustic wail spike (+22)
        constexpr float kCostLeechDrain = 8.0f;    // Stage 2: Leech parasite acoustic drain (+8)

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

// Stage 1.3: Four Difficulty Modes
// - Explorer (Easy): Forgiving hull integrity, slower enemy pursuit, faster silence decay, +25% shards
// - Surveyor (Normal): Standard balanced acoustic survival (target bot win rate 8%..15%)
// - Abyssal  (Hard): +25% enemy spawns, slower silence decay (x0.72), -25% Echo Shards, +20% enemy HP
// - Silence  (Hardcore): 1 HP (1 life), NO minimap (corner or overlay), +25% enemies, x0.68 silence decay
enum class DifficultyMode : std::uint8_t {
    Explorer = 0,
    Surveyor = 1,
    Abyssal = 2,
    Silence = 3,
    Count = 4
};

struct DifficultyProfile {
    DifficultyMode mode;
    const char* name;
    const char* shortDesc;
    int playerStartHp;
    float enemyCountMult;
    float enemyHpMult;
    float enemySpeedMult;
    float noiseGainMult;
    float silenceDecayMult; // < 1.0 means noise decays slower
    float shardRewardMult;  // < 1.0 means fewer Echo Shards
    bool oneLifeOnly;       // true on Silence
    bool allowMinimap;      // false on Silence
};

inline constexpr DifficultyProfile getDifficultyProfile(DifficultyMode mode) noexcept {
    switch (mode) {
        case DifficultyMode::Explorer:
            return {
                DifficultyMode::Explorer,
                "EXPLORER",
                "EASY: 8 HP, -20% ENEMY HP, +25% SILENCE DECAY & SHARDS",
                8, 0.80f, 0.80f, 0.90f, 0.85f, 1.25f, 1.25f, false, true
            };
        case DifficultyMode::Surveyor:
            return {
                DifficultyMode::Surveyor,
                "SURVEYOR",
                "NORMAL: STANDARD ACOUSTIC SURVIVAL BALANCE (6 HP)",
                6, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, false, true
            };
        case DifficultyMode::Abyssal:
            return {
                DifficultyMode::Abyssal,
                "ABYSSAL",
                "HARD: +25% HUNTERS, SLOWER SILENCE DECAY, -25% SHARDS",
                5, 1.25f, 1.20f, 1.08f, 1.15f, 0.72f, 0.75f, false, true
            };
        case DifficultyMode::Silence:
        default:
            return {
                DifficultyMode::Silence,
                "SILENCE",
                "HARDCORE: 1 HP, NO MINIMAP, +25% HUNTERS, SLOWER DECAY",
                1, 1.25f, 1.25f, 1.12f, 1.20f, 0.68f, 0.75f, true, false
            };
    }
}

inline constexpr const char* difficultyModeName(DifficultyMode mode) noexcept {
    switch (mode) {
        case DifficultyMode::Explorer: return "EXPLORER [EASY]";
        case DifficultyMode::Surveyor: return "SURVEYOR [NORMAL]";
        case DifficultyMode::Abyssal:  return "ABYSSAL [HARD]";
        case DifficultyMode::Silence:  return "SILENCE [HARDCORE 1-HP]";
        default:                       return "SURVEYOR [NORMAL]";
    }
}

inline constexpr const char* difficultyName(DifficultyMode mode) noexcept {
    return getDifficultyProfile(mode).name;
}

// Stage 1.2 & Stage 2: Centralized BalanceTable in Config.hpp (HP, speed, damage, noise per action, 13 enemy variants)
namespace cfg {
namespace balance {

struct EnemyBalanceSpec {
    float maxHp;
    float speedScale;
    float patrolSpeed;
    float investigateSpeed;
    float pursueSpeed;
    int contactDamage;
};

struct BalanceTable {
    // Player baseline stats
    int playerBaseHp{6};
    int playerMaxHpCap{12};
    float playerStaminaMax{100.0f};
    float speedSneak{kSpeedSneak};
    float speedWalk{kSpeedWalk};
    float speedSprint{kSpeedSprint};
    float baseSonarDmgScale{11.5f};
    float sonarBaseDamageScale{11.5f};
    int enemyMeleeDamage{1};
    int enemyBoltDamage{1};
    int startingDecoys{3};
    int startingEchoTraps{3};

    // Noise per action
    float noiseSneakPerSec{noise::kRateSneakPerSec};
    float noiseWalkPerSec{noise::kRateWalkPerSec};
    float noiseSprintPerSec{noise::kRateSprintPerSec};
    float noiseSilenceDecayPerSec{noise::kDecaySilencePerSec};
    float noiseWhistle{noise::kCostWhistle};
    float noisePing{noise::kCostPing};
    float noiseWaterStep{noise::kCostWaterStep};
    float noiseDecoy{noise::kCostDecoyImpact};
    float noiseCombatHit{noise::kCostCombatHit};
    float noiseRadialPulse{noise::kCostRadialPulse};
    float noiseStomp{noise::kCostStomp};
    float noiseGlassShatter{noise::kCostGlassShatter};

    // 13 Enemy Archetype Balance Rows (indexed by EnemyVariant 0..12)
    std::array<EnemyBalanceSpec, 13> enemies{{
        {42.0f,  1.00f, 78.0f, 128.0f, 178.0f, 1}, // 0: Stalker
        {95.0f,  0.88f, 78.0f, 128.0f, 178.0f, 1}, // 1: Warden
        {38.0f,  0.65f, 78.0f, 128.0f, 178.0f, 1}, // 2: Shrieker
        {52.0f,  1.15f, 78.0f, 128.0f, 178.0f, 1}, // 3: Phantom
        {240.0f, 0.88f, 78.0f, 128.0f, 178.0f, 1}, // 4: AbyssalBoss (Choir Sovereign)
        {40.0f,  1.12f, 78.0f, 128.0f, 178.0f, 1}, // 5: SilentStalker
        {155.0f, 0.76f, 78.0f, 128.0f, 178.0f, 1}, // 6: DeafBrute
        {58.0f,  1.24f, 78.0f, 128.0f, 178.0f, 1}, // 7: Mimic
        {28.0f,  1.22f, 78.0f, 128.0f, 178.0f, 1}, // 8: EchoSwarm
        {34.0f,  1.28f, 78.0f, 128.0f, 178.0f, 1}, // 9: Leech (Acoustic Parasite)
        {48.0f,  0.94f, 78.0f, 128.0f, 178.0f, 1}, // 10: Banshee (Wailing Siren)
        {76.0f,  0.84f, 78.0f, 128.0f, 178.0f, 1}, // 11: TuningForkConstruct (Resonance Automaton)
        {150.0f, 0.96f, 78.0f, 128.0f, 178.0f, 1}  // 12: TheConductor (Room 16 Mini-Boss)
    }};
};

constexpr BalanceTable kDefaultBalance{};

} // namespace balance
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
    BossDissonanceShock = 14,
    BansheeWail = 15,
    LeechDrain = 16
};

// Stage 1.5: Readability of Fear — human-readable HUD icon & label for the latest noise source
inline constexpr const char* noiseActionHudLabel(NoiseAction action) noexcept {
    switch (action) {
        case NoiseAction::StepSneak:           return "[SNEAK] SOFT STEP (+2/s)";
        case NoiseAction::StepWalk:            return "[WALK] FOOTSTEP (+8/s)";
        case NoiseAction::StepSprint:          return "[SPRINT] HEAVY RUN (+25/s)";
        case NoiseAction::DirectionalPing:     return "[PING] SONAR CONE (+12)";
        case NoiseAction::RadialPulse:         return "[PULSE] RADIAL WAVE (+30)";
        case NoiseAction::DecoyImpact:         return "[DECOY] STONE LURE (+15)";
        case NoiseAction::CombatHit:           return "[IMPACT] RESONANCE HIT (+10)";
        case NoiseAction::SilenceDecay:        return "[SILENCE] QUIET DECAY (-10/s)";
        case NoiseAction::WhistlePing:         return "[WHISTLE] NARROW BEAM (+5)";
        case NoiseAction::StompImpact:         return "[STOMP] SEISMIC SHOCK (+35)";
        case NoiseAction::GlassShatter:        return "[GLASS] BARRIER SHATTER (+40)";
        case NoiseAction::WaterSplash:         return "[WATER] PUDDLE SPLASH (+15)";
        case NoiseAction::EchoSwarmAmplify:    return "[SWARM] PARASITE RELAY (+12)";
        case NoiseAction::EchoStormPulse:      return "[STORM] AMBIENT SWEEP (+6)";
        case NoiseAction::BossDissonanceShock: return "[CHOIR] DISSONANCE WAVE (+15)";
        case NoiseAction::BansheeWail:         return "[BANSHEE] PIERCING WAIL (+22)";
        case NoiseAction::LeechDrain:          return "[LEECH] PARASITE FEED (+8)";
    }
    return "[SILENCE] QUIET";
}

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
        case NoiseAction::BansheeWail:
            delta = cfg::noise::kCostBansheeWail * gainMultiplier;
            break;
        case NoiseAction::LeechDrain:
            delta = cfg::noise::kCostLeechDrain * gainMultiplier;
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

// 8 Surface Materials (including Stage 2 EchoCrystal)
enum class SurfaceMaterial : std::uint8_t {
    Stone = 0,
    Concrete = 1,
    Metal = 2,
    Water = 3,
    DampMoss = 4,
    ResonanceDoor = 5,
    Glass = 6,
    EchoCrystal = 7,
    Count = 8
};

// Stage 2: 4 Acoustic Biomes of 8 Chambers Each (32 Rooms total)
enum class BiomeId : std::uint8_t {
    DrownedGalleries = 0, // Rooms 0..7   (Water, Concrete: x1.5 sound range, muffled steps, water ripples)
    CrystalVaults = 1,    // Rooms 8..15  (EchoCrystal, Glass: chaotic ray splitting & shatter noise; Room 16 = The Conductor)
    IronFoundry = 2,      // Rooms 16..23 (Metal, Concrete: long reverberation & periodic steam-hammer masking)
    LivingMycelium = 3,   // Rooms 24..31 (DampMoss, Stone: sound absorption, fast silence decay & spore enemy reveal)
    Count = 4
};

struct BiomeSpec {
    BiomeId id;
    const char* name;
    const char* acousticRuleDesc;
    SurfaceMaterial primaryMaterial;
    SurfaceMaterial secondaryMaterial;
    float soundRangeMult;       // 1.5x in DrownedGalleries, 0.82x in LivingMycelium
    float stepNoiseMult;        // 0.75x muffled footsteps in DrownedGalleries
    float silenceDecayMult;     // 1.50x faster decay in LivingMycelium
};

inline constexpr BiomeSpec getBiomeSpec(BiomeId id) noexcept {
    switch (id) {
        case BiomeId::DrownedGalleries:
            return {
                BiomeId::DrownedGalleries,
                "DROWNED GALLERIES",
                "WATER CONDUCTS SOUND x1.5 FARTHER, MUFFLES SOFT STEPS & VISUALIZES WAVES",
                SurfaceMaterial::Water,
                SurfaceMaterial::Concrete,
                1.50f, 0.75f, 1.00f
            };
        case BiomeId::CrystalVaults:
            return {
                BiomeId::CrystalVaults,
                "CRYSTAL VAULTS",
                "ECHO CRYSTALS SPLIT & SCATTER RAYS; BRITTLE GLASS SHATTERS LOUDLY (+40 NOISE)",
                SurfaceMaterial::EchoCrystal,
                SurfaceMaterial::Glass,
                1.15f, 1.05f, 0.95f
            };
        case BiomeId::IronFoundry:
            return {
                BiomeId::IronFoundry,
                "IRON FOUNDRY",
                "METAL REVERBERATES (+2 BOUNCES); PERIODIC FOUNDRY PRESS MASKS FOOTSTEPS",
                SurfaceMaterial::Metal,
                SurfaceMaterial::Concrete,
                1.10f, 1.00f, 0.90f
            };
        case BiomeId::LivingMycelium:
        default:
            return {
                BiomeId::LivingMycelium,
                "LIVING MYCELIUM",
                "MYCELIUM ABSORBS SOUND (-18% RANGE, +50% SILENCE DECAY) & SPORES REVEAL HUNTERS",
                SurfaceMaterial::DampMoss,
                SurfaceMaterial::Stone,
                0.82f, 0.80f, 1.50f
            };
    }
}

inline constexpr const char* biomeName(BiomeId id) noexcept {
    return getBiomeSpec(id).name;
}

inline constexpr BiomeId biomeForRoomIndex(std::size_t roomIndex) noexcept {
    const std::size_t idx = (roomIndex / 8u) % static_cast<std::size_t>(BiomeId::Count);
    return static_cast<BiomeId>(idx);
}

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

// Implemented in src/Relics.cpp
RelicSpec getRelicSpec(RelicId id) noexcept;
RelicId getPedestalRelicFromPool(std::size_t index) noexcept;

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
        case SurfaceMaterial::EchoCrystal:
            // Stage 2: Crystal Vaults — splits & scatters sonar rays chaotically with high reflectivity
            return {0.96f, 0.38f, 3.8f, 10.0f, 2, 1.25f, sf::Color(232, 121, 249), sf::Color(88, 28, 135), "ECHO CRYSTAL"};
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

// Implemented in src/Codex.cpp
CodexEntrySpec getCodexEntrySpec(int index) noexcept;

} // namespace echo
