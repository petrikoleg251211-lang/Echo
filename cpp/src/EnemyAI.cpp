#include "EnemyAI.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <algorithm>
#include <cmath>

namespace echo {

namespace {
float distanceBetween(sf::Vector2f a, sf::Vector2f b) noexcept {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

float cross2(sf::Vector2f u, sf::Vector2f v) noexcept {
    return u.x * v.y - u.y * v.x;
}

bool segmentsIntersectSimple(sf::Vector2f p, sf::Vector2f p2, sf::Vector2f q, sf::Vector2f q2) noexcept {
    const sf::Vector2f r = p2 - p;
    const sf::Vector2f s = q2 - q;
    const float rxs = cross2(r, s);
    if (std::abs(rxs) < 1e-6f) return false;
    const sf::Vector2f qp = q - p;
    const float t = cross2(qp, s) / rxs;
    const float u = cross2(qp, r) / rxs;
    return (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f);
}

sf::Vector2f closestPointOnSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) noexcept {
    const sf::Vector2f ab = b - a;
    const float lenSq = ab.x * ab.x + ab.y * ab.y;
    if (lenSq < 1e-6f) return a;
    const float t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lenSq, 0.0f, 1.0f);
    return a + ab * t;
}
} // namespace

EnemyAI::EnemyAI() {
    m_bolts.resize(24);
}

const char* EnemyAI::stateName(EnemyState s) noexcept {
    switch (s) {
        case EnemyState::Idle:             return "DORMANT";
        case EnemyState::Patrol:           return "PATROLLING";
        case EnemyState::InvestigateSound: return "INVESTIGATING";
        case EnemyState::SearchLastKnown:  return "SEARCHING";
        case EnemyState::PursuePlayer:     return "HUNTING";
        case EnemyState::Attack:           return "CONTACT";
        case EnemyState::ReturnToPatrol:   return "RETURNING";
    }
    return "PATROLLING";
}

const char* EnemyAI::variantName(EnemyVariant v) noexcept {
    switch (v) {
        case EnemyVariant::Stalker:     return "STALKER";
        case EnemyVariant::Warden:      return "HEAVY WARDEN";
        case EnemyVariant::Shrieker:    return "SONIC SHRIEKER";
        case EnemyVariant::Phantom:     return "PHASE PHANTOM";
        case EnemyVariant::AbyssalBoss: return "THE CHOIR SOVEREIGN";
    }
    return "HUNTER";
}

float EnemyAI::hitRadius() const noexcept {
    switch (m_variant) {
        case EnemyVariant::AbyssalBoss: return cfg::kEnemyRadius + 10.0f;
        case EnemyVariant::Warden:      return cfg::kEnemyRadius + 4.0f;
        default:                        return cfg::kEnemyRadius;
    }
}

int EnemyAI::bossPhase() const noexcept {
    if (m_variant != EnemyVariant::AbyssalBoss || m_maxHp <= 0.0f) return 1;
    const float ratio = m_hp / m_maxHp;
    if (ratio <= 0.33f) return 3;
    if (ratio <= 0.66f) return 2;
    return 1;
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

    for (auto& b : m_bolts) b.active = false;

    switch (variant) {
        case EnemyVariant::Stalker:     m_maxHp = 42.0f; break;
        case EnemyVariant::Warden:      m_maxHp = 95.0f; break;
        case EnemyVariant::Shrieker:    m_maxHp = 38.0f; break;
        case EnemyVariant::Phantom:     m_maxHp = 52.0f; break;
        case EnemyVariant::AbyssalBoss: m_maxHp = 240.0f; break;
    }
    m_hp = m_maxHp;

    if (m_waypoints.empty()) {
        m_active = false;
        m_state = EnemyState::Idle;
    } else {
        m_active = true;
        m_position = m_waypoints.front();
        m_targetPos = m_waypoints.front();
        m_state = EnemyState::Patrol;
    }
}

