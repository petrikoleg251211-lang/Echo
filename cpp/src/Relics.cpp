#include "Config.hpp"
#include "Player.hpp"
#include <algorithm>
#include <array>

namespace echo {

namespace {
constexpr std::array<RelicId, 11> kPedestalPool = {
    RelicId::HarmonicAmplifier,
    RelicId::MossTreadBoots,
    RelicId::PrismaticMirror,
    RelicId::PiezoArmorCore,
    RelicId::PhantomDecoyRig,
    RelicId::ResonanceCapacitor,
    RelicId::SubharmonicVisor,
    RelicId::EchoSiphonCoil,
    RelicId::PhaseDampener,
    RelicId::UltrasonicLens,
    RelicId::AbyssalTuningFork
};
} // namespace

RelicSpec getRelicSpec(RelicId id) noexcept {
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

RelicId getPedestalRelicFromPool(std::size_t index) noexcept {
    return kPedestalPool[index % kPedestalPool.size()];
}

bool Player::hasRelic(RelicId id) const noexcept {
    return std::find(m_relics.begin(), m_relics.end(), id) != m_relics.end();
}

bool Player::addRelic(RelicId id, AcousticSystem* acoustics) {
    if (hasRelic(id)) return false;
    m_relics.push_back(id);

    if (id == RelicId::PiezoArmorCore) {
        if (!m_oneLifeLocked) {
            m_maxHealth += 2;
            m_health = std::min(m_maxHealth, m_health + 2);
        }
    } else if (id == RelicId::PhantomDecoyRig) {
        m_decoyCount += 3;
        m_echoTrapCount += 2;
    } else if (id == RelicId::UltrasonicLens) {
        m_echoTrapCount += 2;
    } else if (id == RelicId::PrismaticMirror && acoustics != nullptr) {
        const auto nextRefl = static_cast<std::uint8_t>(std::min<int>(8, acoustics->maxReflections() + 2));
        acoustics->setMaxReflections(nextRefl);
    }
    return true;
}

float Player::sonarDamageMultiplier(bool isReflectedHit, bool isHighFreqHit) const noexcept {
    float mult = 1.0f;
    if (hasRelic(RelicId::HarmonicAmplifier)) {
        mult *= 1.40f;
    }
    if (isReflectedHit) {
        mult *= hasRelic(RelicId::PrismaticMirror) ? 1.55f : 1.20f;
        // Synergy: HarmonicAmplifier + PrismaticMirror = Phase-Locked Resonance Beam
        if (hasRelic(RelicId::HarmonicAmplifier) && hasRelic(RelicId::PrismaticMirror)) {
            mult *= 1.25f;
        }
    }
    if (isHighFreqHit) {
        mult *= hasRelic(RelicId::UltrasonicLens) ? 1.56f : 1.12f;
    }
    if (hasRelic(RelicId::AbyssalTuningFork)) {
        mult *= 1.30f;
    }
    return mult;
}

float Player::resonatorChargeMultiplier() const noexcept {
    return hasRelic(RelicId::EchoSiphonCoil) ? 2.0f : 1.0f;
}

float Player::noiseGainMultiplier(bool isFootstep) const noexcept {
    float mult = 1.0f;
    if (hasRelic(RelicId::SilentStep)) {
        mult *= 0.65f;
    }
    if (isFootstep && hasRelic(RelicId::MossTreadBoots)) {
        mult *= 0.55f;
    }
    if (!isFootstep && hasRelic(RelicId::PhaseDampener)) {
        mult *= 0.70f;
    }
    return mult;
}

float Player::silenceDecayMultiplier() const noexcept {
    return hasRelic(RelicId::PhaseDampener) ? 1.50f : 1.0f;
}

float Player::shardGainMultiplier() const noexcept {
    return hasRelic(RelicId::AbyssalTuningFork) ? 1.25f : 1.0f;
}

} // namespace echo
