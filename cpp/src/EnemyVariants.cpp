#include "EnemyAI.hpp"
#include <algorithm>
#include <cmath>

namespace echo {

const char* EnemyAI::variantName(EnemyVariant v) noexcept {
    switch (v) {
        case EnemyVariant::Stalker:             return "STALKER";
        case EnemyVariant::Warden:              return "HEAVY WARDEN";
        case EnemyVariant::Shrieker:            return "SONIC SHRIEKER";
        case EnemyVariant::Phantom:             return "PHASE PHANTOM";
        case EnemyVariant::AbyssalBoss:         return "THE CHOIR SOVEREIGN";
        case EnemyVariant::SilentStalker:       return "SILENT HUNTER";
        case EnemyVariant::DeafBrute:           return "DEAF BRUTE";
        case EnemyVariant::Mimic:               return "RESONANCE MIMIC";
        case EnemyVariant::EchoSwarm:           return "ECHO SWARM";
        case EnemyVariant::Leech:               return "ACOUSTIC LEECH";
        case EnemyVariant::Banshee:             return "WAILING BANSHEE";
        case EnemyVariant::TuningForkConstruct: return "TUNING FORK CONSTRUCT";
        case EnemyVariant::TheConductor:        return "THE CONDUCTOR";
    }
    return "HUNTER";
}

float EnemyAI::hitRadius() const noexcept {
    switch (m_variant) {
        case EnemyVariant::AbyssalBoss:         return cfg::kEnemyRadius + 10.0f;
        case EnemyVariant::TheConductor:        return cfg::kEnemyRadius + 7.5f;
        case EnemyVariant::DeafBrute:           return cfg::kEnemyRadius + 5.5f;
        case EnemyVariant::Warden:              return cfg::kEnemyRadius + 4.0f;
        case EnemyVariant::TuningForkConstruct: return cfg::kEnemyRadius + 3.5f;
        case EnemyVariant::EchoSwarm:           return cfg::kEnemyRadius + 2.0f;
        case EnemyVariant::Leech:               return cfg::kEnemyRadius - 2.0f;
        default:                                return cfg::kEnemyRadius;
    }
}

void EnemyAI::configurePatrol(const std::vector<sf::Vector2f>& waypoints, EnemyVariant variant) {
    m_waypoints = waypoints;
    m_variant = variant;
    m_waypointIdx = 0;
    m_suspicion = 0.0f;
    m_stateTimer = 0.0f;
    m_tremorTimer = 0.0f;
    m_rangedAttackTimer = 1.6f;
    m_hitFlashTimer = 0.0f;
    m_staggerTimer = 0.0f;
    m_alertBlend = 0.0f;
    m_difficultySpeedMult = 1.0f;
    m_meleeTelegraphProgress = 0.0f;
    m_rangedTelegraphProgress = 0.0f;
    m_telegraphCuePending = false;
    m_rangedTelegraphEmitted = false;
    m_disguised = (variant == EnemyVariant::Mimic);
    m_inhaleWarningEmitted = false;
    m_triggeredSwarmAmplification = false;
    m_swarmAmplifyCooldown = 0.0f;
    m_triggeredBansheeWail = false;
    m_bansheeWailCooldown = 2.4f;
    m_leechedOnPlayer = false;
    m_triggeredLeechDrain = false;
    m_leechDrainTimer = 0.0f;

    for (auto& b : m_bolts) b.active = false;

    const auto idx = std::min<std::size_t>(
        static_cast<std::size_t>(variant),
        cfg::balance::kDefaultBalance.enemies.size() - 1
    );
    m_maxHp = cfg::balance::kDefaultBalance.enemies[idx].maxHp;
    m_hp = m_maxHp;

    if (m_waypoints.empty()) {
        m_active = false;
        m_state = EnemyState::Idle;
    } else {
        m_active = true;
        m_position = m_waypoints.front();
        m_targetPos = m_waypoints.front();
        m_state = (variant == EnemyVariant::Mimic) ? EnemyState::Idle : EnemyState::Patrol;
    }
}

void EnemyAI::applyDifficultyScaling(float hpMult, float speedMult) noexcept {
    const float clampedHpMult = std::clamp(hpMult, 0.4f, 3.0f);
    m_maxHp = std::max(10.0f, m_maxHp * clampedHpMult);
    m_hp = m_maxHp;
    m_difficultySpeedMult = std::clamp(speedMult, 0.5f, 2.0f);
}

float EnemyAI::attackTelegraphProgress() const noexcept {
    return std::clamp(std::max(m_meleeTelegraphProgress, m_rangedTelegraphProgress), 0.0f, 1.0f);
}

bool EnemyAI::consumeTelegraphAlert() noexcept {
    const bool pending = m_telegraphCuePending;
    m_telegraphCuePending = false;
    return pending;
}

void EnemyAI::applyNoiseTierStimulus(
    sf::Vector2f stimulusPos,
    NoiseTier tier,
    bool isGroundVibration
) noexcept {
    if (!m_active) return;

    // Block 4.2: DeafBrute does not hear airborne sounds — only reacts to ground vibrations!
    if (m_variant == EnemyVariant::DeafBrute && !isGroundVibration) {
        return;
    }

    // Block 4.3: Disguised Mimic stays motionless in disguise until player approaches or scans/damages it
    if (m_variant == EnemyVariant::Mimic && m_disguised) {
        return;
    }

    switch (tier) {
        case NoiseTier::Quiet:
            break;

        case NoiseTier::Audible:
            m_targetPos = stimulusPos;
            m_suspicion = std::max(m_suspicion, 42.0f);
            if (m_state == EnemyState::Patrol || m_state == EnemyState::ReturnToPatrol || m_state == EnemyState::Idle) {
                m_state = EnemyState::InvestigateSound;
                m_stateTimer = 4.2f;
            }
            break;

        case NoiseTier::Loud:
            m_targetPos = stimulusPos;
            m_suspicion = std::max(m_suspicion, 72.0f);
            if (m_state != EnemyState::Attack && m_state != EnemyState::PursuePlayer) {
                m_state = EnemyState::InvestigateSound;
                m_stateTimer = 5.0f;
            }
            break;

        case NoiseTier::Scream:
            m_targetPos = stimulusPos;
            m_suspicion = 100.0f;
            if (m_state != EnemyState::Attack) {
                m_state = EnemyState::PursuePlayer;
                m_stateTimer = 6.0f;
            }
            break;
    }
}

bool EnemyAI::consumeSwarmAmplification() noexcept {
    const bool triggered = m_triggeredSwarmAmplification;
    m_triggeredSwarmAmplification = false;
    return triggered;
}

bool EnemyAI::consumeBansheeWail() noexcept {
    const bool triggered = m_triggeredBansheeWail;
    m_triggeredBansheeWail = false;
    return triggered;
}

bool EnemyAI::consumeLeechDrainPulse() noexcept {
    const bool triggered = m_triggeredLeechDrain;
    m_triggeredLeechDrain = false;
    return triggered;
}

bool EnemyAI::detachLeech(sf::Vector2f pushOrigin) noexcept {
    if (!m_active || m_variant != EnemyVariant::Leech || !m_leechedOnPlayer) {
        return false;
    }
    m_leechedOnPlayer = false;
    m_leechDrainTimer = 0.0f;
    m_staggerTimer = 1.15f;
    m_hitFlashTimer = 0.25f;
    m_state = EnemyState::SearchLastKnown;
    m_stateTimer = 2.5f;
    const sf::Vector2f delta = m_position - pushOrigin;
    const float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    const sf::Vector2f dir = (dist > 1e-3f) ? (delta / dist) : sf::Vector2f{1.0f, 0.0f};
    m_position = pushOrigin + dir * 48.0f;
    return true;
}

bool EnemyAI::applyStompKnockback(
    sf::Vector2f stompOrigin,
    float stompRadius,
    float knockbackDist,
    const std::vector<WallSegment>& walls
) noexcept {
    if (!m_active) return false;
    const sf::Vector2f delta = m_position - stompOrigin;
    const float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (dist > stompRadius + hitRadius()) {
        return false;
    }

    m_leechedOnPlayer = false;
    const sf::Vector2f pushDir = (dist > 1e-3f) ? (delta / dist) : sf::Vector2f{1.0f, 0.0f};
    const float falloff = std::clamp(1.0f - (dist / std::max(1.0f, stompRadius * 1.25f)), 0.35f, 1.0f);
    const float scale = (m_variant == EnemyVariant::AbyssalBoss || m_variant == EnemyVariant::TheConductor) ? 0.45f : 1.0f;

    m_position += pushDir * (knockbackDist * falloff * scale);
    resolveWallCollisions(walls);

    m_staggerTimer = std::max(m_staggerTimer, 0.48f);
    m_hitFlashTimer = 0.25f;
    m_meleeTelegraphProgress = 0.0f;
    m_rangedTelegraphProgress = 0.0f;
    m_disguised = false;

    m_targetPos = stompOrigin;
    m_suspicion = std::min(100.0f, m_suspicion + 55.0f);
    if (m_state != EnemyState::Attack) {
        m_state = EnemyState::PursuePlayer;
        m_stateTimer = 4.8f;
    }
    return true;
}

bool EnemyAI::applyAcousticDamage(
    float amount,
    bool isReflectedHit,
    sf::Vector2f sourcePos,
    const std::vector<WallSegment>& walls,
    bool isHighFreqHit
) noexcept {
    if (!m_active || amount <= 0.0f) return false;

    float finalDmg = amount;
    // Stealth Ambush Bonus: hitting an unalerted enemy deals 1.45x resonance damage
    if (m_state == EnemyState::Patrol || m_state == EnemyState::Idle) {
        finalDmg *= 1.45f;
    }
    // Phantom takes reduced damage from direct pings, but bonus damage from reflected wall echoes
    if (m_variant == EnemyVariant::Phantom) {
        finalDmg *= isReflectedHit ? 1.50f : 0.65f;
    } else if ((m_variant == EnemyVariant::Warden || m_variant == EnemyVariant::DeafBrute) && !isReflectedHit) {
        finalDmg *= 0.80f;
    } else if (m_variant == EnemyVariant::EchoSwarm && isHighFreqHit) {
        finalDmg *= 2.0f;
    } else if (m_variant == EnemyVariant::TuningForkConstruct) {
        // Stage 2: TuningForkConstruct dampens non-resonant Low/Mid direct hits (0.35x),
        // but sympathetically shatters (2.40x) when hit by High-Frequency [3] or reflected rays!
        finalDmg *= (isHighFreqHit || isReflectedHit) ? 2.40f : 0.35f;
    } else if (m_variant == EnemyVariant::TheConductor) {
        // Stage 2: Room 16 Mini-Boss takes 1.65x bonus damage from reflected or High-Frequency [3] sonar rays
        finalDmg *= (isHighFreqHit || isReflectedHit) ? 1.65f : 0.78f;
    } else if (m_variant == EnemyVariant::AbyssalBoss && bossAcousticPhase() == BossArenaAcousticPhase::AbyssalSilence) {
        finalDmg *= (isReflectedHit || isHighFreqHit) ? 1.85f : 0.65f;
    }

    if (m_variant == EnemyVariant::Leech && m_leechedOnPlayer) {
        detachLeech(sourcePos);
    }

    if (m_variant == EnemyVariant::Mimic && m_disguised) {
        m_disguised = false;
        m_state = EnemyState::PursuePlayer;
        m_stateTimer = 5.5f;
        m_suspicion = 100.0f;
    }

    m_hp = std::max(0.0f, m_hp - finalDmg);
    m_hitFlashTimer = 0.24f;
    m_staggerTimer = (m_variant == EnemyVariant::AbyssalBoss || m_variant == EnemyVariant::TheConductor || m_variant == EnemyVariant::DeafBrute)
        ? 0.06f
        : ((m_variant == EnemyVariant::TuningForkConstruct && (isHighFreqHit || isReflectedHit)) ? 0.55f : 0.16f);
    // Hitting an enemy with sonar interrupts its melee windup slightly
    m_meleeTelegraphProgress = std::max(0.0f, m_meleeTelegraphProgress - 0.35f);

    const sf::Vector2f away = m_position - sourcePos;
    const float dist = std::sqrt(away.x * away.x + away.y * away.y);
    if (dist > 1e-3f) {
        const float kb = (m_variant == EnemyVariant::AbyssalBoss || m_variant == EnemyVariant::TheConductor || m_variant == EnemyVariant::DeafBrute) ? 3.5f : 9.0f;
        m_position += (away / dist) * kb;
        resolveWallCollisions(walls);
    }

    m_targetPos = sourcePos;
    m_suspicion = std::min(100.0f, m_suspicion + 35.0f);
    if (m_state != EnemyState::PursuePlayer && m_state != EnemyState::Attack) {
        m_state = (m_suspicion >= 70.0f) ? EnemyState::PursuePlayer : EnemyState::InvestigateSound;
        m_stateTimer = 4.5f;
    }

    if (m_hp <= 0.0f) {
        m_active = false;
        m_leechedOnPlayer = false;
        for (auto& b : m_bolts) b.active = false;
        return true;
    }
    return false;
}

} // namespace echo