void EnemyAI::applyNoiseTierStimulus(sf::Vector2f stimulusPos, NoiseTier tier) noexcept {
    if (!m_active) return;

    switch (tier) {
        case NoiseTier::Quiet:
            // [0.0, 30.0]: Quiet — enemies remain in their current patrol routine
            break;

        case NoiseTier::Audible:
            // (30.0, 60.0): Audible — transition from Patrol/ReturnToPatrol to InvestigateSound
            m_targetPos = stimulusPos;
            m_suspicion = std::max(m_suspicion, 42.0f);
            if (m_state == EnemyState::Patrol || m_state == EnemyState::ReturnToPatrol || m_state == EnemyState::Idle) {
                m_state = EnemyState::InvestigateSound;
                m_stateTimer = 4.2f;
            }
            break;

        case NoiseTier::Loud:
            // [60.0, 85.0): Loud — enemies move directly toward the sound origin with high suspicion
            m_targetPos = stimulusPos;
            m_suspicion = std::max(m_suspicion, 72.0f);
            if (m_state != EnemyState::Attack && m_state != EnemyState::PursuePlayer) {
                m_state = EnemyState::InvestigateSound;
                m_stateTimer = 5.0f;
            }
            break;

        case NoiseTier::Scream:
            // [85.0, 100.0]: Scream — priority hunt lock on the player position
            m_targetPos = stimulusPos;
            m_suspicion = 100.0f;
            if (m_state != EnemyState::Attack) {
                m_state = EnemyState::PursuePlayer;
                m_stateTimer = 6.0f;
            }
            break;
    }
}

void EnemyAI::clearContactState() noexcept {
    if (m_state == EnemyState::Attack) {
        m_state = EnemyState::PursuePlayer;
        m_stateTimer = 3.5f;
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
            b.radius = (m_variant == EnemyVariant::AbyssalBoss) ? 7.5f : 5.8f;
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
        const float d = distanceBetween(b.pos, playerPos);
        if (d <= b.radius + playerRadius) {
            b.active = false;
            return true;
        }
    }
    return false;
}

bool EnemyAI::applyAcousticDamage(
    float amount,
    bool isReflectedHit,
    sf::Vector2f sourcePos,
    const std::vector<WallSegment>& walls
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
    } else if (m_variant == EnemyVariant::Warden && !isReflectedHit) {
        // Armored Warden has frontal plating; bounced shots bypass armor
        finalDmg *= 0.80f;
    }

    m_hp = std::max(0.0f, m_hp - finalDmg);
    m_hitFlashTimer = 0.24f;
    m_staggerTimer = (m_variant == EnemyVariant::AbyssalBoss) ? 0.06f : 0.16f;

    // Knockback away from sound pulse origin
    const sf::Vector2f away = m_position - sourcePos;
    const float dist = std::sqrt(away.x * away.x + away.y * away.y);
    if (dist > 1e-3f) {
        const float kb = (m_variant == EnemyVariant::AbyssalBoss) ? 3.5f : 9.0f;
        m_position += (away / dist) * kb;
        resolveWallCollisions(walls);
    }

    // Alert enemy toward the source of the sonar disruption
    m_targetPos = sourcePos;
    m_suspicion = std::min(100.0f, m_suspicion + 35.0f);
    if (m_state != EnemyState::PursuePlayer && m_state != EnemyState::Attack) {
        m_state = (m_suspicion >= 70.0f) ? EnemyState::PursuePlayer : EnemyState::InvestigateSound;
        m_stateTimer = 4.5f;
    }

    if (m_hp <= 0.0f) {
        m_active = false;
        for (auto& b : m_bolts) b.active = false;
        return true;
    }
    return false;
}

bool EnemyAI::hasLineOfSight(sf::Vector2f target, const std::vector<WallSegment>& walls) const noexcept {
    for (const auto& wall : walls) {
        if (wall.isOpen) continue;
        if (segmentsIntersectSimple(m_position, target, wall.a, wall.b)) {
            return false;
        }
    }
    return true;
}

void EnemyAI::resolveWallCollisions(const std::vector<WallSegment>& walls) noexcept {
    const float radius = hitRadius();
    for (int iter = 0; iter < 4; ++iter) {
        for (const auto& wall : walls) {
            if (wall.isOpen) continue;
            const sf::Vector2f closest = closestPointOnSegment(m_position, wall.a, wall.b);
            const sf::Vector2f delta = m_position - closest;
            const float distSq = delta.x * delta.x + delta.y * delta.y;
            const float minDist = radius + 2.5f;
            if (distSq < minDist * minDist && distSq > 1e-6f) {
                const float dist = std::sqrt(distSq);
                m_position += (delta / dist) * (minDist - dist);
            }
        }
    }
}

