#include "EnemyAI.hpp"
#include <algorithm>
#include <cmath>

namespace echo {

namespace {
float distanceBetweenBoss(sf::Vector2f a, sf::Vector2f b) noexcept {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

float cross2Boss(sf::Vector2f u, sf::Vector2f v) noexcept {
    return u.x * v.y - u.y * v.x;
}

bool segmentsIntersectBoss(sf::Vector2f p, sf::Vector2f p2, sf::Vector2f q, sf::Vector2f q2) noexcept {
    const sf::Vector2f r = p2 - p;
    const sf::Vector2f s = q2 - q;
    const float rxs = cross2Boss(r, s);
    if (std::abs(rxs) < 1e-6f) return false;
    const sf::Vector2f qp = q - p;
    const float t = cross2Boss(qp, s) / rxs;
    const float u = cross2Boss(qp, r) / rxs;
    return (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f);
}
} // namespace

int EnemyAI::bossPhase() const noexcept {
    if (m_maxHp <= 0.0f) return 1;
    if (m_variant == EnemyVariant::TheConductor) {
        return (m_hp / m_maxHp <= 0.50f) ? 2 : 1;
    }
    if (m_variant != EnemyVariant::AbyssalBoss) return 1;
    const float ratio = m_hp / m_maxHp;
    if (ratio <= 0.33f) return 3;
    if (ratio <= 0.66f) return 2;
    return 1;
}

BossArenaAcousticPhase EnemyAI::bossAcousticPhase() const noexcept {
    switch (bossPhase()) {
        case 3:  return BossArenaAcousticPhase::AbyssalSilence;
        case 2:  return BossArenaAcousticPhase::DissonantPulse;
        case 1:
        default: return BossArenaAcousticPhase::HarmonicChoir;
    }
}

void EnemyAI::spawnSonicBolt(sf::Vector2f dir, float speed) noexcept {
    const float lenSq = dir.x * dir.x + dir.y * dir.y;
    if (lenSq < 1e-6f) return;
    dir /= std::sqrt(lenSq);

    for (auto& b : m_bolts) {
        if (!b.active) {
            b.pos = m_position + dir * (hitRadius() + 6.0f);
            b.vel = dir * speed;
            b.radius = (m_variant == EnemyVariant::AbyssalBoss || m_variant == EnemyVariant::TheConductor) ? 7.2f : 5.8f;
            b.lifetime = 3.2f;
            b.active = true;
            return;
        }
    }
}

bool EnemyAI::consumeBoltHitOnPlayer(sf::Vector2f playerPos, float playerRadius) noexcept {
    if (!m_active) return false;
    for (auto& b : m_bolts) {
        if (!b.active) continue;
        const float d = distanceBetweenBoss(b.pos, playerPos);
        if (d <= b.radius + playerRadius) {
            b.active = false;
            return true;
        }
    }
    return false;
}

void EnemyAI::updateBossAndRangedVolleys(
    float dt,
    const std::vector<WallSegment>& walls,
    AcousticSystem& acoustics
) noexcept {
    // 1. Update active ranged SonicBolts and wall collisions
    for (auto& b : m_bolts) {
        if (!b.active) continue;
        const sf::Vector2f nextPos = b.pos + b.vel * dt;
        b.lifetime -= dt;
        if (b.lifetime <= 0.0f) {
            b.active = false;
            continue;
        }
        bool hitWall = false;
        for (const auto& w : walls) {
            if (w.isOpen) continue;
            if (segmentsIntersectBoss(b.pos, nextPos, w.a, w.b)) {
                hitWall = true;
                break;
            }
        }
        if (hitWall) {
            b.active = false;
        } else {
            b.pos = nextPos;
        }
    }

    // 2. Fire ranged SonicBolt volleys for Shrieker, TuningForkConstruct, TheConductor (Room 16 Mini-Boss), and Choir Sovereign (AbyssalBoss)
    //    Stage 1.1: Telegraph ranged attack 0.42s before firing with visual + acoustic cue
    const bool isRangedCapable =
        (m_variant == EnemyVariant::Shrieker ||
         m_variant == EnemyVariant::TuningForkConstruct ||
         m_variant == EnemyVariant::TheConductor ||
         m_variant == EnemyVariant::AbyssalBoss);

    if (isRangedCapable &&
        (m_state == EnemyState::InvestigateSound || m_state == EnemyState::PursuePlayer || m_state == EnemyState::SearchLastKnown)) {
        m_rangedAttackTimer -= dt;

        if (m_rangedAttackTimer <= cfg::feel::kEnemyRangedTelegraphSec && m_rangedAttackTimer > 0.0f) {
            m_rangedTelegraphProgress = std::clamp(
                1.0f - (m_rangedAttackTimer / cfg::feel::kEnemyRangedTelegraphSec),
                0.0f,
                1.0f
            );
            if (!m_rangedTelegraphEmitted) {
                m_rangedTelegraphEmitted = true;
                m_telegraphCuePending = true;
                acoustics.emitPulse(
                    m_position,
                    360.0f,
                    165.0f,
                    0.75f,
                    24,
                    WaveSignature::EnemyTremor,
                    0.0f,
                    6.2831853f,
                    false
                );
            }
        } else {
            m_rangedTelegraphProgress = 0.0f;
        }

        if (m_rangedAttackTimer <= 0.0f) {
            m_rangedTelegraphProgress = 0.0f;
            m_rangedTelegraphEmitted = false;
            const sf::Vector2f toTarget = m_targetPos - m_position;
            const float baseAng = std::atan2(toTarget.y, toTarget.x);

            if (m_variant == EnemyVariant::Shrieker) {
                m_rangedAttackTimer = 2.1f;
                spawnSonicBolt({std::cos(baseAng), std::sin(baseAng)}, 250.0f);
            } else if (m_variant == EnemyVariant::TuningForkConstruct) {
                m_rangedAttackTimer = 2.3f;
                spawnSonicBolt({std::cos(baseAng - 0.14f), std::sin(baseAng - 0.14f)}, 255.0f);
                spawnSonicBolt({std::cos(baseAng + 0.14f), std::sin(baseAng + 0.14f)}, 255.0f);
            } else if (m_variant == EnemyVariant::TheConductor) {
                const int phase = bossPhase();
                m_rangedAttackTimer = (phase == 2) ? 1.55f : 2.00f;
                const int count = (phase == 2) ? 6 : 4;
                for (int k = 0; k < count; ++k) {
                    const float a = baseAng + static_cast<float>(k) * (6.2831853f / static_cast<float>(count));
                    spawnSonicBolt({std::cos(a), std::sin(a)}, 255.0f);
                }
            } else {
                const int phase = bossPhase();
                m_rangedAttackTimer = (phase == 3) ? 1.35f : (phase == 2 ? 1.75f : 2.15f);
                if (phase == 1) {
                    for (int k = -1; k <= 1; ++k) {
                        const float a = baseAng + static_cast<float>(k) * 0.26f;
                        spawnSonicBolt({std::cos(a), std::sin(a)}, 265.0f);
                    }
                } else {
                    const int count = (phase == 3) ? 8 : 6;
                    for (int k = 0; k < count; ++k) {
                        const float a = baseAng + static_cast<float>(k) * (6.2831853f / static_cast<float>(count));
                        spawnSonicBolt({std::cos(a), std::sin(a)}, 245.0f);
                    }
                }
            }
        }
    } else {
        m_rangedTelegraphProgress = 0.0f;
        m_rangedTelegraphEmitted = false;
    }
}

} // namespace echo
