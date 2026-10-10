#include "EnemyAI.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
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

EnemyAI::EnemyAI() = default;

const char* EnemyAI::stateName(EnemyState s) noexcept {
    switch (s) {
        case EnemyState::Idle:             return "DORMANT";
        case EnemyState::Patrol:           return "PATROLLING";
        case EnemyState::InvestigateSound: return "INVESTIGATING";
        case EnemyState::SearchLastKnown:  return "SEARCHING";
        case EnemyState::PursuePlayer:     return "HUNTING";
        case EnemyState::Attack:           return "LETHAL CONTACT";
        case EnemyState::ReturnToPatrol:   return "RETURNING";
    }
    return "PATROLLING";
}

void EnemyAI::configurePatrol(const std::vector<sf::Vector2f>& waypoints) {
    m_waypoints = waypoints;
    m_waypointIdx = 0;
    m_suspicion = 0.0f;
    m_stateTimer = 0.0f;
    m_tremorTimer = 0.0f;
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
    for (int iter = 0; iter < 4; ++iter) {
        for (const auto& wall : walls) {
            if (wall.isOpen) continue;
            const sf::Vector2f closest = closestPointOnSegment(m_position, wall.a, wall.b);
            const sf::Vector2f delta = m_position - closest;
            const float distSq = delta.x * delta.x + delta.y * delta.y;
            const float minDist = cfg::kEnemyRadius + 2.5f;
            if (distSq < minDist * minDist && distSq > 1e-6f) {
                const float dist = std::sqrt(distSq);
                m_position += (delta / dist) * (minDist - dist);
            }
        }
    }
}

void EnemyAI::steerToward(
    sf::Vector2f target,
    float speed,
    float dt,
    const std::vector<WallSegment>& walls
) noexcept {
    sf::Vector2f goal = target;

    // If direct line to target is blocked by a wall, steer toward the nearest visible patrol node first
    if (!hasLineOfSight(target, walls) && !m_waypoints.empty()) {
        float bestScore = 1e9f;
        for (const auto& wp : m_waypoints) {
            if (hasLineOfSight(wp, walls)) {
                const float score = distanceBetween(m_position, wp) + distanceBetween(wp, target);
                if (score < bestScore) {
                    bestScore = score;
                    goal = wp;
                }
            }
        }
    }

    const sf::Vector2f delta = goal - m_position;
    const float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (dist < 2.0f) return;

    const sf::Vector2f dir = delta / dist;
    m_position += dir * (speed * dt);
    resolveWallCollisions(walls);
}

bool EnemyAI::update(
    float dt,
    sf::Vector2f playerPos,
    const std::vector<WallSegment>& walls,
    AcousticSystem& acoustics
) {
    if (!m_active || m_state == EnemyState::Attack) return false;

    m_animPhase += dt * 4.5f;
    const EnemyState prevState = m_state;

    const float distToPlayer = distanceBetween(m_position, playerPos);
    if (distToPlayer <= (cfg::kPlayerRadius + cfg::kEnemyRadius - 2.0f)) {
        m_state = EnemyState::Attack;
        return true;
    }

    // 1. Evaluate acoustic events emitted in the current frame using material-aware multi-wall transmission
    for (const auto& ev : acoustics.recentEvents()) {
        if (!ev.emittedByPlayer) continue;
        const float dist = distanceBetween(m_position, ev.origin);
        if (dist > ev.radius * 1.15f) continue;

        const float transmissionFactor = AcousticSystem::computeTransmissionFactor(ev.origin, m_position, walls);
        const float perceived = ev.loudness * transmissionFactor * (1.0f - dist / (ev.radius * 1.25f));

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

    // 2. Decay suspicion & update finite state machine
    m_suspicion = std::max(0.0f, m_suspicion - 6.5f * dt);

    switch (m_state) {
        case EnemyState::Idle:
            break;

        case EnemyState::Patrol: {
            if (!m_waypoints.empty()) {
                const sf::Vector2f wp = m_waypoints[m_waypointIdx];
                steerToward(wp, 78.0f, dt, walls);
                if (distanceBetween(m_position, wp) < 18.0f) {
                    m_waypointIdx = (m_waypointIdx + 1) % m_waypoints.size();
                }
            }
            break;
        }

        case EnemyState::InvestigateSound: {
            steerToward(m_targetPos, 128.0f, dt, walls);
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
            steerToward(m_targetPos + orbitOffset, 92.0f, dt, walls);
            if (m_stateTimer <= 0.0f) {
                m_state = EnemyState::ReturnToPatrol;
            }
            break;
        }

        case EnemyState::PursuePlayer: {
            steerToward(m_targetPos, 178.0f, dt, walls);
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
            steerToward(home, 88.0f, dt, walls);
            if (distanceBetween(m_position, home) < 20.0f) {
                m_state = EnemyState::Patrol;
            }
            break;
        }

        case EnemyState::Attack:
            break;
    }

    // 3. Emit crimson acoustic tremor from enemy footsteps
    m_tremorTimer += dt;
    const float tremorInterval = (m_state == EnemyState::PursuePlayer) ? 0.42f : 0.85f;
    if (m_tremorTimer >= tremorInterval) {
        m_tremorTimer = 0.0f;
        acoustics.emitPulse(
            m_position,
            255.0f,
            190.0f,
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

void EnemyAI::render(sf::RenderTarget& target, float echoIllumination) const {
    if (!m_active) return;

    const float baseVis = (m_state == EnemyState::PursuePlayer)
        ? 0.92f
        : (m_state == EnemyState::InvestigateSound ? 0.72f : 0.35f);
    const float vis = std::clamp(std::max(baseVis, echoIllumination), 0.0f, 1.0f);
    const auto alpha = static_cast<sf::Uint8>(vis * 240.0f);

    // Outer acoustic membrane ring
    const float pulseR = cfg::kEnemyRadius + 5.0f + std::sin(m_animPhase) * 2.5f;
    sf::CircleShape outerRing(pulseR);
    outerRing.setOrigin(pulseR, pulseR);
    outerRing.setPosition(m_position);
    outerRing.setFillColor(sf::Color(45, 10, 18, static_cast<sf::Uint8>(vis * 110.0f)));
    outerRing.setOutlineThickness(1.5f);
    outerRing.setOutlineColor(sf::Color(239, 68, 68, static_cast<sf::Uint8>(vis * 145.0f)));
    target.draw(outerRing);

    // Inner core
    sf::CircleShape core(cfg::kEnemyRadius - 2.0f);
    core.setOrigin(cfg::kEnemyRadius - 2.0f, cfg::kEnemyRadius - 2.0f);
    core.setPosition(m_position);
    core.setFillColor(sf::Color(24, 8, 14, alpha));
    core.setOutlineThickness(2.0f);
    core.setOutlineColor(sf::Color(248, 113, 113, alpha));
    target.draw(core);

    // Acoustic sensor tendrils
    sf::VertexArray tendrils(sf::Lines);
    for (int i = 0; i < 6; ++i) {
        const float a = m_animPhase * 0.6f + static_cast<float>(i) * 1.0471975f;
        const sf::Vector2f dir{std::cos(a), std::sin(a)};
        tendrils.append(sf::Vertex(m_position + dir * 6.0f, sf::Color(248, 113, 113, alpha)));
        tendrils.append(sf::Vertex(m_position + dir * (pulseR + 4.0f), sf::Color(239, 68, 68, 0)));
    }
    target.draw(tendrils);
}

} // namespace echo