void EnemyAI::steerToward(sf::Vector2f target, float speed, float dt, const std::vector<WallSegment>& walls) noexcept {
    sf::Vector2f desiredTarget = target;

    if (!hasLineOfSight(target, walls) && !m_waypoints.empty()) {
        float bestScore = 1e9f;
        for (const auto& wp : m_waypoints) {
            if (hasLineOfSight(wp, walls)) {
                const float score = distanceBetween(m_position, wp) * 0.45f + distanceBetween(wp, target);
                if (score < bestScore) {
                    bestScore = score;
                    desiredTarget = wp;
                }
            }
        }
    }

    const sf::Vector2f delta = desiredTarget - m_position;
    const float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (dist > 1.5f) {
        const sf::Vector2f dir = delta / dist;
        m_facingAngle = std::atan2(dir.y, dir.x);
        m_position += dir * (speed * dt);
        resolveWallCollisions(walls);
    }
}

bool EnemyAI::update(
    float dt,
    sf::Vector2f playerPos,
    const std::vector<WallSegment>& walls,
    AcousticSystem& acoustics
) {
    if (!m_active) return false;

    m_hitFlashTimer = std::max(0.0f, m_hitFlashTimer - dt);
    m_staggerTimer = std::max(0.0f, m_staggerTimer - dt);

    // Update active ranged SonicBolts
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
            if (segmentsIntersectSimple(b.pos, nextPos, w.a, w.b)) {
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

    float speedScale = 1.0f;
    if (m_variant == EnemyVariant::Warden) speedScale = 0.88f;
    else if (m_variant == EnemyVariant::Shrieker) speedScale = 0.65f;
    else if (m_variant == EnemyVariant::Phantom) speedScale = 1.15f;
    else if (m_variant == EnemyVariant::AbyssalBoss) {
        const int phase = bossPhase();
        speedScale = (phase == 3) ? 1.25f : (phase == 2 ? 1.05f : 0.88f);
    }

    if (m_staggerTimer > 0.0f) {
        speedScale *= 0.25f;
    }

    const float animSpeed = (m_state == EnemyState::PursuePlayer) ? 8.5f : 4.5f;
    m_animPhase += dt * animSpeed;

    const float targetAlert = (m_state == EnemyState::PursuePlayer)
        ? 1.0f
        : (m_state == EnemyState::InvestigateSound || m_state == EnemyState::SearchLastKnown ? 0.55f : 0.0f);
    m_alertBlend += (targetAlert - m_alertBlend) * std::min(1.0f, dt * 5.0f);

    const EnemyState prevState = m_state;

    const float catchRadius = cfg::kPlayerRadius + hitRadius() - 1.5f;
    const float distToPlayer = distanceBetween(m_position, playerPos);
    if (distToPlayer <= catchRadius) {
        m_state = EnemyState::Attack;
        return true;
    }

    for (const auto& ev : acoustics.recentEvents()) {
        if (!ev.emittedByPlayer) continue;
        const float dist = distanceBetween(m_position, ev.origin);
        if (dist > ev.radius * 1.15f) continue;

        const float transmissionFactor = AcousticSystem::computeTransmissionFactor(ev.origin, m_position, walls);
        const float sensitivity = (m_variant == EnemyVariant::Shrieker) ? 1.20f : 1.0f;
        const float perceived = ev.loudness * sensitivity * transmissionFactor * (1.0f - dist / (ev.radius * 1.25f));

        if (perceived > 0.14f) {
            m_targetPos = ev.origin;
            m_suspicion = std::min(100.0f, m_suspicion + perceived * 54.0f);

            if (perceived > 0.65f || m_suspicion >= 74.0f) {
                m_state = EnemyState::PursuePlayer;
                m_stateTimer = 5.0f;
            } else if (m_state != EnemyState::PursuePlayer) {
                m_state = EnemyState::InvestigateSound;
                m_stateTimer = 4.4f;
            }
        }
    }

    m_suspicion = std::max(0.0f, m_suspicion - 6.5f * dt);

    switch (m_state) {
        case EnemyState::Idle:
            break;

        case EnemyState::Patrol: {
            if (!m_waypoints.empty()) {
                const sf::Vector2f wp = m_waypoints[m_waypointIdx];
                steerToward(wp, 78.0f * speedScale, dt, walls);
                if (distanceBetween(m_position, wp) < 18.0f) {
                    m_waypointIdx = (m_waypointIdx + 1) % m_waypoints.size();
                }
            }
            break;
        }

        case EnemyState::InvestigateSound: {
            steerToward(m_targetPos, 128.0f * speedScale, dt, walls);
            m_stateTimer -= dt;
            if (distanceBetween(m_position, m_targetPos) < 20.0f || m_stateTimer <= 0.0f) {
                m_state = EnemyState::SearchLastKnown;
                m_stateTimer = 3.2f;
            }
            break;
        }

        case EnemyState::SearchLastKnown: {
            m_stateTimer -= dt;
            const sf::Vector2f orbitOffset{
                std::cos(m_stateTimer * 2.6f) * 42.0f,
                std::sin(m_stateTimer * 2.6f) * 42.0f
            };
            steerToward(m_targetPos + orbitOffset, 92.0f * speedScale, dt, walls);
            if (m_stateTimer <= 0.0f) {
                m_state = EnemyState::ReturnToPatrol;
            }
            break;
        }

        case EnemyState::PursuePlayer: {
            steerToward(m_targetPos, 178.0f * speedScale, dt, walls);
            m_stateTimer -= dt;
            if (distanceBetween(m_position, m_targetPos) < 20.0f && distToPlayer > 44.0f) {
                m_state = EnemyState::SearchLastKnown;
                m_stateTimer = 3.5f;
            } else if (m_stateTimer <= 0.0f) {
                m_state = EnemyState::SearchLastKnown;
                m_stateTimer = 3.0f;
            }
            break;
        }

        case EnemyState::ReturnToPatrol: {
            if (m_waypoints.empty()) {
                m_state = EnemyState::Idle;
                break;
            }
            const sf::Vector2f home = m_waypoints[m_waypointIdx];
            steerToward(home, 88.0f * speedScale, dt, walls);
            if (distanceBetween(m_position, home) < 20.0f) {
                m_state = EnemyState::Patrol;
            }
            break;
        }

        case EnemyState::Attack:
            break;
    }

    // Ranged SonicBolt attacks for Shrieker and AbyssalBoss when alerted
    if ((m_variant == EnemyVariant::Shrieker || m_variant == EnemyVariant::AbyssalBoss) &&
        (m_state == EnemyState::InvestigateSound || m_state == EnemyState::PursuePlayer || m_state == EnemyState::SearchLastKnown)) {
        m_rangedAttackTimer -= dt;
        if (m_rangedAttackTimer <= 0.0f) {
            const sf::Vector2f toTarget = m_targetPos - m_position;
            const float baseAng = std::atan2(toTarget.y, toTarget.x);

            if (m_variant == EnemyVariant::Shrieker) {
                m_rangedAttackTimer = 2.1f;
                spawnSonicBolt({std::cos(baseAng), std::sin(baseAng)}, 250.0f);
            } else {
                // AbyssalBoss multi-phase sonic patterns
                const int phase = bossPhase();
                m_rangedAttackTimer = (phase == 3) ? 1.35f : (phase == 2 ? 1.75f : 2.15f);
                if (phase == 1) {
                    // Phase 1: 3-way directional fan
                    for (int k = -1; k <= 1; ++k) {
                        const float a = baseAng + static_cast<float>(k) * 0.26f;
                        spawnSonicBolt({std::cos(a), std::sin(a)}, 265.0f);
                    }
                } else {
                    // Phase 2 & 3: Radial nova + aimed center bolt
                    const int count = (phase == 3) ? 8 : 6;
                    for (int k = 0; k < count; ++k) {
                        const float a = baseAng + static_cast<float>(k) * (6.2831853f / static_cast<float>(count));
                        spawnSonicBolt({std::cos(a), std::sin(a)}, 245.0f);
                    }
                }
            }
        }
    }

    m_tremorTimer += dt;
    const float tremorInterval = (m_state == EnemyState::PursuePlayer) ? 0.42f : 0.85f;
    if (m_tremorTimer >= tremorInterval) {
        m_tremorTimer = 0.0f;
        const float tremorRadius = (m_variant == EnemyVariant::AbyssalBoss)
            ? 310.0f
            : ((m_variant == EnemyVariant::Warden) ? 235.0f : 190.0f);
        acoustics.emitPulse(
            m_position,
            255.0f,
            tremorRadius,
            0.55f,
            28,
            WaveSignature::EnemyTremor,
            0.0f,
            6.2831853f,
            false
        );
    }

    const bool newlyAlerted =
        (prevState == EnemyState::Patrol || prevState == EnemyState::ReturnToPatrol) &&
        (m_state == EnemyState::InvestigateSound || m_state == EnemyState::PursuePlayer);
    return newlyAlerted;
}

void EnemyAI::render(
    sf::RenderTarget& target,
    float echoIllumination,
    sf::Vector2f playerPos,
    const AssetManager* assets
) const {
    if (!m_active) return;

    // 1. Render active ranged SonicBolts
    for (const auto& b : m_bolts) {
        if (!b.active) continue;
        sf::CircleShape halo(b.radius + 4.0f);
        halo.setOrigin(b.radius + 4.0f, b.radius + 4.0f);
        halo.setPosition(b.pos);
        halo.setFillColor(sf::Color(244, 63, 94, 70));
        target.draw(halo);

        sf::CircleShape core(b.radius);
        core.setOrigin(b.radius, b.radius);
        core.setPosition(b.pos);
        core.setFillColor(sf::Color(253, 164, 175));
        core.setOutlineThickness(1.5f);
        core.setOutlineColor(sf::Color(225, 29, 72));
        target.draw(core);
    }

    float baseVis = (m_state == EnemyState::PursuePlayer)
        ? 0.94f
        : (m_state == EnemyState::InvestigateSound || m_state == EnemyState::SearchLastKnown ? 0.75f : 0.36f);
    if (m_variant == EnemyVariant::Phantom && m_hitFlashTimer <= 0.0f) {
        baseVis *= 0.42f; // Cloaked phase stalker is harder to see without sonar echo illumination
    }
    const float vis = std::clamp(std::max(baseVis, echoIllumination), 0.12f, 1.0f);
    const auto alpha = static_cast<sf::Uint8>(vis * 245.0f);

    const float radius = hitRadius();

    // 2. Outer acoustic resonance membrane
    const float pulseR = radius + 6.0f + std::sin(m_animPhase) * (2.5f + m_alertBlend * 2.5f);
    sf::CircleShape outerRing(pulseR);
    outerRing.setOrigin(pulseR, pulseR);
    outerRing.setPosition(m_position);
    outerRing.setFillColor(sf::Color(48, 10, 18, static_cast<sf::Uint8>(vis * 105.0f)));
    outerRing.setOutlineThickness(1.5f);
    outerRing.setOutlineColor(
        m_hitFlashTimer > 0.0f
            ? sf::Color(125, 211, 252, 240)
            : sf::Color(239, 68, 68, static_cast<sf::Uint8>(vis * 145.0f))
    );
    target.draw(outerRing);

    // 3. Articulated 6-limb stride animation
    const sf::Vector2f forward{std::cos(m_facingAngle), std::sin(m_facingAngle)};
    const sf::Vector2f right{-forward.y, forward.x};
    sf::VertexArray legs(sf::Lines);

    const float legReach = radius + 6.0f + m_alertBlend * 4.0f;
    for (int side = -1; side <= 1; side += 2) {
        for (int legIdx = 0; legIdx < 3; ++legIdx) {
            const float phaseOffset = static_cast<float>(legIdx) * 2.094395f + (side > 0 ? 1.570796f : 0.0f);
            const float strideSwing = std::sin(m_animPhase + phaseOffset) * (5.5f + m_alertBlend * 3.5f);
            const float longOffset = (static_cast<float>(legIdx) - 1.0f) * 8.0f;

            const sf::Vector2f hip = m_position + forward * (longOffset * 0.4f) + right * (static_cast<float>(side) * 6.0f);
            const sf::Vector2f knee = m_position
                + forward * (longOffset * 0.8f + strideSwing * 0.5f)
                + right * (static_cast<float>(side) * (legReach * 0.62f))
                + sf::Vector2f{0.0f, -4.5f};
            const sf::Vector2f foot = m_position
                + forward * (longOffset + strideSwing)
                + right * (static_cast<float>(side) * legReach);

            const sf::Color hipCol(248, 113, 113, alpha);
            const sf::Color kneeCol(251, 146, 60, alpha);
            const sf::Color footCol(239, 68, 68, static_cast<sf::Uint8>(vis * 110.0f));

            legs.append(sf::Vertex(hip, hipCol));
            legs.append(sf::Vertex(knee, kneeCol));
            legs.append(sf::Vertex(knee, kneeCol));
            legs.append(sf::Vertex(foot, footCol));
        }
    }
    target.draw(legs);

    // 4. Render 2.5D 3D Mesh via AssetManager
    sf::Color baseCol(52, 12, 22, alpha);
    sf::Color rimCol(248, 113, 113, alpha);

    if (m_variant == EnemyVariant::Warden) {
        baseCol = sf::Color(72, 16, 28, alpha);
        rimCol = sf::Color(251, 146, 60, alpha);
    } else if (m_variant == EnemyVariant::Shrieker) {
        baseCol = sf::Color(48, 16, 68, alpha);
        rimCol = sf::Color(216, 180, 254, alpha);
    } else if (m_variant == EnemyVariant::Phantom) {
        baseCol = sf::Color(16, 42, 58, alpha);
        rimCol = sf::Color(45, 212, 191, alpha);
    } else if (m_variant == EnemyVariant::AbyssalBoss) {
        baseCol = sf::Color(88, 12, 36, alpha);
        rimCol = (bossPhase() == 3) ? sf::Color(250, 204, 21, alpha) : sf::Color(244, 63, 94, alpha);
    }

    if (m_hitFlashTimer > 0.0f) {
        baseCol = sf::Color(56, 189, 248, 235);
        rimCol = sf::Color(224, 242, 254, 255);
    }

    if (assets != nullptr && assets->isInitialized()) {
        ModelId modelId = ModelId::ListenerStalker;
        float meshScale = 16.0f;
        if (m_variant == EnemyVariant::Warden) {
            modelId = ModelId::ListenerWarden;
            meshScale = 19.5f;
        } else if (m_variant == EnemyVariant::Shrieker) {
            modelId = ModelId::ResonatorCrystal;
            meshScale = 17.5f;
        } else if (m_variant == EnemyVariant::AbyssalBoss) {
            modelId = ModelId::ListenerWarden;
            meshScale = 27.0f;
        }
        const float pitchLunge = (m_state == EnemyState::PursuePlayer || m_state == EnemyState::Attack) ? 0.24f : 0.05f;
        assets->renderMesh25D(
            target,
            modelId,
            m_position,
            m_facingAngle,
            meshScale,
            baseCol,
            rimCol,
            vis,
            playerPos,
            pitchLunge
        );
    } else {
        sf::CircleShape core(radius - 2.0f);
        core.setOrigin(radius - 2.0f, radius - 2.0f);
        core.setPosition(m_position);
        core.setFillColor(baseCol);
        core.setOutlineThickness(2.0f);
        core.setOutlineColor(rimCol);
        target.draw(core);
    }

    // 5. Render compact harmonic stability bar above damaged or Boss enemies
    if (m_hp < m_maxHp || m_variant == EnemyVariant::AbyssalBoss) {
        const float barW = (m_variant == EnemyVariant::AbyssalBoss) ? 58.0f : 32.0f;
        const float barH = 4.0f;
        const sf::Vector2f barPos{m_position.x - barW * 0.5f, m_position.y - radius - 16.0f};

        sf::RectangleShape bg({barW, barH});
        bg.setPosition(barPos);
        bg.setFillColor(sf::Color(15, 23, 42, 200));
        bg.setOutlineThickness(1.0f);
        bg.setOutlineColor(sf::Color(71, 85, 105, 180));
        target.draw(bg);

        const float ratio = std::clamp(m_hp / std::max(1.0f, m_maxHp), 0.0f, 1.0f);
        sf::RectangleShape fill({barW * ratio, barH});
        fill.setPosition(barPos);
        fill.setFillColor(m_hitFlashTimer > 0.0f ? sf::Color(125, 211, 252) : sf::Color(244, 63, 94));
        target.draw(fill);
    }
}

} // namespace echo
